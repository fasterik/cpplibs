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
#include <cstdlib>
#include <cstring>
#include <utility>
#include <type_traits>

template<typename K, typename V>
struct HashTable {
    static_assert(std::is_integral<K>::value || std::is_pointer<K>::value, "Key must be an integer type");
    static_assert(std::is_trivially_copyable<V>::value, "Value must be trivially copyable");

public:
    HashTable() {
        _count = 0;
        _capacity = 8;
        _mask = _capacity - 1;
        _data = allocate(_capacity);
    }

    ~HashTable() {
        free(_data);
    }

    HashTable(const HashTable<K, V> &rhs) = delete;
    HashTable(HashTable<K, V> &&rhs) = delete;

    size_t count() const {
        return _count;
    }

    void clear() {
        memset(_data, 0, _capacity * sizeof(Bucket));
        _count = 0;
    }

    V *get(K key) const {
        Bucket *bucket = find_bucket(_data, _capacity, _mask, key);
        return bucket->present && bucket->key == key ? &bucket->value : nullptr;
    }

    void set(K key, const V &value) {
        V &item = emplace(key);
        item = value;
    }

    V &emplace(K key) {
        maybe_resize();

        Bucket *bucket = find_bucket(_data, _capacity, _mask, key);
        if (!bucket->present) {
            _count++;
            bucket->present = true;
        }

        bucket->key = key;

        return bucket->value;
    }

    void remove(K key) {
        Bucket *deleted = find_bucket(_data, _capacity, _mask, key);
        if (!deleted->present)
            return;

        deleted->present = false;
        _count--;

        Bucket *bucket = deleted;
        Bucket *buckets_end = _data + _capacity;

        while (true) {
            bucket++;
            if (bucket == buckets_end)
                bucket = _data;

            if (!bucket->present)
                break;

            Bucket *natural = &_data[split_mix_64(bucket->key) & _mask];
            if (natural == bucket)
                continue;

            /*
               A key will sometimes land in a different bucket than the natural
               one defined by the key's hash. Since we use linear probing, we
               need to ensure that there are no empty buckets between a key's
               natural bucket and its actual bucket.

               B = bucket
               N = natural
               D = deleted

               There are six possible cases:

               D < B:
               OK   ----D--------N>>>>>>>>B----
               BAD  >>>>D--------B--------N>>>>
               BAD  ----N>>>>>>>>D--------B----

               D > B:
               OK   ----N>>>>>>>>B--------D----
               OK   >>>>B--------D--------N>>>>
               BAD  ----B--------N>>>>>>>>D----

               If D comes after N and before B, accounting for wrapping, then
               we copy B to D and repeat this process until we find an empty
               bucket.
            */

            if (deleted < bucket) {
                if (deleted < natural && natural < bucket)
                    continue;
            } else {
                if (natural < bucket || deleted < natural)
                    continue;
            }

            *deleted = *bucket;
            deleted = bucket;
            deleted->present = false;
        }
    }

private:
    struct Bucket {
        K key;
        V value;
        bool present;
    };

    Bucket *allocate(size_t new_capacity) {
        size_t new_size = new_capacity * sizeof(Bucket);
        Bucket *new_data = (Bucket *)malloc(new_size);
        memset(new_data, 0, new_size);

        return new_data;
    }

    Bucket *find_bucket(Bucket *data, size_t capacity, size_t mask, K key) const {
        Bucket *bucket = &data[split_mix_64(key) & mask];
        Bucket *buckets_end = data + capacity;

        while (bucket->present && bucket->key != key) {
            bucket++;
            if (bucket == buckets_end)
                bucket = data;
        }

        return bucket;
    }

    void maybe_resize() {
        if (_count * 100 / _capacity > 60) {
            size_t new_capacity = _capacity * 2;
            size_t new_mask = new_capacity - 1;
            Bucket *new_data = allocate(new_capacity);

            Bucket *buckets_end = _data + _capacity;
            for (Bucket *bucket = _data; bucket < buckets_end; bucket++) {
                if (!bucket->present)
                    continue;

                Bucket *new_bucket = find_bucket(new_data, new_capacity, new_mask, bucket->key);
                *new_bucket = *bucket;
            }

            free(_data);

            _data = new_data;
            _capacity = new_capacity;
            _mask = new_mask;
        }
    }

    // Adapted from https://prng.di.unimi.it/splitmix64.c
    uint64_t split_mix_64(uint64_t x) const {
        x += 0x9e3779b97f4a7c15;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
        x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
        return x ^ (x >> 31);
    }

    size_t _count;
    size_t _capacity;
    size_t _mask;
    Bucket *_data;
};
