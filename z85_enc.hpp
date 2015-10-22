
#ifndef _Z85_ENC_HPP_
#define _Z85_ENC_HPP_

#include <vector>
#include <string>
#include <cstdint>
#include <boost/optional.hpp> //<experimental/optional>
#include <boost/math/special_functions/pow.hpp>
		

//Implementation of Z85 encoding
//
// http://rfc.zeromq.org/spec:32
// https://en.wikipedia.org/wiki/Ascii85
//
// Use C++ constructs and best practices when possible
// Encode and decode any length data buffer losslessly
// 
// throw std::exception (with reason) if unable to decode 
// Document any extension that you impliment

//We don't really need a struct or class namespace is just enough
namespace Z85 {
#define VALID_Z85_CHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#"
// Maps base 85 to base 256
// We chop off lower 32 and higher 128 ranges
const std::vector<char> decoder = {
0x00, 0x44, 0x00, 0x54, 0x53, 0x52, 0x48, 0x00,
0x4B, 0x4C, 0x46, 0x41, 0x00, 0x3F, 0x3E, 0x45,
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
0x08, 0x09, 0x40, 0x00, 0x49, 0x42, 0x4A, 0x47,
0x51, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
0x3B, 0x3C, 0x3D, 0x4D, 0x00, 0x4E, 0x43, 0x00,
0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
0x21, 0x22, 0x23, 0x4F, 0x00, 0x50, 0x00, 0x00
};

using encoded_type = boost::optional<std::string> ;
using decoded_type = boost::optional<std::vector<uint8_t> > ;

// Encode binary data with Z85 encoding
decoded_type decode(std::string &data) {
    decoded_type decoded ;
    if (data.size() % 5) { //According to algorithm stop processing , not possible
        return decoded; //instead of throwing exception can return uninitialized string
    }

    decoded = std::vector<uint8_t>() ;
    
    std::function<void (std::string::iterator, std::string::iterator) > func = 
    [&] (std::string::iterator b , std::string::iterator e) 
    {
       if ( b==e) {
           return;
       }
       uint32_t value = std::accumulate(b,e, 0 , [] (uint32_t v, uint8_t x) { return v * 85 + decoder[x-32] ; } )  ;
       uint divisor= boost::math::pow<3>(256);
       while(divisor) {
           //less performance then preallocated string but sufice for now
           uint8_t c = value / divisor % 256 ;
           decoded->push_back(c);
           divisor /=256 ;
       }       
       func(e , e+5 > data.end() ? e : e+5 ) ;
       return; 
    } ;
    func(data.begin(),data.begin()+5) ;
}

// Decode binary data with Z85 encoding
encoded_type encode(std::vector<uint8_t> &data) {
    encoded_type encoded ;
    if (data.size() % 4) { //According to algorithm stop processing , not possible
        return encoded; //instead of throwing exception can return uninitialized string
    }

    encoded = std::string() ;
    
    std::function<void (std::vector<uint8_t>::iterator, std::vector<uint8_t>::iterator) > func = 
    [&] (std::vector<uint8_t>::iterator b , std::vector<uint8_t>::iterator e) 
    {
       if ( b==e) {
           return;
       }
       uint32_t value = std::accumulate(b,e, 0 , [] (uint32_t v, uint8_t x) { return v * 256 + x ; } )  ;
       uint divisor= boost::math::pow<4>(85);
       while(divisor) {
           //less performance then preallocated string but sufice for now
           char c = VALID_Z85_CHARS [value / divisor % 85] ;
           encoded->push_back(c);
           divisor /=85 ;
       }       
       func(e , e+4 > data.end() ? e : e+4 ) ;
       return; 
    } ;
    func(data.begin(),data.begin()+4) ;

}

// Print out data in a pretty fashion
//
// Print offset into buffer as an 32 bit hexadecimal number, with padding
// Print up to the next sixteen bytes as two digit hexadecimal numbers
// Print ascii (0x20 - 0x7e), or '.' if out of range
// Pad the end with spaces ' '
//
// Example
// 00000000 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
// 00000010 20 21 22 23 24 25 26 00 00 00 00 00 00 00 00 00  !"#$%&.........
// 00000020 00 00 00 00 00 00 00 00 00 00 00 00 00 00 41 00 ..............A.
// 00000030 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
// 00000040 00 00 00 7E                                     ...~
void dumpData(const std::vector<uint8_t> &data) {}

}
#endif//_Z85_ENC_HPP_






