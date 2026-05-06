cmake_minimum_required(VERSION 3.20)

project(TennisDuel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg Multimedia)

add_executable(TennisDuel
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/GameWidget.h
    src/GameWidget.cpp
    src/ItemCatalog.h
    src/ItemCatalog.cpp
    src/MatchController.h
    src/MatchController.cpp
    src/GameTypes.h
)

target_link_libraries(TennisDuel PRIVATE Qt6::Widgets)
target_link_libraries(TennisDuel PRIVATE Qt6::Svg)
target_link_libraries(TennisDuel PRIVATE Qt6::Multimedia)

qt_add_resources(TennisDuel "game_assets"
    PREFIX "/"
    FILES
        assets/court_hard.svg
        assets/female1.png
        assets/female2.png
        assets/female3.png
        assets/female4.png
        assets/female5.png
        assets/male1.png
        assets/male2.png
        assets/male3.png
        assets/male4.png
        assets/male5.png
        assets/menu_logo.png
        assets/select_button_double.png
        assets/select_button_help.png
        assets/select_button_single.png
        assets/select_button_shop_ui.png
        assets/select_button_exit_ui.png
        assets/shop_title_1.png
        assets/racket_pink.png
        assets/racket_black.png
        assets/racket_yellow.png
        assets/racket_blue.png
        assets/racket_green.png
        assets/select_coin_bar.png
        assets/help_page.jpg
        assets/upbeat_tennis_loop.ogg
)

target_include_directories(TennisDuel PRIVATE src)

if(MSVC)
    target_compile_options(TennisDuel PRIVATE /utf-8)
endif()
