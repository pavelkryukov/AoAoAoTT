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

#include "../aoaoaott.hpp"
#include "catch.hpp"

#include <cstring>

struct A {
    int val;
    int key;
    int dum;
};

static_assert(std::is_same_v<struct_reader::as_type_list<A>, struct_reader::type_list<int, int, int>>);

struct EmptyStruct {};

static_assert(std::is_same_v<struct_reader::as_type_list<EmptyStruct>, struct_reader::type_list<>>);

struct WithArray {
    int size;
    char array[1024];
};

struct BwithA {
    A x;
    int val;
    int key;
    int dum;
};

TEST_CASE("Empty structures")
{
    AoS<EmptyStruct>();
    SoA<EmptyStruct>();
}

TEST_CASE("AoS: initialize and r/w")
{
    AoS<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;
    storage[4]->*(&A::key) = 9;
    storage[4]->*(&A::val) = 6;
    
    CHECK( storage[3]->*(&A::key) == 10 );
    CHECK( storage[3]->*(&A::val) == 3 );
    CHECK( storage[4]->*(&A::key) == 9 );
    CHECK( storage[4]->*(&A::val) == 6 );
}

TEST_CASE("SoA: initialize and r/w")
{
    SoA<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;
    storage[4]->*(&A::key) = 9;
    storage[4]->*(&A::val) = 6;
 
    CHECK( storage[3]->*(&A::key) == 10 );
    CHECK( storage[3]->*(&A::val) == 3 );
    CHECK( storage[4]->*(&A::key) == 9 );
    CHECK( storage[4]->*(&A::val) == 6 );
}

TEST_CASE("AoS: structure with array")
{
    AoS<WithArray> storage(10);
    std::memset(&(storage[3]->*(&WithArray::array)), 0x11, 1024);
    CHECK( (storage[3]->*(&WithArray::array))[300] == 0x11);
}

TEST_CASE("AoS: get() interface")
{
    AoS<A> storage(10);
    storage[2].get<&A::val>() = 234;
    CHECK( storage[2].get<&A::val>() == 234 );
}

TEST_CASE("SoA: get() interface")
{
    SoA<A> storage(10);
    storage[2].get<&A::val>() = 234;
    CHECK( storage[2].get<&A::val>() == 234 );
}

TEST_CASE("SoA: structure with array")
{
    SoA<WithArray> storage(10);
    std::memset(&(storage[3]->*(&WithArray::array)), 0x11, 1024);
    CHECK( (storage[3]->*(&WithArray::array))[300] == 0x11);
}

TEST_CASE("AoS: assign structure")
{
    AoS<A> storage( 10);
    const A x{3, 7, 11};
    storage[2] = x; // copy assignment
    storage[3] = A{10, 3, 8}; // move assignment

    CHECK( storage[2]->*(&A::val) == 3 );
    CHECK( storage[2]->*(&A::key) == 7 );
    CHECK( storage[2]->*(&A::dum) == 11 );
    CHECK( storage[3]->*(&A::val) == 10 );
    CHECK( storage[3]->*(&A::key) == 3 );
    CHECK( storage[3]->*(&A::dum) == 8 );
}

TEST_CASE("SoA: assign structure")
{
    SoA<A> storage( 10);
    const A x{3, 7, 11};
    storage[2] = x; // copy assignment
    storage[3] = A{10, 3, 8}; // move assignment

    CHECK( storage[2]->*(&A::val) == 3 );
    CHECK( storage[2]->*(&A::key) == 7 );
    CHECK( storage[2]->*(&A::dum) == 11 );
    CHECK( storage[3]->*(&A::val) == 10 );
    CHECK( storage[3]->*(&A::key) == 3 );
    CHECK( storage[3]->*(&A::dum) == 8 );
}

TEST_CASE("AoS: constant functions")
{
    AoS<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;

    const auto& const_ref = storage;
    CHECK( const_ref[3]->*(&A::key) == 10 );
    CHECK( const_ref[3]->*(&A::val) == 3 );
    CHECK( const_ref[3].get<&A::val>() == 3 );
}

TEST_CASE("SoA: constant functions")
{
    SoA<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;

    const auto& const_ref = storage;
    CHECK( const_ref[3]->*(&A::key) == 10 );
    CHECK( const_ref[3]->*(&A::val) == 3 );
    CHECK( const_ref[3].get<&A::val>() == 3 );
}

struct DefaultInitializer
{
    int x = 9;
    int y = 0;
};

static_assert(!std::is_trivially_constructible_v<DefaultInitializer>);

TEST_CASE("AoS: default initialization")
{
    AoS<DefaultInitializer> storage( 10);
    CHECK( storage[2]->*(&DefaultInitializer::x) == 9 );
    CHECK( storage[3]->*(&DefaultInitializer::x) == 9 );
}

TEST_CASE("SoA: default initialization")
{
    SoA<DefaultInitializer> storage( 10);
    CHECK( storage[2]->*(&DefaultInitializer::x) == 9 );
    CHECK( storage[3]->*(&DefaultInitializer::x) == 9 );
}
