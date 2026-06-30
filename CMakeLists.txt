cmake_minimum_required(VERSION 3.10)
project(Snake CXX)

set(CMAKE_CXX_STANDARD 17)

# ── Find SDL2 ──────────────────────────────────────────────────────────────────
find_package(SDL2 REQUIRED)

# SDL2_ttf is not always a cmake package, find it manually
find_path(SDL2_TTF_INCLUDE_DIR SDL2/SDL_ttf.h
    PATHS
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    /mingw64/include
)
find_library(SDL2_TTF_LIBRARY
    NAMES SDL2_ttf
    PATHS
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
    /mingw64/lib
)

if(NOT SDL2_TTF_LIBRARY)
    message(FATAL_ERROR "SDL2_ttf library not found. Please install libsdl2-ttf-dev.")
endif()

add_executable(snake snake.cpp)

target_include_directories(snake PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIR}
)

target_link_libraries(snake PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARY}
)

# On Windows with MinGW, also link mingw32
if(MINGW)
    target_link_libraries(snake PRIVATE mingw32)
endif()
