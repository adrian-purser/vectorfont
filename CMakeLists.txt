cmake_minimum_required(VERSION 3.15)
project(
	vectorfont 
	VERSION 1.0.0 
	DESCRIPTION "Utilities for parsing vector fonts"
	LANGUAGES CXX
)

option(ENABLE_TESTS "Enable Unit Tests using Catch2" OFF)

if(MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(cmake/StandardProjectSettings.cmake)
include(cmake/StaticAnalyzers.cmake)

add_library( 	vectorfont 
							STATIC
							src/vectorfont/hershey.cpp
						)

# Make sure the compiler can find include files for our library
# when other libraries or executables link to vectorfont
target_include_directories(vectorfont PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ENABLE_TESTS)
	enable_testing()
	add_subdirectory(src/tests)
endif()
