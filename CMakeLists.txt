cmake_minimum_required(VERSION 3.22)
project(FlappyBird C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(FlappyBird
    src/main.c
    src/cli.c
    src/game.c
    src/mcu.c
    src/score.c
    src/ui.c
)

target_include_directories(FlappyBird PRIVATE include)

target_compile_options(FlappyBird PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(FlappyBird PRIVATE pthread)