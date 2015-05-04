/* 
 * File:   fixed_string.hpp
 * Author: vladimir Venediktov
 *  It's just implements minimum interfaces, 
 *  the rest is in the base class of boost::array<T,N> / std::array<T,N>
 * Created on May 3, 2015, 1:28 PM
 */

#include <string>
#include <cstring>
#if __cplusplus <= 199711L
#include <boost/array.hpp>
#include <boost/static_assert.hpp> 
#else
#include <array>
#endif

#ifndef __FIXED_STRING_HPP__
#define	__FIXED_STRING_HPP__

#if __cplusplus <= 199711L
namespace std {
    template< class InputIt, class Size, class OutputIt>
    OutputIt copy_n(InputIt first, Size count, OutputIt result)
    {
        if (count > 0) {
            *result++ = *first;
            for (Size i = 1; i < count; ++i) {
                *result++ = *++first;
            }
        }
        return result;
    }
}
#endif

template<typename CharT, unsigned int N>
class fixed_string : public
#if __cplusplus <= 199711L
    boost::array<CharT,N> {
    typedef boost::array<CharT,N> base_t ;
#else
    std::array<CharT,N> {
    using base_t = std::array<CharT, N> ;
#endif
    public :
    fixed_string(const std::basic_string<CharT> &s) : base_t(), impl_string_() {
        base_t::fill(0) ;
        impl_string_.assign(base_t::begin(), std::copy_n(s.begin(), std::min(N,s.size()), base_t::begin() ) );
    }
    
    fixed_string(const CharT * s) : base_t(), impl_string_() {
        base_t::fill(0) ;
        impl_string_.assign(base_t::begin(), std::copy_n(s, std::min(N,std::strlen(s)), base_t::begin() ) );
    }
    
    std::string& operator=(const std::basic_string<CharT> & s)  {
        base_t::fill(0) ;
        return impl_string_.assign(base_t::begin(), std::copy_n(s.begin(), std::min(N,s.size()), base_t::begin() ) );
    }
    std::string& operator=(const CharT * s)  {
        return this->operator =(std::basic_string<CharT>(s)) ;    
    }
    
    operator std::string() {
        return impl_string_ ;
    }
    
     //TODO: assert for size N != 1
     std::basic_string<CharT> & operator=(CharT t) {
#if __cplusplus <= 199711L 
         BOOST_STATIC_ASSERT_MSG(N == 1 , "cannot use operator=(Char T) for N>1" ) ;
#else
         static_assert(N == 1 , "cannot use operator=(Char T) for N>1" ) ;
#endif         
         base_t::elems[0] = t ;
         return impl_string_.assign(1,t) ;
     }
     
     private:
         std::basic_string<CharT> impl_string_ ;
    
};



#endif	/* __FIXED_STRING_HPP__ */

