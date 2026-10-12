cmake_minimum_required(VERSION 3.21)
project(Stratovise VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

qt_add_executable(stratovise WIN32
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/qemubackend.h
    src/qemubackend.cpp
)

target_link_libraries(stratovise PRIVATE Qt6::Widgets)
target_include_directories(stratovise PRIVATE src)
if(MSVC)
    target_compile_options(stratovise PRIVATE /W4 /permissive-)
else()
    target_compile_options(stratovise PRIVATE -Wall -Wextra -Wpedantic)
endif()
install(TARGETS stratovise RUNTIME DESTINATION .)
