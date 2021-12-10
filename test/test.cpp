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

#include "../aoaoaott.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CONTAINER

#ifndef CONTAINER
#error "CONTAINER macro must be defined"
#endif

#include <boost/test/included/unit_test.hpp>
#include <cstring>

#define PASTER(x,y) x ## y
#define EVALUATOR(x,y) PASTER(x,y)
#define VECTOR_CONTAINER EVALUATOR(CONTAINER, Vector)
#define ARRAY_CONTAINER EVALUATOR(CONTAINER, Array)

using namespace aoaoaott;

template<typename R>
constexpr const char* bold_cast(const R& ref)
{
    return reinterpret_cast<const char*>(&ref);
}

BOOST_AUTO_TEST_CASE(principal_test)
{
    struct Structure
    {
        int val;
        int key;
        int dum;
    };
    static_assert(sizeof(Structure) != sizeof(int));

    ARRAY_CONTAINER<Structure, 20> storage;
    ptrdiff_t distance = bold_cast(storage[10]->*(&Structure::key)) - bold_cast(storage[0]->*(&Structure::key));

    // That is the only test dependent on container type
    if constexpr (std::is_same_v<ARRAY_CONTAINER<Structure, 20>, AoSArray<Structure, 20>>)
        BOOST_TEST( distance == 10 * sizeof(Structure) );
    else
        BOOST_TEST( distance == 10 * sizeof(int) );
}

struct A {
    int val;
    int key;
    int dum;
};

/*
static const constexpr ARRAY_CONTAINER<A, 11> array = {};
static const constexpr size_t distance = bold_cast(array[10].get<&A::key>()) - bold_cast(array[0].get<&A::key>());
static_assert(std::is_same_v<decltype(array), AoSArray<A, 11>>
                ? distance == 10 * sizeof(A)
                : distance == 10 * sizeof(int));
*/

BOOST_AUTO_TEST_CASE(initialize_and_rw)
{
    VECTOR_CONTAINER<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;
    storage[4]->*(&A::key) = 9;
    storage[4]->*(&A::val) = 6;

    BOOST_TEST( (storage[3]->*(&A::key)) == 10 );
    BOOST_TEST( (storage[3]->*(&A::val)) == 3 );
    BOOST_TEST( (storage[4]->*(&A::key)) == 9 );
    BOOST_TEST( (storage[4]->*(&A::val)) == 6 );
};

BOOST_AUTO_TEST_CASE(get__interface)
{
    VECTOR_CONTAINER<A> storage(10);
    storage[2].get<&A::val>() = 234;
    BOOST_TEST( storage[2].get<&A::val>() == 234 );
};

BOOST_AUTO_TEST_CASE(assign_structure)
{
    VECTOR_CONTAINER<A> storage( 10);
    const A x{3, 7, 11};
    storage[2] = x; // copy assignment
    storage[3] = A{10, 3, 8}; // move assignment

    BOOST_TEST( (storage[2]->*(&A::val)) == 3 );
    BOOST_TEST( (storage[2]->*(&A::key)) == 7 );
    BOOST_TEST( (storage[2]->*(&A::dum)) == 11 );
    BOOST_TEST( (storage[3]->*(&A::val)) == 10 );
    BOOST_TEST( (storage[3]->*(&A::key)) == 3 );
    BOOST_TEST( (storage[3]->*(&A::dum)) == 8 );
}

BOOST_AUTO_TEST_CASE(constant_functions)
{
    VECTOR_CONTAINER<A> storage( 10);
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;

    const auto& const_ref = storage;
    BOOST_TEST( (const_ref[3]->*(&A::key)) == 10 );
    BOOST_TEST( (const_ref[3]->*(&A::val)) == 3 );
    BOOST_TEST( (const_ref[3].get<&A::val>()) == 3 );
}

struct WithArray {
    int size;
    std::array<char, 16> array;
};

static_assert(sizeof(WithArray) == sizeof(int) + 16);

BOOST_AUTO_TEST_CASE(structure_with_array)
{
    VECTOR_CONTAINER<WithArray> storage(10);
    std::memset(&(storage[3]->*(&WithArray::array)), 0x11, 16);
    BOOST_TEST( ((storage[3]->*(&WithArray::array))[8]) == 0x11);
}

