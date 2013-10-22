#ifndef WAVEOPTICS_H_
#define WAVEOPTICS_H_

#include <arrayfire.h>

#include "common.h"

using namespace af;

array WOPropTF(const array& u1, const float L, const float wavelength, const int z);

#endif