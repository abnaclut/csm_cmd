cmake_minimum_required(VERSION 3.16)
project(csm_cmd VERSION 1.1.0 LANGUAGES CXX)

# ============================================================
# Compiler settings
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ============================================================
# Options for embedding in thirdparty
# ============================================================
option(BUILD_SHARED_LIBS         "Build shared libraries"                                  OFF)
option(CSM_CMD_BUILD_TESTS       "Build csm_cmd unit tests"                                ON)
option(CSM_CMD_BUILD_EXAMPLES    "Build example applications"                              OFF)
option(CSM_CMD_BUILD_TERMINAL    "Build terminal test application"                         ON)
option(CSM_CMD_USE_SYSTEM_REPLXX "Use system replxx instead of FetchContent"               OFF)

# ============================================================
# Dependencies
# ============================================================
include(FetchContent)

find_package(Threads REQUIRED)

# replxx - REPL library for terminal input
if(CSM_CMD_USE_SYSTEM_REPLXX)
  find_package(replxx REQUIRED)
  set(REPLXX_TARGET replxx::replxx)
else()
  FetchContent_Declare(
          replxx
          GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
          GIT_TAG        release-0.0.4
  )
  set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(replxx)
  set(REPLXX_TARGET replxx)
endif()

# ============================================================
# Common compiler warnings
# ============================================================
function(target_add_warnings TARGET)
  target_compile_options(${TARGET} PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
          -Wall -Wextra -Wpedantic
          >
          $<$<CXX_COMPILER_ID:MSVC>:
          /W4
          >
  )
endfunction()

# ============================================================
# Main library
# ============================================================
set(SOURCES
        src/cmd_parser.cpp
        src/cmd_registry.cpp
        src/cmd_registration.cpp
        src/csm_terminal.cpp
        src/logger.cpp
)

set(PUBLIC_HEADERS
        include/cmd_parser.hpp
        include/cmd_registry.hpp
        include/cmd_registration.hpp
        include/csm_terminal.hpp
        include/logger.hpp
)

add_library(csm_cmd ${SOURCES} ${PUBLIC_HEADERS})

# Set version properties
set_target_properties(csm_cmd PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        DESCRIPTION "csm_cmd command processing library"
        POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(csm_cmd
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(csm_cmd
        PUBLIC
        ${REPLXX_TARGET}
        Threads::Threads
)

# Apply warnings with extra flags for library
target_compile_options(csm_cmd PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall -Wextra -Wpedantic -Werror=return-type
        >
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX-
        >
)

# ============================================================
# Terminal application
# ============================================================
if(CSM_CMD_BUILD_TERMINAL)
  add_executable(csm_terminal src/main.cpp)
  target_link_libraries(csm_terminal PRIVATE csm_cmd)
  target_add_warnings(csm_terminal)

  set_target_properties(csm_terminal PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()

# ============================================================
# Tests
# ============================================================
if(CSM_CMD_BUILD_TESTS)
  FetchContent_Declare(
          googletest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG        v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(csm_cmd_tests
          src/tests/test_parser.cpp
          src/tests/test_registry.cpp
          src/tests/test_completion.cpp
          src/tests/test_timeout.cpp
  )

  target_link_libraries(csm_cmd_tests
          PRIVATE
          csm_cmd
          gtest
          gtest_main
  )

  target_add_warnings(csm_cmd_tests)

  set_target_properties(csm_cmd_tests PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

  include(GoogleTest)
  gtest_discover_tests(csm_cmd_tests)
endif()

# ============================================================
# Examples
# ============================================================
if(CSM_CMD_BUILD_EXAMPLES)
  add_executable(example_simple examples/simple/main.cpp)
  target_link_libraries(example_simple PRIVATE csm_cmd)
  target_add_warnings(example_simple)
  set_target_properties(example_simple PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

  add_executable(example_custom examples/custom/main.cpp)
  target_link_libraries(example_custom PRIVATE csm_cmd)
  target_add_warnings(example_custom)
  set_target_properties(example_custom PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()

# ============================================================
# Print configuration summary
# ============================================================
message(STATUS "")
message(STATUS "csm_cmd configuration:")
message(STATUS "  Version:                    ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:              ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type:                ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libraries:          ${BUILD_SHARED_LIBS}")
message(STATUS "  Build tests:               ${CSM_CMD_BUILD_TESTS}")
message(STATUS "  Build examples:            ${CSM_CMD_BUILD_EXAMPLES}")
message(STATUS "  Build terminal:            ${CSM_CMD_BUILD_TERMINAL}")
message(STATUS "  Use system replxx:         ${CSM_CMD_USE_SYSTEM_REPLXX}")
message(STATUS "")