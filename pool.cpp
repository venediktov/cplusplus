#include <memory>
#include <utility>
#include <mutex>
#include <vector>
#include <forward_list>
#include <algorithm>
#include <future>


namespace memory {
    
template< class Function, class... Args>
std::future<typename std::result_of<Function(Args...)>::type> async( Function&& f, Args&&... args ) 
{
    typedef typename std::result_of<Function(Args...)>::type R;
    auto bound_task = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
    std::packaged_task<R()> task(std::move(bound_task));
    auto ret = task.get_future();
    std::thread t(std::move(task));
    t.detach();
    return ret;   
}

template<std::size_t MAX_OBJ_SIZE>
struct alignas(8) object_holder
{
    using object_t  = unsigned char[MAX_OBJ_SIZE] ;
    object_holder() : object() {}

    template<typename T, typename ...Args>
    T* construct_inner(Args&&... args) {
        static_assert (sizeof(T) <= MAX_OBJ_SIZE, "size of object is not supported");
        return new(&object) T(std::forward<Args>(args)...) ;
    }

    template<typename T>
    void destruct_inner() {
        reinterpret_cast<T&>(object).~T() ;
    }
    alignas(8) object_t object;
};


template<std::size_t MAX_OBJ_SIZE, 
         std::size_t MAX_CAPACITY=1024 , 
         std::size_t MIN_CAPACITY=128>
class object_pool
{
public:
    object_pool () : 
        indexes_(MAX_CAPACITY), pool_impl_(), first_(), 
        capacity_threshhold_(MAX_CAPACITY-MIN_CAPACITY), 
        increase_in_progress_(),
        lock_()
    { 
        segment_ptr sp(new memory_chunk_t) ;
        pool_impl_.push_front(sp) ;
        std::transform(std::begin(*sp),  std::end(*sp) , 
                       std::begin(indexes_) , [] (object_impl_t &o){ return &o ; });
    }

    template<typename Object, typename ...Args>
    void alloc (std::shared_ptr<Object> & object, Args&&... args) {
       std::lock_guard<std::mutex> guard {lock_} ;
       if ( first_ < capacity_threshhold_ ) {
           Object *p = indexes_[first_++]->template construct_inner<Object>( std::forward<Args>(args)... );          
           object =  std::shared_ptr<Object>(p, [this]( Object *p )
           {
               std::lock_guard<std::mutex> guard {lock_} ; 
               object_impl_t *o = reinterpret_cast<object_impl_t*>(p) ;
               indexes_[--first_] = o ;  
               o->template destruct_inner<Object>() ;
           }) ;
       }
       else { 
           if ( !increase_in_progress_) {
               //std::clog << "Calling asynch tid" << std::this_thread::get_id() << std::endl;
               increase_in_progress_=true;
               memory::async(&object_pool::increase_capacity, this);
           }
           //std::clog << "Getting from heap first_=" << first_ << ", tid=" << std::this_thread::get_id() << std::endl;
           object = std::shared_ptr<Object>(new Object(std::forward<Args>(args)...)) ;
       }
    }
    object_pool (object_pool const &) =  delete ;
    object_pool (object_pool &&) =  delete ;
    object_pool & operator=(object_pool const &) = delete ;
    object_pool && operator=(object_pool &&) = delete ;
private:
    void increase_capacity() {
        std::lock_guard<std::mutex> guard {lock_} ; 
        //std::clog << "tid=" << std::this_thread::get_id() <<",first_=" << first_ << ", increassing capacity from " << indexes_.size() << ", to " << indexes_.size()+MAX_CAPACITY << std::endl;
        std::size_t last_size=indexes_.size() ;
        indexes_.resize(last_size+MAX_CAPACITY) ;
        segment_ptr sp(new memory_chunk_t) ;
        pool_impl_.push_front(sp) ;
        std::transform(std::begin(*sp),  std::end(*sp) , 
                       &indexes_[last_size] , [] (object_impl_t &o){ return &o ; });
        capacity_threshhold_ *= 2 ;
        increase_in_progress_=false;
        return ;
    }
    using object_impl_t = object_holder<MAX_OBJ_SIZE> ;
    using memory_chunk_t = std::array<object_impl_t,MAX_CAPACITY> ;
    using segment_ptr = std::shared_ptr<memory_chunk_t> ; 
    using pool_impl_t =  std::forward_list<segment_ptr> ;
    using index_buffer_t = std::vector<object_impl_t*> ;
    index_buffer_t indexes_ ;
    pool_impl_t pool_impl_ ;
    std::size_t first_ ; //index to first free address in the pool 
    std::size_t capacity_threshhold_ ;
    bool increase_in_progress_ ;
    std::mutex lock_ ;
};


} //namespace 


