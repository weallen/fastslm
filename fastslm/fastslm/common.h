#ifndef COMMON_H_
#define COMMON_H_


#include <stdint.h>
#include <arrayfire.h>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#define WIN32_LEAN_AND_MEAN

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define USE_SHIFT 

#define fftshift(in)  shift(in, in.dims(0)/2, in.dims(1)/2)
#define ifftshift(in) shift(in, (in.dims(0)+1)/2, (in.dims(1)+1)/2)

// split string by whitespace and add to vector
void Tokenize(const std::string& currstr, std::vector<std::string>& tokens);

af::array phase(const af::array& h);
af::array amplitude(const af::array& h);
af::array cexp(const af::array& h);
af::array linspace(float d1, float d2, int n);

#endif