BOOST_AUTO_TEST_CASE(assign_array)
{
    VECTOR_CONTAINER<WithArray> storage( 10);
    WithArray hw{ 16, {"Hello World!!!!"}};
    storage[6] = hw;

    BOOST_TEST( std::strcmp((storage[6]->*(&WithArray::array)).data(), "Hello World!!!!") == 0 );
}

struct DefaultInitializer
{
    int x = 9;
    float y = 0;
};

static_assert(!std::is_trivially_constructible_v<DefaultInitializer>);

BOOST_AUTO_TEST_CASE(default_initialization)
{
    VECTOR_CONTAINER<DefaultInitializer> storage( 10);
    BOOST_TEST( (storage[2]->*(&DefaultInitializer::x)) == 9 );
    BOOST_TEST( (storage[3]->*(&DefaultInitializer::x)) == 9 );
}

BOOST_AUTO_TEST_CASE(initialization_via_copies)
{
    DefaultInitializer example{ 234, 123};
    VECTOR_CONTAINER<DefaultInitializer> storage( 10, example);
    BOOST_TEST( (storage[2]->*(&DefaultInitializer::x)) == 234 );
    BOOST_TEST( (storage[2]->*(&DefaultInitializer::y)) == 123 );
    BOOST_TEST( (storage[3]->*(&DefaultInitializer::x)) == 234 );
}

BOOST_AUTO_TEST_CASE(resize_by_example)
{
    DefaultInitializer example{ 234, 123};
    VECTOR_CONTAINER<DefaultInitializer> storage(10);
    storage.resize(20, example);
    BOOST_TEST( (storage[5]->*(&DefaultInitializer::x)) == 9 );
    BOOST_TEST( (storage[15]->*(&DefaultInitializer::x)) == 234 );
}

BOOST_AUTO_TEST_CASE(structure_with_constant_member)
{
    struct ConstantMember
    {
        const int x = 9;
        int y = 0;
    };
    VECTOR_CONTAINER<ConstantMember> storage;
    storage.resize(20);
    BOOST_TEST( (storage[5]->*(&ConstantMember::x)) == 9 );
    BOOST_TEST( (storage[15]->*(&ConstantMember::x)) == 9 );
}

BOOST_AUTO_TEST_CASE(allow_substructure)
{
    struct WithA {
        A a;
        int val;
        int key;
        int dum;
    };
    VECTOR_CONTAINER<WithA> storage( 10);
    const A x{3, 7, 11};
    const WithA y{x, 4, 8, 12};
    storage[5] = y;

    BOOST_TEST(((storage[5]->*(&WithA::a)).dum) == 11 );
    BOOST_TEST((storage[5]->*(&WithA::dum)) == 12 );
}

struct HasMethod
{
    int alain;
    int delon;

    int drink_cologne(int cologne) const
    {
        return alain + delon + 3 * cologne;
    }

    void drink_double_bourbon()
    {
        std::swap(alain, delon);
    }
};

BOOST_AUTO_TEST_CASE(aggregate_and_run_method)
{
    VECTOR_CONTAINER<HasMethod> storage( 10);
    storage[4] = HasMethod{33, 44};
    auto val = storage[4].aggregate();
    val.drink_double_bourbon();

    BOOST_TEST( val.alain == 44 );
    BOOST_TEST( val.delon == 33 );
    BOOST_TEST( (storage[4]->*(&HasMethod::alain)) == 33 );
    BOOST_TEST( (storage[4]->*(&HasMethod::delon)) == 44 );
}

BOOST_AUTO_TEST_CASE(implicit_aggregate)
{
    VECTOR_CONTAINER<HasMethod> storage( 10);
    storage[4] = HasMethod{33, 44};
    HasMethod x(storage[4]);
    x.drink_double_bourbon();

    BOOST_TEST( x.alain == 44 );
    BOOST_TEST( x.delon == 33 );
}

BOOST_AUTO_TEST_CASE(const_iterator)
{
    VECTOR_CONTAINER<A> storage(10, { 11, 12, 13});
    size_t i = 0;
    for (const auto& entry : storage) {
        BOOST_TEST( (entry->*(&A::val)) == 11);
        BOOST_TEST( (entry->*(&A::key)) == 12);
        BOOST_TEST( (entry->*(&A::dum)) == 13);
        ++i;
    }
    BOOST_TEST(i == 10);
}

