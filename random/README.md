An easy and fast non-cryptographic random number library in C++.

The default constructor seeds the PRNG using `chrono::high_resolution_clock`.

```c++
Random random;
```

Or you can pass in an unsigned 64-bit seed.

```c++
Random random(0xdeadbeefcafef00d);
```

Under the hood these call the `seed_time()` and `seed()` methods. You can use
these to re-seed the PRNG.

```c++
random.seed_time();
random.seed(0xdeadbeefcafef00d);
```

Generate a random integer:

```c++
uint64_t x = random.get_u64();
```

Generate a random integer `x < 100`:

```c++
uint64_t x = random.get_range(100);
```

Generate a random integer `-10 <= x <= 10`:

```c++
int x = random.get_int(-10, 10);
```

Generate a random float or double `0.0 <= x < 1.0`:

```c++
float x = random.get_float_01();
double y = random.get_double_01();
```

Generate a random float or double `-5.0 <= x < 5.0`:

```c++
float x = random.get_float(-5.0f, 5.0f);
double y = random.get_double(-5.0, 5.0);
```

Get a random element from an array:

```c++
int a[100];
for (int i = 0; i < 100; i++)
    a[i] = i;

int x = random.choice(a, 100);
```

Shuffle an array:

```c++
random.shuffle(a, 100);
```

The library uses the `xoroshiro256++` algorithm, which has nice properties for
most non-cryptographic applications. It passes standard statistical tests and has a
period of `2^256 - 1`. For more information, see
[this page](https://prng.di.unimi.it/).

The benchmark is a program playing random games of tic-tac-toe. Compared to the
the 64-bit generators in the standard `<random>` library, ours is 2.5x to 2.7x
faster (clang 14.0.6, i5-6500):

```
xoroshiro256++: 12.5M games/s
mt19937_64: 5.1M games/s
ranlux48_base: 4.7M games/s
```
