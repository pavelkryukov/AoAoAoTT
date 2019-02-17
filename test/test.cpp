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

#ifndef CONTAINER
#error "CONTAINER macro must be defined"
#endif

#define STRINGIZE(s) STRINGIZE_H(s)
#define STRINGIZE_H(s) #s
#define CONTAINER_STR STRINGIZE(CONTAINER)

using namespace ao_ao_ao_tt;

struct EmptyStruct {};

TEST_CASE(CONTAINER_STR "Empty structures")
{
    CONTAINER<EmptyStruct>();
}

struct A {
    int val;
    int key;
    int dum;
};

namespace ao_ao_ao_tt::loophole_ns
{
    static_assert(std::is_same_v<as_type_list<A>, type_list<int, int, int>>);
    static_assert(std::is_same_v<as_type_list<EmptyStruct>, type_list<>>);
}

TEST_CASE(CONTAINER_STR " initialize and r/w")
{
    CONTAINER<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;
    storage[4]->*(&A::key) = 9;
    storage[4]->*(&A::val) = 6;
    
    CHECK( storage[3]->*(&A::key) == 10 );
    CHECK( storage[3]->*(&A::val) == 3 );
    CHECK( storage[4]->*(&A::key) == 9 );
    CHECK( storage[4]->*(&A::val) == 6 );
}

TEST_CASE(CONTAINER_STR " get() interface")
{
    CONTAINER<A> storage(10);
    storage[2].get<&A::val>() = 234;
    CHECK( storage[2].get<&A::val>() == 234 );
}

TEST_CASE(CONTAINER_STR " assign structure")
{
    CONTAINER<A> storage( 10);
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

TEST_CASE(CONTAINER_STR " constant functions")
{
    CONTAINER<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;

    const auto& const_ref = storage;
    CHECK( const_ref[3]->*(&A::key) == 10 );
    CHECK( const_ref[3]->*(&A::val) == 3 );
    CHECK( const_ref[3].get<&A::val>() == 3 );
}

struct WithArray {
    int size;
    std::array<char, 16> array;
};

static_assert(sizeof(WithArray) == sizeof(int) + 16);

TEST_CASE(CONTAINER_STR " structure with array")
{
    CONTAINER<WithArray> storage(10);
    std::memset(&(storage[3]->*(&WithArray::array)), 0x11, 16);
    CHECK( (storage[3]->*(&WithArray::array))[8] == 0x11);
}

TEST_CASE(CONTAINER_STR " assign array")
{
    CONTAINER<WithArray> storage( 10);
    WithArray hw{ 16, "Hello World!!!!"};
    storage[6] = hw;

    CHECK( std::strcmp((storage[6]->*(&WithArray::array)).data(), "Hello World!!!!") == 0 );
}

struct DefaultInitializer
{
    int x = 9;
    int y = 0;
};

static_assert(!std::is_trivially_constructible_v<DefaultInitializer>);

TEST_CASE(CONTAINER_STR " default initialization")
{
    CONTAINER<DefaultInitializer> storage( 10);
    CHECK( storage[2]->*(&DefaultInitializer::x) == 9 );
    CHECK( storage[3]->*(&DefaultInitializer::x) == 9 );
}

TEST_CASE(CONTAINER_STR " initialization via copies")
{
    DefaultInitializer example{ 234, 123};
    CONTAINER<DefaultInitializer> storage( 10, example);
    CHECK( storage[2]->*(&DefaultInitializer::x) == 234 );
    CHECK( storage[2]->*(&DefaultInitializer::y) == 123 );
    CHECK( storage[3]->*(&DefaultInitializer::x) == 234 );
}

TEST_CASE(CONTAINER_STR " resize by example")
{
    DefaultInitializer example{ 234, 123};
    CONTAINER<DefaultInitializer> storage( 10);
    storage.resize(20, example);
    CHECK( storage[5]->*(&DefaultInitializer::x) == 9 );
    CHECK( storage[15]->*(&DefaultInitializer::x) == 234 );
}

struct ConstantMember
{
    const int x = 9;
    int y = 0;
};

TEST_CASE(CONTAINER_STR " structure with constant member")
{
    CONTAINER<ConstantMember> storage( 10);
    storage.resize(20);
    CHECK( storage[5]->*(&ConstantMember::x) == 9 );
    CHECK( storage[15]->*(&ConstantMember::x) == 9 );
}

struct WithA {
    A a;
    int val;
    int key;
    int dum;
};

TEST_CASE(CONTAINER_STR " allow substructure")
{
    CONTAINER<WithA> storage( 10);
    const A x{3, 7, 11};
    const WithA y{x, 4, 8, 12};
    storage[5] = y;

    CHECK( (storage[5]->*(&WithA::a)).dum == 11 );
    CHECK( storage[5]->*(&WithA::dum) == 12 );
}

struct HasMethod
{
    int alain;
    int delon;
    void drink_double_bourbon()
    {
        std::swap(alain, delon);
    }
};

TEST_CASE(CONTAINER_STR " aggregate and run method")
{
    CONTAINER<HasMethod> storage( 10);
    storage[4] = HasMethod{33, 44};
    auto val = storage[4].aggregate_object();
    val.drink_double_bourbon();

    CHECK( val.alain == 44 );
    CHECK( val.delon == 33 );
    CHECK( storage[4]->*(&HasMethod::alain) == 33 );
    CHECK( storage[4]->*(&HasMethod::delon) == 44 );
}

TEST_CASE(CONTAINER_STR " iterator")
{
    
}
