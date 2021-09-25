/*
 * Copyright (c) 2019-2021 Pavel I. Kryukov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <benchmark/benchmark.h>
#include "../aoaoaott.hpp"

#include <iostream>
#include <memory>
#include <new>

#define KB * 1024
#define MB KB KB

#define ASSERT_SIZE(X) static_assert(sizeof(A ## X) == X)

struct A12
{
    int32_t x, y, z;
    auto sum() const { return x + y + z; }
};

struct __attribute__ ((packed)) A13
{
    int32_t x, y, z;
    int8_t w;
    auto sum() const { return x + y + z + w; }
};

struct A16
{
    int32_t x, y, z, w;
    auto sum() const { return x + y + z + w; }
};

struct A32
{
    A16 a;
    int32_t x, y, z, w;
    auto sum() const { return x + y + z + w + a.sum(); }
};

struct A48
{
    A32 a;
    int32_t x, y, z, w;
    auto sum() const { return x + y + z + w + a.sum(); }
};

struct A64
{
    A48 a;
    int32_t x, y, z, w;
    auto sum() const { return x + y + z + w + a.sum(); }
};

struct A60
{
    A48 a;
    int32_t x, y, z;
    auto sum() const { return x + y + z + a.sum(); }
};

struct A68
{
    A48 a;
    int32_t x, y, z, w, u;
    auto sum() const { return x + y + z + w + u + a.sum(); }
};

struct A96
{
    A64 a;
    A16 b;
    int32_t x, y, z, w;
    auto sum() const { return x + y + z + w + a.sum() + b.sum(); }
};

struct A128
{
    A64 a;
    A48 b;
    int32_t x, y, z, w;
    auto sum() const { return x + y + z + w + a.sum() + b.sum(); }
};

ASSERT_SIZE(12);
ASSERT_SIZE(13);
ASSERT_SIZE(16);
ASSERT_SIZE(32);
ASSERT_SIZE(48);
ASSERT_SIZE(60);
ASSERT_SIZE(64);
ASSERT_SIZE(68);
ASSERT_SIZE(96);
ASSERT_SIZE(128);

template<template<typename, size_t> typename Container, typename A>
auto get_prepared_container()
{
    const constexpr size_t CAPACITY = (1ull << 23ull) / sizeof(A);
    std::unique_ptr<Container<A, CAPACITY>> ptr;
    ptr.reset(new (std::align_val_t(4 KB)) Container<A, CAPACITY>());

    assert(sizeof(*ptr) == CAPACITY * sizeof(A));
    return ptr;
}

template<template<typename, size_t> typename Container, typename A>
__attribute__((optimize("no-tree-vectorize")))
static void Bytes12(benchmark::State& state)
{
    auto storage = get_prepared_container<Container, A>();
    const auto iterations = state.range(0) / sizeof(A);
    for (size_t i = 0; i < iterations; ++i)
        (*storage)[i] = A();

    assert(iterations <= storage->size());
    for (auto _ : state) {
        for (size_t i = 0; i < iterations; ++i) {
	    (*storage)[i]->*(&A::x) = (*storage)[i]->*(&A::y) << (*storage)[i]->*(&A::z);
	}
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * iterations * sizeof(int32_t) * 3);
}

template<template<typename, size_t> typename Container, typename A>
__attribute__((optimize("no-tree-vectorize")))
static void AllBytes(benchmark::State& state)
{
    auto storage = get_prepared_container<Container, A>();
    const auto iterations = state.range(0) / sizeof(A) / state.range(1);
    for (size_t i = 0; i < state.range() / sizeof(A); ++i)
        (*storage)[i] = A();

    assert(iterations <= storage->size());
    for (auto _ : state) {
         for (size_t i = 0, j = 0; i < iterations; ++i, j += state.range(1)) {
              (*storage)[j]->*(&A::x) = (*storage)[j].aggregate().sum();
         }
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * iterations * sizeof(A));
}

template<typename T, size_t N>
using SoA = aoaoaott::SoAArray<T, N>;

template<typename T, size_t N>
using AoS = aoaoaott::AoSArray<T, N>;

BENCHMARK_TEMPLATE(Bytes12, SoA, A12)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
//BENCHMARK_TEMPLATE(Bytes12, SoA, A13)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A16)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A32)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A48)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A60)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A64)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A68)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A96)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, SoA, A128)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);

BENCHMARK_TEMPLATE(Bytes12, AoS, A12)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A13)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A16)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A32)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A48)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A60)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A64)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A68)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A96)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);
BENCHMARK_TEMPLATE(Bytes12, AoS, A128)->Arg(16 KB)->Arg(64 KB)->Arg(1 MB)->Arg(4 MB);

BENCHMARK_TEMPLATE(AllBytes, SoA, A12)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
//BENCHMARK_TEMPLATE(AllBytes, SoA, A13)->Args({16 KB,1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A16)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A32)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A48)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A60)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A64)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A68)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A96)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, SoA, A128)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});

BENCHMARK_TEMPLATE(AllBytes, AoS, A12)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A13)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A16)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A32)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A48)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A60)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A64)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A68)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A96)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});
BENCHMARK_TEMPLATE(AllBytes, AoS, A128)->Args({16 KB, 1})->Args({64 KB, 1})->Args({1 MB, 1})->Args({4 MB, 1})->Args({1 MB, 16});

BENCHMARK_MAIN();

