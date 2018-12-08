# CMakeList.txt : CMake project for XbSymbolCacheGenTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (XbSymbolCacheGenTest)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include_directories("${PROJECT_SOURCE_DIR}/import/XbSymbolDatabase" "${PROJECT_SOURCE_DIR}/import/SimpleIni")

add_subdirectory ("${PROJECT_SOURCE_DIR}/import/XbSymbolDatabase")

# Suppress extra stuff from generated solution
set(CMAKE_SUPPRESS_REGENERATION true)

# Add source to this project's executable.
file(GLOB SOURCES "src/*.cpp")
add_executable(XbSymbolCacheGenTest ${SOURCES})
target_link_libraries (XbSymbolCacheGenTest LINK_PUBLIC XbSymbolDatabase)