BOOST_AUTO_TEST_CASE(mutable_iterator)
{
    VECTOR_CONTAINER<A> storage(10);
    for (auto& entry : storage) {
        entry->*(&A::val) = 21;
        entry->*(&A::key) = 22;
        entry->*(&A::dum) = 23;
    }
    for (const auto& entry : storage) {
        BOOST_TEST( (entry->*(&A::val)) == 21);
        BOOST_TEST( (entry->*(&A::key)) == 22);
        BOOST_TEST( (entry->*(&A::dum)) == 23);
    }
}

BOOST_AUTO_TEST_CASE(iterator_arrow_operator)
{
    VECTOR_CONTAINER<A> storage(10, { 11, 12, 13});
    BOOST_TEST( storage.cbegin()->get<&A::val>() == 11 );
    BOOST_TEST( storage.begin()->get<&A::key>()  == 12 );
}

BOOST_AUTO_TEST_CASE(bidirectional_iterator)
{
    VECTOR_CONTAINER<A> storage(10, { 11, 12, 13});
    size_t i = 0;
    auto it = storage.cend();
    do {
        --it;
        BOOST_TEST( (*it->*(&A::val)) == 11);
        BOOST_TEST( (*it->*(&A::key)) == 12);
        BOOST_TEST( (*it->*(&A::dum)) == 13);
        ++i;
    } while (it != storage.cbegin());
    BOOST_TEST(i == 10);
}

BOOST_AUTO_TEST_CASE(random_access_iterator)
{
    VECTOR_CONTAINER<A> storage(90);
    BOOST_TEST((storage.end() - storage.begin()) == 90 );
    BOOST_TEST((storage.begin() <= storage.end()));
    auto it = std::next(storage.begin(), 60);
    BOOST_TEST((storage.end() - it) == 30 );
}

BOOST_AUTO_TEST_CASE(reverse_iterator)
{
    VECTOR_CONTAINER<A> storage(10);
    int value = 20;
    for (auto& entry : storage) {
        entry->*(&A::val) = ++value;
        entry->*(&A::key) = ++value;
        entry->*(&A::dum) = ++value;
    }
    size_t i = 0;
    for (auto it = storage.crbegin(); it != storage.crend(); ++it) {
        BOOST_TEST( it->get<&A::val>() == value - 3 * i - 2 );
        BOOST_TEST( it->get<&A::key>() == value - 3 * i - 1 );
        BOOST_TEST( it->get<&A::dum>() == value - 3 * i );
        ++i;
    }
}

BOOST_AUTO_TEST_CASE(const_method)
{
    const VECTOR_CONTAINER<HasMethod> storage( 10, HasMethod{33, 44});
    BOOST_TEST( storage[3].method<&HasMethod::drink_cologne>(1) == 80);
}

BOOST_AUTO_TEST_CASE(non_const_method)
{
    VECTOR_CONTAINER<HasMethod> storage( 10, HasMethod{33, 44});
    storage[3].method<&HasMethod::drink_double_bourbon>();

    BOOST_TEST( (storage[3]->*(&HasMethod::alain)) == 44 );
    BOOST_TEST( (storage[3]->*(&HasMethod::delon)) == 33 );
}

BOOST_AUTO_TEST_CASE(const_method_and_mutable_field)
{
    struct HasMutable {
        mutable int x;
        void update_x() const { ++x; }
    };

    const VECTOR_CONTAINER<HasMutable> storage( 10, HasMutable{109});
    storage[3].method<&HasMutable::update_x>();
    BOOST_TEST( (storage[3]->*(&HasMutable::x)) == 110 );
}

BOOST_AUTO_TEST_CASE(arrow_star_method)
{
    const VECTOR_CONTAINER<HasMethod> storage( 10, HasMethod{33, 44});
    BOOST_TEST( (storage[3]->*(&HasMethod::drink_cologne))(1) == 80);
}

