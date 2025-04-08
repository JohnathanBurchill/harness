cmake_minimum_required(VERSION 3.16)
project(SimpleHarness C)

include(FetchContent)

# Set where raylib will be cloned and built
set(RAYLIB_SOURCE_DIR ${CMAKE_BINARY_DIR}/raylib)

# Automatically download raylib at tag 5.5
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
  SOURCE_DIR ${RAYLIB_SOURCE_DIR}
)

# Disable building raylib examples/games
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Set default build type to Release if not specified (required for raylib).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  message(STATUS "CMAKE_BUILD_TYPE not set -- defaulting to Release")
endif()

# Download and add raylib
FetchContent_MakeAvailable(raylib)

# Your executable
add_executable(simple_harness simple_harness.c fira_code_bold_font.h)

# Link with raylib
target_link_libraries(simple_harness PRIVATE raylib)

# macOS-specific: frameworks and deployment target
if(APPLE)
  # Set deployment target
  set(CMAKE_OSX_DEPLOYMENT_TARGET "14.6" CACHE STRING "Minimum macOS deployment version")

  # Link necessary frameworks
  target_link_libraries(simple_harness PRIVATE
    "-framework CoreVideo"
    "-framework IOKit"
    "-framework Cocoa"
    "-framework GLUT"
    "-framework OpenGL"
  )
endif()

# Linux-specific system libs
if(UNIX AND NOT APPLE)
  target_link_libraries(simple_harness PRIVATE m pthread dl rt X11)
endif()
