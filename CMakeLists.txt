# La Sala Segreta — build con CMake (alternativa al Makefile). Passo M2 della roadmap.
#   cmake -S . -B build && cmake --build build -j      ->  build/sala_segreta
cmake_minimum_required(VERSION 3.16)
project(sala_segreta CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)               # -O3 di default; il Makefile usa -O2 (stesso comportamento)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# sala_segreta.cpp + tutti i .cpp di src/ (per ora un solo eseguibile; dal passo M6 diventeranno librerie per modulo)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(sala_segreta sala_segreta.cpp ${GAME_SOURCES})
target_include_directories(sala_segreta PRIVATE src)
target_compile_options(sala_segreta PRIVATE -Wall -Wextra)
target_link_libraries(sala_segreta PRIVATE PkgConfig::SDL2 PkgConfig::SDL2_TTF OpenGL::GL)

# Test headless (nessun SDL/GL): `cmake --build build --target test_defs && build/test_defs`
add_executable(test_defs EXCLUDE_FROM_ALL tests/unit/test_defs.cpp src/game/defs.cpp src/items.cpp src/state.cpp)
target_include_directories(test_defs PRIVATE src)
target_compile_options(test_defs PRIVATE -Wall -Wextra)
