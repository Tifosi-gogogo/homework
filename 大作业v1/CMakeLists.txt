cmake_minimum_required(VERSION 3.20)

project(TennisDuel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(TennisDuel
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/GameWidget.h
    src/GameWidget.cpp
    src/MatchController.h
    src/MatchController.cpp
    src/GameTypes.h
)

target_link_libraries(TennisDuel PRIVATE Qt6::Widgets)

target_include_directories(TennisDuel PRIVATE src)

if(MSVC)
    target_compile_options(TennisDuel PRIVATE /utf-8)
endif()
