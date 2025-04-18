cmake_minimum_required(VERSION 3.20)
project(tam 
  VERSION 0.1.0
  DESCRIPTION "Triangle Abstract Machine emulator"
  HOMEPAGE_URL "https://github.com/pszik/tam"
  LANGUAGES C
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsave-optimization-record=yaml")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

