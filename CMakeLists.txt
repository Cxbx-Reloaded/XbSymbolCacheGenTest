# CMakeList.txt : CMake project for XbSymbolCacheGenTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (XbSymbolCacheGenTest)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

include_directories("${PROJECT_SOURCE_DIR}/import/XbSymbolDatabase" "${PROJECT_SOURCE_DIR}/import/SimpleIni")

add_subdirectory ("${PROJECT_SOURCE_DIR}/import/XbSymbolDatabase")

# Suppress extra stuff from generated solution
set(CMAKE_SUPPRESS_REGENERATION true)

# Add source to this project's executable.
file(GLOB SOURCES "src/*.cpp")
add_executable(XbSymbolCacheGenTest ${SOURCES})

# GCC needs a lib for std::filesystem
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(STD_FILESYSTEM_LIBRARY -lstdc++fs)
else()
  set(STD_FILESYSTEM_LIBRARY)
endif()

target_link_libraries(XbSymbolCacheGenTest LINK_PUBLIC XbSymbolDatabase ${STD_FILESYSTEM_LIBRARY})
