cmake_minimum_required(VERSION 3.2.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
project(aws-polly)

# Options
option(BUILD_TEST                       "Build the testing tree."                           OFF)

# Make warning as error
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

# Compiled as C99
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Print option values
message(STATUS "BUILD_TEST                      = ${BUILD_TEST}")

include(FetchContent)

# Add libraries
include(libmbedtls)
include(libllhttp)
if(${BUILD_TEST})
    include(libgtest)
endif()

# Add sources
add_subdirectory(src)

# Add samples
add_subdirectory(samples)

# Add test
if(${BUILD_TEST})
    add_subdirectory(tests)
endif()