#include <iostream>

struct A { A( const std::string &s_) : s(s_) {} ~A() {std::clog<<"~A()"<<std::endl;}   std::string s; };
struct B { B( const std::string &s_ , const std::string &t_) : s(s_), t(t_) {} ~B(){std::clog<<"~B()"<<std::endl;} std::string s; std::string t; };
struct C { C( const std::string &s ) : i() , t() , r() {} ~C(){std::clog<<"~C()"<<std::endl;} int i; int t; int r ; };



template<typename... Types> struct max_size ;

template<typename T, typename... Types>
struct max_size<T, Types...>
{ 
   static constexpr std::size_t value  = sizeof(T) > max_size<Types...>::value ? sizeof(T) : max_size<Types...>::value ; 
} ;

template <typename  T>
struct max_size<T>
{
    static constexpr std::size_t value = sizeof(T);
} ;

/*
 * Test Cases below :
 */
#include <future>
#include <chrono>

template <typename... Types> struct parallel ;

template <typename T>
struct parallel<T> {
    static constexpr std::size_t count=1;
    template <typename Pool, typename... Args>
    static auto fetch(const Pool &pool, Args&&... args) -> decltype(std::chrono::system_clock::now() - std::chrono::system_clock::now())
    {
        std::shared_ptr<T> t ;
        auto start = std::chrono::system_clock::now();
        const_cast<Pool &>(pool).alloc(t, std::forward<Args>(args)...) ;
        auto end = std::chrono::system_clock::now();
        auto result = end - start ;
        //std::clog << "elapsed=" << std::chrono::duration_cast<std::chrono::microseconds>(result).count() << ", tid=" << std::this_thread::get_id() << std::endl ;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return result;
    }
};

template <typename T, typename... Types>
struct parallel<T,Types...> {
    static constexpr std::size_t count = parallel<T>::count + parallel<Types...>::count ;
    template <typename Pool, typename... Args>
    static auto fetch(const Pool &pool, Args&&... args) -> decltype(std::chrono::system_clock::now() - std::chrono::system_clock::now())
    {
        auto handle = std::async(std::launch::async,
                                 parallel<T>::template fetch<Pool,Args...>,  std::cref(pool), std::forward<Args>(args)...);
        auto elapsed = parallel<Types...>::template fetch<Pool,Args...>(pool, std::forward<Args>(args)...);
        return handle.get() + elapsed ;
    }
};

int main(int argc, char** argv) {
    memory::object_pool<max_size<A,B,C>::value, 100, 10> my_pool ; ;
    std::shared_ptr<A> a ;
    my_pool.alloc(a, std::string("vlad")) ;
    {
    std::shared_ptr<B> b ;
    my_pool.alloc(b, std::string("Vlad") , std::string("Venediktov")) ;
    std::clog << "name=" << b->s << ",last_name=" << b->t << std::endl ;
    }
    std::clog << "name=" << a->s << std::endl ;

    using test_pool_increase_t = memory::object_pool<max_size<A,C>::value, 5, 2> ;
    test_pool_increase_t parallel_pool ; 
    std::chrono::nanoseconds elapsed(0);
    std::size_t iteration_count=10000;
    std::vector<std::future<std::chrono::nanoseconds>> futures;
    for (int i = 0; i <= iteration_count; ++i) {
        futures.push_back( std::async(std::launch::async,
                                 parallel<A,C,A,C,A,C,A,C,A,C>::fetch<test_pool_increase_t,std::string>,
                                 std::cref(parallel_pool) , std::string("PARALLEL_EXECUTION"))
        ) ;
        if ( futures.size() % 100) {
            for ( auto &f : futures) 
               elapsed += f.get() ;
            futures.resize(0);
        }
    
    }
    
    auto avrg_microsec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()/(parallel<A,C,A,C,A,C,A,C,A,C>::count * iteration_count) ;
    std::cout << "pool average access time =" << avrg_microsec << " microsec" << std::endl ;
    std::cout << "parallel count(checking) = " << parallel<A,C,A,C,A,C,A,C,A,C>::count ;
    return 0;
}

