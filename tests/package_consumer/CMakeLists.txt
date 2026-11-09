cmake_minimum_required(VERSION 3.21)
project(SpriteUIPackageConsumer LANGUAGES CXX)

find_package(SpriteUI 0.6 CONFIG REQUIRED)

add_executable(spriteui_package_consumer main.cpp)
target_link_libraries(spriteui_package_consumer PRIVATE SpriteUi::SpriteUI)
