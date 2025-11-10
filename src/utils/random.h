#pragma once
#include <stdint.h>

void random_seed(uint32_t seed);
// Pseudorandom
uint32_t random_u32(void);
float random_float01(void);
float random_float_range(float min, float max);
