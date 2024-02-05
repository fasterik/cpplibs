/*

MIT License

Copyright (c) 2024 Erik Fast

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// Version 1.0

#pragma once

#include <cstdint>
#include <chrono>
#include <algorithm>

#if defined(_M_X64)
#include <immintrin.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

class Random {
public:
    Random() {
        seed_time();
    }

    Random(uint64_t seed_val) {
        seed(seed_val);
    }

    void seed(uint64_t val) {
        state[0] = val = split_mix_64(val);
        state[1] = val = split_mix_64(val);
        state[2] = val = split_mix_64(val);
        state[3] = val = split_mix_64(val);
    }

    void seed_time() {
        seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    // Adapted from https://prng.di.unimi.it/xoshiro256plusplus.c
    uint64_t get_u64() {
        uint64_t result = rotl(state[0] + state[3], 23) + state[0];
        uint64_t t = state[1] << 17;

        state[2] ^= state[0];
        state[3] ^= state[1];
        state[1] ^= state[2];
        state[0] ^= state[3];
        state[2] ^= t;
        state[3] = rotl(state[3], 45);

        return result;
    }

    // Unbiased random numbers in a range.
    // Adapted from https://www.pcg-random.org/posts/bounded-rands.html
    uint64_t get_range(uint64_t range) {
#if defined(_M_X64) || defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
        // Do the bitmask rejection method if the lzcnt/bsr intrinsic is
        // available.

        uint64_t mask = ~0ull;
        range--;

        unsigned long index;

#if defined(_M_X64)
        index = _lzcnt_u64(range | 1);
#elif defined(__clang__) || defined(__GNUC__)
        index = __builtin_clzll(range | 1);
#else
        _BitScanReverse64(&index, range | 1);
        index = 63 - index;
#endif

        mask >>= index;

        uint64_t x;
        do {
            x = get_u64() & mask;
        } while (x > range);

        return x;
#else
        // This is slower than the bitmask rejection method, but still unbiased
        // and portable.
        uint64_t x, r;
        do {
            x = get_u64();
            r = x % range;
        } while (x - r > (-range));

        return r;
#endif
    }

    int get_int(int min_val, int max_val) {
        return min_val + (int)get_range(max_val - min_val + 1);
    }

    float get_float_01() {
        return 0x1p-24f * (get_u64() >> 40);
    }

    double get_double_01() {
        return 0x1p-53 * (get_u64() >> 11);
    }

    float get_float(float min_val, float max_val) {
        return min_val + (max_val - min_val) * get_float_01();
    }

    double get_double(double min_val, double max_val) {
        return min_val + (max_val - min_val) * get_double_01();
    }

    template<typename T>
    T& choice(T *array, size_t size) {
        return array[get_range(size)];
    }

    template<typename T>
    void shuffle(T *array, size_t size) {
        for (size_t i = 0; i < size - 1; i++)
            std::swap(array[i], array[i + get_range(size - i)]);
    }

private:
    // Adapted from https://prng.di.unimi.it/splitmix64.c
    uint64_t split_mix_64(uint64_t x) {
        x += 0x9e3779b97f4a7c15;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
        x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
        return x ^ (x >> 31);
    }

    uint64_t rotl(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t state[4];
};
