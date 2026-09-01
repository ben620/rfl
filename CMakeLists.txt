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
    foreach(test_name IN ITEMS rfl_tests rfl_bin_tests)
        if(test_name STREQUAL "rfl_tests")
            set(test_source tests/cppreflect_tests.cpp)
        else()
            set(test_source tests/bin_tests.cpp)
        endif()

        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE rfl::rfl)
        target_compile_options(
            ${test_name}
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
                $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

option(RFL_BUILD_BENCHMARKS "Build the rfl JSON benchmarks" ON)
if(RFL_BUILD_BENCHMARKS)
    add_executable(rfl_benchmarks benchmarks/json_benchmark.cpp)
    target_link_libraries(rfl_benchmarks PRIVATE rfl::rfl)
endif()
