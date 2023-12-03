cmake_minimum_required(VERSION 3.22)

set(VCPKG_INSTALLED_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed")

project(lldext)

if(DEFINED ENV{LLDEXT_VERSION_MAJOR})
    set(lldext_VERSION_MAJOR $ENV{LLDEXT_VERSION_MAJOR})
else()
    set(lldext_VERSION_MAJOR 1)
endif()

if(DEFINED ENV{LLDEXT_VERSION_MINOR})
    set(lldext_VERSION_MINOR $ENV{LLDEXT_VERSION_MINOR})
else()
    set(lldext_VERSION_MINOR 0)
endif()

if(DEFINED ENV{LLDEXT_VERSION_PATCH})
    set(lldext_VERSION_PATCH $ENV{LLDEXT_VERSION_PATCH})
else()
    set(lldext_VERSION_PATCH 0)
endif()

if(DEFINED ENV{LLDEXT_VERSION_TWEAK})
    set(lldext_VERSION_TWEAK $ENV{LLDEXT_VERSION_TWEAK})
else()
    set(lldext_VERSION_TWEAK 0)
endif()

set(lldext_VERSION
  ${lldext_VERSION_MAJOR}.${lldext_VERSION_MINOR}.${lldext_VERSION_PATCH}.${lldext_VERSION_TWEAK}
)

message("Building version ${lldext_VERSION}")

add_compile_definitions(UNICODE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")

add_subdirectory(lldext)
