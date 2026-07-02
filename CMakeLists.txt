cmake_minimum_required(VERSION 3.10)

project(MicroBoxEmulator
  VERSION 1.0)

# This configures the language version required by the project.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()


# This provides the dynamic setup where the version from the project() function
# above gets inserted into the emulator_config.h header file.
configure_file(emulator_config.h.in emulator_config.h)

# Ensures that SFML-specific code is included
add_compile_definitions(EMULATOR=1)

# This recursively grabs all .cpp files in the specified directories.
# It allows for adding new source files without needing to modify this CMakeLists.txt.
file(GLOB_RECURSE SFML_PLATFORM_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/platform/emulator/*.cpp)
file(GLOB_RECURSE PLATFORM_DEFS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/platform/interface/*.cpp)
file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/common/*.cpp)
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/games/*.cpp)
file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/apps/*.cpp)
file(GLOB TOP_LEVEL_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(microbox-core
  ${SFML_PLATFORM_SOURCES}
  ${PLATFORM_DEFS}
  ${COMMON_SOURCES}
  ${GAME_SOURCES}
  ${APP_SOURCES}
  ${TOP_LEVEL_SOURCES}
)


target_include_directories(microbox-core PUBLIC
  "${PROJECT_BINARY_DIR}"
)

target_link_libraries(microbox-core PRIVATE
  SFML::Graphics
  CURL::libcurl
)

add_executable(microbox-emulator
  microbox_emulator.cpp
)

# Ensure that we can fetch dependencies from the internet.
include(FetchContent)

# Set up SFML dependency
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# This downloads the Catch2 testing framework.
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# We assume that libcurl is installed on the system. If not, you may need to install it manually.
find_package(CURL REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(microbox-emulator PRIVATE DEBUG_BUILD)
    target_compile_options(microbox-emulator PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(microbox-emulator PRIVATE
        -fsanitize=address
    )
endif()

target_link_libraries(microbox-emulator PRIVATE
  microbox-core
  SFML::Graphics
  CURL::libcurl
)

# This ensures that the `tests/` directory in our project is recognized as part of
# the cmake build.
add_subdirectory(tests)
