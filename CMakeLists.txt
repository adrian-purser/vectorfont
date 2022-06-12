cmake_minimum_required(VERSION 3.15)
project(
	vectorfont 
	VERSION 1.0.0 
	DESCRIPTION "Utilities for parsing vector fonts"
	LANGUAGES CXX
)

option(ENABLE_VECTORFONT_TESTS "Enable Unit Tests for vectorfont" OFF)

if(MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library( 	vectorfont 
							STATIC
							src/vectorfont/hershey.cpp
						)

# Make sure the compiler can find include files for our library
# when other libraries or executables link to vectorfont
target_include_directories(vectorfont PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(vectorfont PUBLIC cxx_std_20)

if(ENABLE_VECTORFONT_TESTS)
	enable_testing()
	add_subdirectory(src/tests)
endif()
