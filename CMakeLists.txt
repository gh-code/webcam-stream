# Copyright 2023 Gary Huang

cmake_minimum_required(VERSION 3.13)

project(webcam_stream)

find_package(Boost REQUIRED PATHS C:/boost)
find_package(OpenCV REQUIRED PATHS C:/opencv/OpenCV-MinGW-Build-OpenCV-4.5.5-x64
    core imgcodecs)

include_directories(include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
# include_directories(${GStreamer_INCLUDE_DIRS})

add_library(server SHARED src/server.cpp)
add_library(webcam STATIC src/webcam.cpp)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
if (WIN32)
    set (Socket_LIBS wsock32 ws2_32)
endif()

add_executable(webcam_stream
    main.cpp)

target_compile_options(server PRIVATE -Wa,-mbig-obj)

message(Boost_LIBS="${Boost_LIBS}")
message(Socket_LIBS="${Socket_LIBS}")
message(OpenCV_LIBS="${OpenCV_LIBS}")

target_link_libraries(server
    ${Boost_LIBS}
    ${Socket_LIBS}
)

target_link_libraries(webcam
    ${OpenCV_LIBS}
)

target_link_libraries(webcam_stream
    server
    ${OpenCV_LIBS}
)
