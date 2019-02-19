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
#define TEST_CONTAINER_CASE(x) TEST_CASE(STRINGIZE(CONTAINER) ": " x)

using namespace ao_ao_ao_tt;

TEST_CONTAINER_CASE("Empty structures")
{
    struct EmptyStruct {};
    CONTAINER<EmptyStruct>();
}

template<typename R>
const char* bold_cast(const R& ref)
{
    return reinterpret_cast<const char*>(&ref);
}

TEST_CONTAINER_CASE("principal test")
{
    struct Structure
    {
        int val;
        int key;
        int dum;
    };
    static_assert(sizeof(Structure) != sizeof(int));

    CONTAINER<Structure> storage( 11);
    ptrdiff_t distance = bold_cast(storage[10]->*(&Structure::key)) - bold_cast(storage[0]->*(&Structure::key));

    // That is the only test dependent on container type
    if constexpr (std::is_same_v<CONTAINER<Structure>, AoS<Structure>>)
        CHECK( distance == 10 * sizeof(Structure) );
    else    
        CHECK( distance == 10 * sizeof(int) );
}

struct A {
    int val;
    int key;
    int dum;
};

TEST_CONTAINER_CASE("initialize and r/w")
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

TEST_CONTAINER_CASE("get() interface")
{
    CONTAINER<A> storage(10);
    storage[2].get<&A::val>() = 234;
    CHECK( storage[2].get<&A::val>() == 234 );
}

TEST_CONTAINER_CASE("assign structure")
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

TEST_CONTAINER_CASE("constant functions")
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

TEST_CONTAINER_CASE("structure with array")
{
    CONTAINER<WithArray> storage(10);
    std::memset(&(storage[3]->*(&WithArray::array)), 0x11, 16);
    CHECK( (storage[3]->*(&WithArray::array))[8] == 0x11);
}

TEST_CONTAINER_CASE("assign array")
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

TEST_CONTAINER_CASE("default initialization")
{
    CONTAINER<DefaultInitializer> storage( 10);
    CHECK( storage[2]->*(&DefaultInitializer::x) == 9 );
    CHECK( storage[3]->*(&DefaultInitializer::x) == 9 );
}

TEST_CONTAINER_CASE("initialization via copies")
{
    DefaultInitializer example{ 234, 123};
    CONTAINER<DefaultInitializer> storage( 10, example);
    CHECK( storage[2]->*(&DefaultInitializer::x) == 234 );
    CHECK( storage[2]->*(&DefaultInitializer::y) == 123 );
    CHECK( storage[3]->*(&DefaultInitializer::x) == 234 );
}

TEST_CONTAINER_CASE("resize by example")
{
    DefaultInitializer example{ 234, 123};
    CONTAINER<DefaultInitializer> storage( 10);
    storage.resize(20, example);
    CHECK( storage[5]->*(&DefaultInitializer::x) == 9 );
    CHECK( storage[15]->*(&DefaultInitializer::x) == 234 );
}

TEST_CONTAINER_CASE("structure with constant member")
{
    struct ConstantMember
    {
        const int x = 9;
        int y = 0;
    };
    CONTAINER<ConstantMember> storage( 10);
    storage.resize(20);
    CHECK( storage[5]->*(&ConstantMember::x) == 9 );
    CHECK( storage[15]->*(&ConstantMember::x) == 9 );
}

TEST_CONTAINER_CASE("allow substructure")
{
    struct WithA {
        A a;
        int val;
        int key;
        int dum;
    };
    CONTAINER<WithA> storage( 10);
    const A x{3, 7, 11};
    const WithA y{x, 4, 8, 12};
    storage[5] = y;

    CHECK( (storage[5]->*(&WithA::a)).dum == 11 );
    CHECK( storage[5]->*(&WithA::dum) == 12 );
}

TEST_CONTAINER_CASE("aggregate and run method")
{
    struct HasMethod
    {
        int alain;
        int delon;
        void drink_double_bourbon()
        {
            std::swap(alain, delon);
        }
    };
    CONTAINER<HasMethod> storage( 10);
    storage[4] = HasMethod{33, 44};
    auto val = storage[4].aggregate();
    val.drink_double_bourbon();

    CHECK( val.alain == 44 );
    CHECK( val.delon == 33 );
    CHECK( storage[4]->*(&HasMethod::alain) == 33 );
    CHECK( storage[4]->*(&HasMethod::delon) == 44 );
}

TEST_CONTAINER_CASE("const iterator")
{
    CONTAINER<A> storage(10, { 11, 12, 13});
    size_t i = 0;
    for (const auto& entry : storage) {
        CHECK( entry->*(&A::val) == 11);
        CHECK( entry->*(&A::key) == 12);
        CHECK( entry->*(&A::dum) == 13);
        ++i;
    }
    CHECK(i == 10);
}

TEST_CONTAINER_CASE("mutable iterator")
{
    CONTAINER<A> storage(10);
    for (auto& entry : storage) {
        entry->*(&A::val) = 21;
        entry->*(&A::key) = 22;
        entry->*(&A::dum) = 23;
    }
    for (const auto& entry : storage) {
        CHECK( entry->*(&A::val) == 21);
        CHECK( entry->*(&A::key) == 22);
        CHECK( entry->*(&A::dum) == 23);
    }
}

TEST_CONTAINER_CASE("iterator arrow operator")
{
    CONTAINER<A> storage(10, { 11, 12, 13});
    CHECK( storage.cbegin()->get<&A::val>() == 11 );
    CHECK( storage.begin()->get<&A::key>()  == 12 );
}

TEST_CONTAINER_CASE("bidirectional iterator")
{
    CONTAINER<A> storage(10, { 11, 12, 13});
    size_t i = 0;
    auto it = storage.cend();
    do {
        --it;
        CHECK( *it->*(&A::val) == 11);
        CHECK( *it->*(&A::key) == 12);
        CHECK( *it->*(&A::dum) == 13);
        ++i;
    } while (it != storage.cbegin());
    CHECK(i == 10);
}

TEST_CONTAINER_CASE("random access iterator")
{
    CONTAINER<A> storage(90);
    CHECK( storage.end() - storage.begin() == 90 );
    CHECK( storage.begin() <= storage.end() );
    auto it = std::next(storage.begin(), 60);
    CHECK( storage.end() - it == 30 );
}

#if 0
TEST_CONTAINER_CASE("reverse iterator")
{
    CONTAINER<A> storage(10);
    int value = 20;
    for (auto& entry : storage) {
        entry->*(&A::val) = ++value;
        entry->*(&A::key) = ++value;
        entry->*(&A::dum) = ++value;
    }
    size_t i = 0;
    CHECK( &(*std::make_reverse_iterator(storage.end())) == &*(storage.end() - 1) );
    for (auto it = storage.crbegin(); it != storage.crend(); ++it) {
        CHECK( it->get<&A::val>() == value - 3 * i - 2 );
        CHECK( it->get<&A::key>() == value - 3 * i - 1 );
        CHECK( it->get<&A::dum>() == value - 3 * i );
        ++i;
    }
}
#endif