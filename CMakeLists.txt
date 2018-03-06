cmake_minimum_required(VERSION 3.0)
project(smolog)

option(SMOLOG_DISABLE_MACROS "Disable macros" OFF)

add_library(smolog STATIC smolog.hpp smolog.cpp)
if(SMOLOG_DISABLE_MACROS)
	target_compile_definitions(smolog PRIVATE -DSMOLOG_DISABLE_MACROS)
endif(SMOLOG_DISABLE_MACROS)
add_executable(smolog_examples examples.cpp)

target_link_libraries(smolog_examples smolog)

