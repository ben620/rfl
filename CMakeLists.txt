cmake_minimum_required(VERSION 3.20)

project(rfl LANGUAGES CXX)

add_library(rfl INTERFACE)
add_library(rfl::rfl ALIAS rfl)

target_include_directories(
    rfl
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(rfl INTERFACE cxx_std_20)

include(CTest)
if(BUILD_TESTING)
    add_executable(rfl_tests tests/cppreflect_tests.cpp)
    target_link_libraries(rfl_tests PRIVATE rfl::rfl)
    target_compile_options(
        rfl_tests
        PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )

    add_test(NAME rfl_tests COMMAND rfl_tests)
endif()

option(RFL_BUILD_BENCHMARKS "Build the rfl JSON benchmarks" ON)
if(RFL_BUILD_BENCHMARKS)
    add_executable(rfl_benchmarks benchmarks/json_benchmark.cpp)
    target_link_libraries(rfl_benchmarks PRIVATE rfl::rfl)
endif()
