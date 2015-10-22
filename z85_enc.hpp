#pragma once
#ifndef _Z85_ENC_H_
#define _Z85_ENC_H_

#include <vector>
#include <string>
#include <cstdint>


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

#define VALID_Z85_CHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#"

// Encode binary data with Z85 encoding
std::vector<uint8_t> decodeZ85(const std::string &encodedString);

// Decode binary data with Z85 encoding
std::string encodeZ85(const std::vector<uint8_t> &data);

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
void dumpData(const std::vector<uint8_t> &data);

#endif//_Z85_ENC_H_
