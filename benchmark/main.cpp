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
#include <random>

struct A
{
    int x;
    int y;
    int z;
    int w;
};

enum class Type { AoSVector, SoAVector, AoSArray, SoAArray };
static const constexpr size_t CAPACITY = 8ull << 24ull;
template<Type t> auto get_prepared_container();

template<>
auto get_prepared_container<Type::AoSVector>()
{
    static auto ptr = std::make_shared<aoaoaott::AoSVector<A>>(CAPACITY);
    return ptr;
}

template<>
auto get_prepared_container<Type::SoAVector>()
{
    static auto ptr = std::make_shared<aoaoaott::SoAVector<A>>(CAPACITY);
    return ptr;
}

template<>
auto get_prepared_container<Type::AoSArray>()
{
    static auto ptr = std::make_shared<aoaoaott::AoSArray<A, CAPACITY>>();
    return ptr;
}

template<>
auto get_prepared_container<Type::SoAArray>()
{
    static auto ptr = std::make_shared<aoaoaott::SoAArray<A, CAPACITY>>();
    return ptr;
}
    
template<Type t>
static void SwapXandZFromDifferentSides(benchmark::State& state)
{
    auto storage = get_prepared_container<t>();
    auto max = state.range(0);
    for (auto _ : state)
        for (int i = 0; i < max; ++i)
            std::swap((*storage)[i]->*(&A::x), (*storage)[max - i - 1]->*(&A::z));
}

template<Type t>
static void AccessRandomElement(benchmark::State& state)
{
    auto storage = get_prepared_container<t>();
    auto max = state.range(0);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(0, max);

    for (auto _ : state)
        for (int j = 0; j < max; ++j) {
            auto i = dist(mt);
            (*storage)[i]->*(&A::w) += (*storage)[i]->*(&A::x) * ((*storage)[i]->*(&A::y) - (*storage)[i]->*(&A::z));
        }
}

BENCHMARK_TEMPLATE(SwapXandZFromDifferentSides, Type::AoSArray)->Range(8, CAPACITY);
BENCHMARK_TEMPLATE(SwapXandZFromDifferentSides, Type::SoAArray)->Range(8, CAPACITY);

BENCHMARK_TEMPLATE(AccessRandomElement, Type::AoSArray)->Range(8, CAPACITY / 8);
BENCHMARK_TEMPLATE(AccessRandomElement, Type::SoAArray)->Range(8, CAPACITY / 8);

BENCHMARK_MAIN();