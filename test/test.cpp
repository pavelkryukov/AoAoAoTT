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

struct A {
    int val;
    int key;
    int dum; 
};

TEST_CASE("AoS: initialize and r/w")
{
    AoS<A> aos( 10);
    aos[3]->*(&A::key) = 10;
    aos[3]->*(&A::val) = 3;
    aos[4]->*(&A::key) = 9;
    aos[4]->*(&A::val) = 6;
    
    CHECK( aos[3]->*(&A::key) == 10 );
    CHECK( aos[3]->*(&A::val) == 3 );
    CHECK( aos[4]->*(&A::key) == 9 );
    CHECK( aos[4]->*(&A::val) == 6 );
}

TEST_CASE("SoA: initialize and r/w")
{
    SoA<A> soa( 10);
    soa[3]->*(&A::key) = 10;
    soa[3]->*(&A::val) = 3;
    soa[4]->*(&A::key) = 9;
    soa[4]->*(&A::val) = 6;
    
    CHECK( soa[3]->*(&A::key) == 10 );
    CHECK( soa[3]->*(&A::val) == 3 );
    CHECK( soa[4]->*(&A::key) == 9 );
    CHECK( soa[4]->*(&A::val) == 6 );
}

TEST_CASE("AoS: assign structure")
{
    AoS<A> aos( 10);
    aos[3] = A{10, 3, 8};

    CHECK( aos[3]->*(&A::val) == 10 );
    CHECK( aos[3]->*(&A::key) == 3 );
    CHECK( aos[3]->*(&A::dum) == 8 );
}