#include <memory>
#include <utility>
#include <mutex>
#include <vector>
#include <algorithm>

namespace memory {
    
template<std::size_t MAX_OBJ_SIZE>
struct alignas(8) object_holder
{
    using object_t  = unsigned char[MAX_OBJ_SIZE] ;
    object_holder() : object() {}

    template<typename T, typename ...Args>
    T* construct_inner(Args&&... args) {
        return new(&object) T(std::forward<Args>(args)...) ;
    }

    template<typename T>
    void destruct_inner() {
        reinterpret_cast<T&>(object).~T() ;
    }
    alignas(8) object_t object;
};


template<std::size_t MAX_OBJ_SIZE>
class object_pool
{
public:
    object_pool (std::size_t maxCapacity, std::size_t minCapacity) :
        indexes_(maxCapacity),
        pool_impl_(maxCapacity),
        min_capacity_(minCapacity) , max_capacity_(maxCapacity),
        first_(std::begin(indexes_))
    { 
            std::transform(std::begin(pool_impl_), 
                           std::end(pool_impl_) , 
                           indexes_.begin() , 
                           [] (object_impl_t &o){ return &o ; });
    }

    template<typename Object, typename ...Args>
    void alloc (std::shared_ptr<Object> & object, Args&&... args) {
       std::lock_guard<std::mutex> guard(lock_) ;
       if ( first_ != std::end(indexes_) ) {
           Object *p = (*first_)->template construct_inner<Object>( std::forward<Args>(args)... );          
           std::advance (first_ , std::min(1,std::distance(first_,indexes_.end()) ) ) ; //std::next up to std::end         
           object =  std::shared_ptr<Object>(p, [this]( Object *p )
           {
               std::lock_guard<std::mutex> guard(lock_) ;               
               std::advance (first_ , -std::min(1,std::distance(indexes_.begin(),first_) ) ) ; //std::prev down to std::begin
               object_impl_t *o = reinterpret_cast<object_impl_t*>(p) ;
               *first_ = o ;  
               o->template destruct_inner<Object>() ;
           }) ;
       }
       else {
           //TODO: asynch even to resize 
           object = std::shared_ptr<Object>(new Object(std::forward<Args>(args)...)) ;
       }
    }

private:
    void increase_capacity() { ; } //TODO: implement it and call it asynch
    using object_impl_t = object_holder<MAX_OBJ_SIZE> ;
    using pool_impl_t =  std::vector<object_impl_t> ;
    using index_buffer_t = std::vector<object_impl_t*> ;
    index_buffer_t indexes_ ;
    pool_impl_t pool_impl_ ;
    std::size_t min_capacity_ ;
    std::size_t max_capacity_ ;
    std::mutex lock_ ;
    typename index_buffer_t::iterator first_ ; //pointer to first free slot in the pool 
};


} //namespace 


#include <iostream>

struct A { A( const std::string &s_) : s(s_) {} ~A() {std::clog<<"~A()"<<std::endl;}   std::string s; };
struct B { B( const std::string &s_ , const std::string &t_) : s(s_), t(t_) {} ~B(){std::clog<<"~B()"<<std::endl;} std::string s; std::string t; };
struct C { C( int i_ ) : i(i_) {} ~C(){std::clog<<"~B()"<<std::endl;} int i; };



template<typename... Types> struct max_size ;


template<typename T, typename... Types>
struct max_size<T, Types...>
{ 
   static const std::size_t value  = sizeof(T) > max_size<Types...>::value ? sizeof(T) : max_size<Types...>::value ; 
} ;

template <typename  T>
struct max_size<T>
{
    static const std::size_t value = sizeof(T);
} ;

/*
 * Test Cases below :
 */



int main(int argc, char** argv) {
    memory::object_pool<max_size<A,B,C>::value> my_pool(10,100) ;
    std::shared_ptr<A> a ;
    my_pool.alloc(a, std::string("vlad")) ;
    {
    std::shared_ptr<B> b ;
    my_pool.alloc(b, std::string("Vlad") , std::string("Venediktov")) ;
    std::clog << "name=" << b->s << ",last_name=" << b->t << std::endl ;
    }
    std::clog << "name=" << a->s << std::endl ;
 
    return 0;
}

