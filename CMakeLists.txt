cmake_minimum_required(VERSION 3.2)

project(game VERSION 0.1.0)

find_package(assimp REQUIRED)
find_package(box3d REQUIRED)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../bubble_game_framework
    ${CMAKE_CURRENT_BINARY_DIR}/bubble_game_framework)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb -pg")
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(SOURCES
    source/thirdparty/stb_image.cpp
    source/model_loader.cpp
    source/character_mover.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    assimp::assimp
    box3d::box3d
    bubble
)
