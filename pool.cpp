#include <memory>
#include <utility>
#include <mutex>
#include <vector>
#include <algorithm>


namespace suisse {
    
template<std::size_t MAX_OBJ_SIZE>
struct object_holder
{
    using object_t  = unsigned char[MAX_OBJ_SIZE] ; //or std::array<uchar,MAX_OBJ_SIZE>

    object_holder() : object() , reclaimed() {}

    template<typename T, typename ...Args>
    T* construct_inner(Args&&... args) {
        return new(&object) T(std::forward<Args>(args)...) ;
    }

    template<typename T>
    void destruct_inner() {
        reinterpret_cast<T&>(object).~T() ;
        reclaimed=true ;
    }
    object_t object;
    bool reclaimed;
};


template<std::size_t MAX_OBJ_SIZE>
class pool
{
public:
    //TODO: resize capacity asynch , and improve find_if
    pool (std::size_t maxCapacity, std::size_t minCapacity) :
        pool_impl_(maxCapacity),
        min_capacity_(minCapacity) , max_capacity_(maxCapacity)
    {}

    template<typename Object, typename ...Args>
    void alloc (std::shared_ptr<Object> & object, Args&&... args) {
       std::lock_guard<std::mutex> guard(lock_) ;
       auto itr = std::find_if(std::begin(pool_impl_), std::end(pool_impl_), [] (const object_impl_t &o) { return !o.reclaimed ; } );
       if ( itr != std::end(pool_impl_) ) {
           Object *p = itr->template construct_inner<Object>( std::forward<Args>(args)... );
           object =  std::shared_ptr<Object>(p, [this]( Object *p )
           {
               std::lock_guard<std::mutex> guard(lock_) ;
               auto itr = std::find_if(std::begin(pool_impl_), std::end(pool_impl_), [p] (const object_impl_t &o) 
               { 
                   return (void*)std::addressof(o.object) == (void*)p; 
               });
               if ( itr != std::end(pool_impl_) ) {
                   itr->template destruct_inner<Object>() ;
               }
           }) ;
       }
       else {
           object = std::shared_ptr<Object>(new Object(std::forward<Args>(args)...)) ;
       }
    }

private:
    void increase_capacity() { ; } //TODO: implement it 
    using object_impl_t = object_holder<MAX_OBJ_SIZE> ;
    using pool_impl_t =  std::vector<object_impl_t> ;
    pool_impl_t pool_impl_ ;
    std::size_t min_capacity_ ;
    std::size_t max_capacity_ ;
    std::mutex lock_ ;
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
    suisse::pool<max_size<A,B,C>::value> my_pool(10,100) ;
    std::shared_ptr<A> a ;
    my_pool.alloc(a, std::string("vlad")) ;
    std::shared_ptr<B> b ;
    my_pool.alloc(b, std::string("Vlad") , std::string("Venediktov")) ;
    std::clog << "name=" << a->s << std::endl ;
    std::clog << "name=" << b->s << ",last_name=" << b->t << std::endl ;
    return 0;
}

