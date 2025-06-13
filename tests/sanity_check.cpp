#include <catch2/catch_test_macros.hpp>

int successor(int a) {return a + 1;}

TEST_CASE( "We can add!", "[sanity]" ) {
	REQUIRE( successor(1) == 2);
}
