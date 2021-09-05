/*
 * Copyright (c) 2019 Pavel I. Kryukov
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

#include <memory>

struct A12
{
    int32_t x, y, z;
};

struct A16
{
    int32_t x, y, z, w;
};

struct A32
{
    int32_t x, y, z, w, a, b, c, d;
};

struct A48
{
    int32_t x, y, z, w, a, b, c, d, e, f, g, h;
};

static const constexpr size_t CAPACITY = 1ull << 20ull;
static const constexpr size_t MAX_STRIDE = 1024;
static const constexpr size_t ITERATIONS = CAPACITY / MAX_STRIDE;

template<template<typename, size_t> typename Container, typename A>
auto get_prepared_container()
{
    static auto ptr = std::make_shared<Container<A, CAPACITY>>();
    return ptr;
}

template<template<typename, size_t> typename Container, typename A>
static void StridedAccess(benchmark::State& state)
{
    auto storage = get_prepared_container<Container, A>();
    size_t stride = state.range(0);
    for (auto _ : state)
        for (size_t j = 0, i = 0; j < ITERATIONS; ++j, i += stride)
            (*storage)[i]->*(&A::x) += (*storage)[i]->*(&A::y) << (*storage)[i]->*(&A::z);

    state.SetBytesProcessed(int64_t(state.iterations()) * ITERATIONS * sizeof(int32_t) * 3);
}

BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::AoSArray, A12)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::AoSArray, A16)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::AoSArray, A32)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::AoSArray, A48)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::SoAArray, A12)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::SoAArray, A16)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::SoAArray, A32)->RangeMultiplier(2)->Range(1, MAX_STRIDE);
BENCHMARK_TEMPLATE(StridedAccess, aoaoaott::SoAArray, A48)->RangeMultiplier(2)->Range(1, MAX_STRIDE);

BENCHMARK_MAIN();

