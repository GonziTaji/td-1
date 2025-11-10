#include "random.h"
#include <stdint.h>

static uint32_t rng_state = 1;

void random_seed(uint32_t seed) {
    // Seed cannot be 0 for xorshift32
    rng_state = (seed == 0) ? 1 : seed;
}

// xorshift32
uint32_t random_u32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

// Convertimos a [0,1) usando multiplicación por 1/2^32
float random_float01(void) {
    return (random_u32() * (1.0f / 4294967296.0f));
}

float random_float_range(float min, float max) {
    return min + random_float01() * (max - min);
}
