cmake_minimum_required(VERSION 3.12)

# cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja .

# set the project name and version
project(PublishSubscribe VERSION 1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# https://stackoverflow.com/questions/11944060/how-to-detect-target-architecture-using-cmake

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(UNIX AND (${CMAKE_SYSTEM_NAME} STREQUAL ${CMAKE_HOST_SYSTEM_NAME}))
    EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
elseif(ANDROID)
    set(ARCHITECTURE ${ANDROID_ABI})
elseif(APPLE)
    set(ARCHITECTURE ${OSX_ARCHITECTURES})
else()
    set(ARCHITECTURE "unknown")
endif()

#detect Platform (Jetson, Raspberry Pi, ...)
#EXECUTE_PROCESS( COMMAND cat /proc/cpuinfo COMMAND grep Hardware COMMAND tr -d '\n' OUTPUT_VARIABLE PLATFORM_DETECTION )
# grep Hardware /proc/cpuinfo
set(PLATFORM_DETECTION "unknown")

message( STATUS "Architecture: ${ARCHITECTURE}" )

# if(NOT LINUX) should work, too, if you need that
if(LINUX)
    message(STATUS "OS: Linux")
# linux stuff here
else()
    message(STATUS "OS: Unix Not Linux")
    # stuff that should happen not on Linux
endif()

if(WIN32)
    # for Windows operating system in general
    message(STATUS "OS: Windows")
endif()

if(MSVC OR MSYS OR MINGW)
    # for detecting Windows compilers
endif()

#if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
#    // do something for Linux
#else
#    // do something for other OS

# Host CMAKE_HOST_SYSTEM_NAME
# Target CMAKE_SYSTEM_NAME

# CMAKE_COMPILER_IS_GNUCC
# CMAKE_SYSTEM_PROCESSOR
# CMAKE_HOST_SYSTEM_PROCESSOR
# The ${ANDROID_ABI} variable is the way to go in Android, where it assumes values like arm64-v8a, x86_64 and so on

# specify the C++ standard
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(LINUX)
    set(CMAKE_CXX_FLAGS "-march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -D DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -funsafe-math-optimizations -D NDEBUG")

    set(CMAKE_C_FLAGS "-march=native")
    set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -D DEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -funsafe-math-optimizations -D NDEBUG")

elseif(WIN32)
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_DEBUG "/D DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox /D NDEBUG /fp:fast")

    set(CMAKE_C_FLAGS "")
    set(CMAKE_C_FLAGS_DEBUG "/D DEBUG")
    set(CMAKE_C_FLAGS_RELEASE "/Ox /D NDEBUG /fp:fast")
endif()

if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    message(STATUS "C++20 or newer is supported/enabled")
endif()

# mem pool allocator to prevent memory fragmentation for small and frequently dynamically allocated events/messages
# note: it can introduce some slow-down but memory allocation pattern is more predictive and stable

# uncomment to use mem pool allocator instead of regular heap allocation
#add_compile_definitions(USE_MEM_POOL_ALLOCATOR)

# uncomment to warmup the mem pool with pre-allocated chunks of memory
#add_compile_definitions(USE_MEM_POOL_ALLOCATOR_WARMUP)

# display actual compiling definitions
get_directory_property(DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
foreach( d ${DirDefs} )
    message( STATUS "Found Define: " ${d} )
endforeach()

# uname -m
# i386 i686 x86_64 ia64 alpha amd64 arm armeb armel hppa m32r m68k mips mipsel powerpc ppc64 s390 s390x sh3 sh3eb sh4 sh4eb sparc

# https://developer.android.com/ndk/guides/abis
#

# Local header files here ONLY
file(GLOB_RECURSE TARGET_H CONFIGURE_DEPENDS
    *.h
    *.hpp
    *.inl
)

# Local source files here

set(TARGET_TOOLS_SRC
    tools/mem_pool_allocator.cpp
    tools/sync_object.cpp
)

set(PORTABLE_CONCURRENCY_SRC
    portable_concurrency/bits/portable_concurrency.cpp
)

set(TARGET_SRC
    "${PORTABLE_CONCURRENCY_SRC}"
    "${TARGET_TOOLS_SRC}"
)

include_directories("${PROJECT_SOURCE_DIR}")

# framework code as a static library so it can be shared between the main
# executable and the test executable below
add_library(framework_modules STATIC
    "${TARGET_SRC}"
    "${TARGET_H}"
)
target_include_directories(framework_modules PUBLIC "${PROJECT_SOURCE_DIR}")

add_executable(publish_subscribe
    main.cpp
)
target_link_libraries(publish_subscribe PRIVATE framework_modules)

if(LINUX)
    target_link_libraries(framework_modules PUBLIC -lm -lpthread)
elseif(WIN32)
    target_link_libraries(framework_modules PUBLIC)
endif()

################################
# Google Test
################################

# https://matgomes.com/integrate-google-test-into-cmake/
include(FetchContent)

find_package(GTest 1.11.0 QUIET)
if(GTest_FOUND)
    message("Google Test found")
else()
    message(STATUS "Google Test: not found locally, fetching from release archive...")

    set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

    FetchContent_Declare(
        googletest
        URL      https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
        URL_HASH SHA256=b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    if(MSVC OR MSYS OR MINGW)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(BUILD_GTEST ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
endif()

if(TARGET GTest::gtest_main)
    set(GTEST_MAIN_TARGET GTest::gtest_main)
elseif(TARGET gtest_main)
    set(GTEST_MAIN_TARGET gtest_main)
else()
    message(FATAL_ERROR "GoogleTest main target not available")
endif()

include(GoogleTest)
enable_testing()

################################
# Tests
################################

# Add your test files here
set(TEST_SOURCES
    tests/test_sync_dictionary.cpp
)

add_executable(publish_subscribe_tests "${TEST_SOURCES}")

# Static analysis of production code is enough: tests are excluded from clang-tidy.
set_target_properties(publish_subscribe_tests PROPERTIES CXX_CLANG_TIDY "")

target_link_libraries(publish_subscribe_tests PRIVATE framework_modules ${GTEST_MAIN_TARGET})

if(LINUX)
    target_link_libraries(publish_subscribe_tests PRIVATE -lpthread)
endif()

gtest_discover_tests(publish_subscribe_tests)