BOOST_AUTO_TEST_CASE(arrow_star_non_const_method)
{
    VECTOR_CONTAINER<HasMethod> storage( 10, HasMethod{33, 44});
    (storage[3]->*(&HasMethod::drink_double_bourbon))();

    BOOST_TEST( (storage[3]->*(&HasMethod::alain)) == 44 );
    BOOST_TEST( (storage[3]->*(&HasMethod::delon)) == 33 );
}

BOOST_AUTO_TEST_CASE(initialize_array_and_r_w)
{
    ARRAY_CONTAINER<A, 10> storage;
    storage[3]->*(&A::key) = 10;
    storage[3]->*(&A::val) = 3;
    storage[4]->*(&A::key) = 9;
    storage[4]->*(&A::val) = 6;

    BOOST_TEST( (storage[3]->*(&A::key)) == 10 );
    BOOST_TEST( (storage[3]->*(&A::val)) == 3 );
    BOOST_TEST( (storage[4]->*(&A::key)) == 9 );
    BOOST_TEST( (storage[4]->*(&A::val)) == 6 );
}

BOOST_AUTO_TEST_CASE(array_get__interface)
{
    ARRAY_CONTAINER<A, 10> storage;
    storage[2].get<&A::val>() = 234;
    BOOST_TEST( storage[2].get<&A::val>() == 234 );
}

BOOST_AUTO_TEST_CASE(array_assign_structure)
{
    ARRAY_CONTAINER<A, 10> storage;
    const A x{3, 7, 11};
    storage[2] = x; // copy assignment
    storage[3] = A{10, 3, 8}; // move assignment

    BOOST_TEST( (storage[2]->*(&A::val)) == 3 );
    BOOST_TEST( (storage[2]->*(&A::key)) == 7 );
    BOOST_TEST( (storage[2]->*(&A::dum)) == 11 );
    BOOST_TEST( (storage[3]->*(&A::val)) == 10 );
    BOOST_TEST( (storage[3]->*(&A::key)) == 3 );
    BOOST_TEST( (storage[3]->*(&A::dum)) == 8 );
}

BOOST_AUTO_TEST_CASE(vector_methods)
{
    VECTOR_CONTAINER<A> storage;
    storage.push_back(A{3, 14, 15});
    BOOST_TEST( (storage[0]->*(&A::val)) == 3 );
    BOOST_TEST( !storage.empty() );
    BOOST_CHECK_THROW( storage.at(1000), std::out_of_range );
    BOOST_TEST( (storage.front()->*(&A::dum)) == 15);
    BOOST_TEST( (storage.back()->*(&A::dum)) == 15);
}

BOOST_AUTO_TEST_CASE(vector_assign)
{
    VECTOR_CONTAINER<A> storage(10, A{3, 14, 15});
    storage.assign(5, A{2, 7, 1828});
    BOOST_TEST( (storage[7]->*(&A::dum)) == 15 );
    BOOST_TEST( (storage[2]->*(&A::dum)) == 1828 );
}

BOOST_AUTO_TEST_CASE(vector_capacity)
{
    VECTOR_CONTAINER<A> storage(10);
    storage.reserve(200);
    BOOST_TEST( storage.capacity() >= 200 );
    BOOST_TEST( storage.size() == 10 );

    storage.shrink_to_fit();
    BOOST_TEST( storage.capacity() >= 10 );
}

BOOST_AUTO_TEST_CASE(array_fill)
{
    ARRAY_CONTAINER<A, 100> storage;
    storage.fill(A{2, 7, 1828});
    BOOST_TEST( (storage[93]->*(&A::dum)) == 1828 );
}

BOOST_AUTO_TEST_CASE(move_semantics)
{
    struct Point
    {
        int x = 0;
        int y = 0;
        std::unique_ptr<int> z;
    };

    ARRAY_CONTAINER<Point, 100> storage;
    storage[3] = Point{2, 4, std::make_unique<int>(10)};
    storage[4] = Point{2, 4, std::make_unique<int>(7)};

    BOOST_TEST( (storage[3]->*(&Point::x)) == 2 );
    BOOST_TEST( (storage[3]->*(&Point::y)) == 4 );
    BOOST_TEST( *(storage[3]->*(&Point::z)) == 10 );

    Point point = storage[4].aggregate_move();
    BOOST_TEST( *(point.z) == 7 );

    Point point2 = std::move(storage[3]);
    BOOST_TEST( *(point2.z) == 10 );
}
