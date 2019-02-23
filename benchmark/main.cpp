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
#include "../AoAoAoTT/aoaoaott.hpp"

struct A
{
    double x = 0.;
    double y = 1.;
    double z = 2.;
};

enum class Type { AoSVector, SoAVector, AoSArray, SoAArray };

template<Type t>
auto get_prepared_container();

template<>
auto get_prepared_container<Type::AoSVector>()
{
    return aoaoaott::AoSVector<A>(140000);
}

template<>
auto get_prepared_container<Type::SoAVector>()
{
    return aoaoaott::SoAVector<A>(140000);
}

template<>
auto get_prepared_container<Type::AoSArray>()
{
    return aoaoaott::AoSArray<A, 140000>();
}

template<>
auto get_prepared_container<Type::SoAArray>()
{
    return aoaoaott::SoAArray<A, 140000>();
}

#if 0
template<Type t>
static void BM_Assign_OnlyOneMember(benchmark::State& state)
{
    auto storage = get_prepared_container<t>();
    for (auto _ : state) {
        size_t max = state.range(0);
        for (size_t i = 0; i < max; ++i)
            storage[i]->*(&A::x) = 3.1415926;
    }
}

BENCHMARK_TEMPLATE(BM_Assign_OnlyOneMember, Type::AoSArray)->Range(8, 8<<10);
BENCHMARK_TEMPLATE(BM_Assign_OnlyOneMember, Type::SoAArray)->Range(8, 8<<10);
BENCHMARK_TEMPLATE(BM_Assign_OnlyOneMember, Type::AoSVector)->Range(8, 8<<10);
BENCHMARK_TEMPLATE(BM_Assign_OnlyOneMember, Type::SoAVector)->Range(8, 8<<10);

template<Type t>
static void BM_Assign_Altogether(benchmark::State& state)
{
    auto storage = get_prepared_container<t>();
    for (auto _ : state) {
        size_t max = state.range(0);
        for (size_t i = 0; i < max; ++i)
            storage[i] = A{3.14, 2.78, 1.68};
    }
}

BENCHMARK_TEMPLATE(BM_Assign_Altogether, Type::AoSArray)->Range(8, 8<<10);
BENCHMARK_TEMPLATE(BM_Assign_Altogether, Type::SoAArray)->Range(8, 8<<10);
BENCHMARK_TEMPLATE(BM_Assign_Altogether, Type::AoSVector)->Range(8, 8<<10);
BENCHMARK_TEMPLATE(BM_Assign_Altogether, Type::SoAVector)->Range(8, 8<<10);
#endif

template<Type t>
static void BM_Swap_X_and_Z(benchmark::State& state)
{
    auto storage = get_prepared_container<t>();
    for (auto _ : state) {
        size_t max = state.range(0);
        for (size_t i = 0; i < max; ++i)
            std::swap(storage[i]->*(&A::x), storage[max - i]->*(&A::z));
    }
}

BENCHMARK_TEMPLATE(BM_Swap_X_and_Z, Type::AoSArray)->Range(8, 8<<14);
BENCHMARK_TEMPLATE(BM_Swap_X_and_Z, Type::SoAArray)->Range(8, 8<<14);
BENCHMARK_TEMPLATE(BM_Swap_X_and_Z, Type::AoSVector)->Range(8, 8<<14);
BENCHMARK_TEMPLATE(BM_Swap_X_and_Z, Type::SoAVector)->Range(8, 8<<14);


BENCHMARK_MAIN();