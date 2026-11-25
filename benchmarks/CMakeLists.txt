cmake_minimum_required(VERSION 3.31)

# =============================================================================
# Audio benchmarks (google/benchmark).
#
# The benchmarks link the plugin core, like the tests and every plugin format
# do, and time the audio callback of the processor on a few factory presets.
# Build with -DBUILD_BENCHMARKS=ON (the default) and run them with `just bench`,
# or build the `${PROJECT_NAME}_benchmarks` target and execute it directly to
# pass google/benchmark arguments (--benchmark_filter=...).
# =============================================================================

set (BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set (BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set (BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set (BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

_yup_fetchcontent_declare (benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.10.0)

FetchContent_MakeAvailable (benchmark)

set_target_properties (benchmark PROPERTIES FOLDER "Benchmarks")
set_target_properties (benchmark_main PROPERTIES FOLDER "Benchmarks")

set (target_name ${PROJECT_NAME}_benchmarks)

file (GLOB benchmark_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/bench_*.cpp")
list (APPEND benchmark_sources "${CMAKE_CURRENT_LIST_DIR}/main.cpp")

add_executable (${target_name} ${benchmark_sources})

target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../source")

# The core is compiled against SDL without linking it, as each plugin format
# links its own: a plain executable links it like the standalone does.
target_link_libraries (${target_name} PRIVATE
    ${PLUGIN_TARGET}_shared
    sdl::sdl
    benchmark::benchmark)

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${benchmark_sources})

set_target_properties (${target_name} PROPERTIES
    FOLDER "Benchmarks"
    XCODE_GENERATE_SCHEME ON
    XCODE_SCHEME_ARGUMENTS "--benchmark_counters_tabular=true"
    VS_DEBUGGER_COMMAND_ARGUMENTS "--benchmark_counters_tabular=true")

# Runs the benchmarks from the build, wherever the generator put the executable.
add_custom_target (${target_name}_run
    COMMAND ${target_name} --benchmark_counters_tabular=true
    USES_TERMINAL
    VERBATIM)

set_target_properties (${target_name}_run PROPERTIES FOLDER "Benchmarks")
