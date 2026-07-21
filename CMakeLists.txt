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
option(CSM_CMD_BUILD_TERMINAL    "Build terminal test application"                         OFF)
option(CSM_CMD_INSTALL           "Enable installation (disable when used as subdirectory)" OFF)
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
        include/csm_cmd/cmd_parser.hpp
        include/csm_cmd/cmd_registry.hpp
        include/csm_cmd/cmd_registration.hpp
        include/csm_cmd/csm_terminal.hpp
        include/csm_cmd/logger.hpp
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(csm_cmd
        PUBLIC
        ${REPLXX_TARGET}
        Threads::Threads
)

# Compiler warnings
target_compile_options(csm_cmd PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall -Wextra -Wpedantic -Werror=return-type
        >
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX-
        >
)

# ============================================================
# Export for use as subdirectory (thirdparty)
# ============================================================
export(TARGETS csm_cmd
        NAMESPACE csm_cmd::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/csm_cmdTargets.cmake"
)

# ============================================================
# Terminal test application (only for development)
# ============================================================
if(CSM_CMD_BUILD_TERMINAL)
  add_executable(csm_terminal src/main.cpp)
  target_link_libraries(csm_terminal PRIVATE csm_cmd)

  target_compile_options(csm_terminal PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
          -Wall -Wextra -Wpedantic
          >
          $<$<CXX_COMPILER_ID:MSVC>:
          /W4
          >
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

  target_compile_options(csm_cmd_tests PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
          -Wall -Wextra -Wpedantic
          >
          $<$<CXX_COMPILER_ID:MSVC>:
          /W4
          >
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

  add_executable(example_custom examples/custom/main.cpp)
  target_link_libraries(example_custom PRIVATE csm_cmd)
endif()

# ============================================================
# Installation (disabled by default for thirdparty usage)
# ============================================================
if(CSM_CMD_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # Install library
  install(TARGETS csm_cmd
          EXPORT csm_cmdTargets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # Install public headers
  install(FILES ${PUBLIC_HEADERS}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/csm_cmd
  )

  # Install CMake package config
  write_basic_package_version_file(
          "${CMAKE_CURRENT_BINARY_DIR}/csm_cmdConfigVersion.cmake"
          VERSION ${PROJECT_VERSION}
          COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
          "${CMAKE_CURRENT_SOURCE_DIR}/cmake/csm_cmdConfig.cmake.in"
          "${CMAKE_CURRENT_BINARY_DIR}/csm_cmdConfig.cmake"
          INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csm_cmd
  )

  install(EXPORT csm_cmdTargets
          NAMESPACE csm_cmd::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csm_cmd
  )

  install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/csm_cmdConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/csm_cmdConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csm_cmd
  )

  # Install documentation (optional)
  install(DIRECTORY docs/
          DESTINATION ${CMAKE_INSTALL_DOCDIR}
          PATTERN ".git" EXCLUDE
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
message(STATUS "  Install enabled:           ${CSM_CMD_INSTALL}")
message(STATUS "  Use system replxx:         ${CSM_CMD_USE_SYSTEM_REPLXX}")
message(STATUS "")