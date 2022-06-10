

#include <catch2/catch_test_macros.hpp>
#include "vectorfont/vectorfont.h"


TEST_CASE( "Rectangle Dimensions", "[rect-dim]")
{
	vectorfont::Rect rect(10,15,20,25);

	REQUIRE( rect.width() == 20 );
	REQUIRE( rect.height() == 25 );

	SECTION( "Resizing Width" )
	{
		rect.resize(50,0);
		REQUIRE( rect.width() == 50 );
	}

	SECTION( "Resizing Height" )
	{
		rect.resize(0,50);
		REQUIRE( rect.height() == 50 );
	}

	SECTION( "Relative posative move while keeping same dimensions" )
	{
		rect.move_relative(5,7);
		REQUIRE( rect.width() == 20 );
		REQUIRE( rect.height() == 25 );		
	}

	SECTION( "Relative negative move while keeping same dimensions" )
	{
		rect.move_relative(-5,-7);
		REQUIRE( rect.width() == 20 );
		REQUIRE( rect.height() == 25 );		
	}

	SECTION( "Absoluite posative move while keeping same dimensions" )
	{
		rect.move(5,7);
		REQUIRE( rect.width() == 20 );
		REQUIRE( rect.height() == 25 );		
	}

	SECTION( "Absoluite negative move while keeping same dimensions" )
	{
		rect.move(-5,-7);
		REQUIRE( rect.width() == 20 );
		REQUIRE( rect.height() == 25 );		
	}

	SECTION( "Resize Edge Check" )
	{
		rect.set(10,20,3,3);
		REQUIRE( rect.left 		== 10 );
		REQUIRE( rect.top 		== 20 );
		REQUIRE( rect.right 	== 12 );
		REQUIRE( rect.bottom	== 22 );				
	}

	SECTION( "Inflate" )
	{
		rect.inflate(1);
		REQUIRE( rect.width() == 22 );
		REQUIRE( rect.height() == 27 );		
	}	

	SECTION( "Deflate" )
	{
		rect.deflate(1);
		REQUIRE( rect.width() == 18 );
		REQUIRE( rect.height() == 23 );		
	}	

	SECTION( "Deflate by more than the current size" )
	{
		rect.deflate(30);
		REQUIRE( rect.width() == 1 );
		REQUIRE( rect.height() == 1 );		
	}		
}

