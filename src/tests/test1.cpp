

#include <catch2/catch_test_macros.hpp>

int 
test1()
{
	return 5;
}

TEST_CASE( "Test 1", "[test1]")
{
	REQUIRE( test1() == 5 );
}

