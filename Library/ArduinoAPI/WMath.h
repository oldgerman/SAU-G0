#ifndef __WMATH_H
#define __WMATH_H

#ifdef __cplusplus

extern "C" {
#endif

#include "stdlib.h"

//random添加后缀_Arduino
void randomSeed_Arduino(unsigned long seed);
long random_Arduino_1(long howbig);
long random_Arduino_2(long howsmall, long howbig);

#ifdef __cplusplus
}
#endif
#endif
