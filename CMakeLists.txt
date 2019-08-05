cmake_minimum_required(VERSION 3.13)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/JamLib/Deps/cmake-pch)
project(onehop CXX CXXPCH)

include("JamLib/CMakeInclude.cmake")

file(GLOB_RECURSE SOURCE_FILES Src/*.cpp Src/*.hpp)
add_executable(onehop ${SOURCE_FILES})

set_target_properties(onehop PROPERTIES
	COMPILE_FLAGS "-Wall -Wextra -Wshadow -pedantic --std=c++17"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_NAME}"
)

target_link_jamlib(onehop)

target_precompiled_header(onehop PCH.hpp)
