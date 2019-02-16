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
