cmake_minimum_required(VERSION 3.5)

project(camera LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

find_package(Qt5 COMPONENTS Xml Concurrent Widgets REQUIRED)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(CAMERA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/camera)
set(LIBRARIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)
# src
file(GLOB SRC_FILES 
    ${SRC_DIR}/*.h
    ${SRC_DIR}/*.hpp
    ${SRC_DIR}/*.cpp
    ${SRC_DIR}/*.ui)
# test
file(GLOB TEST_FILES
    ${CAMERA_DIR}/usbhotplug.h
    ${CAMERA_DIR}/usbhotplug.cpp
    ${TEST_DIR}/*.h
    ${TEST_DIR}/*.hpp
    ${TEST_DIR}/*.cpp)
# camera
file(GLOB CAMERA_FILES
    ${CAMERA_DIR}/*.h
    ${CAMERA_DIR}/*.hpp
    ${CAMERA_DIR}/*.cpp)
list(APPEND SRC_FILES ${CAMERA_FILES})
# opencv
set(OpenCV_DIR ${LIBRARIES_DIR}/opencv47/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# libyuv
include_directories(${LIBRARIES_DIR}/libyuv/include)
include_directories(${LIBRARIES_DIR}/libjpeg/include)
set(LIBYUV_LIBS
    ${LIBRARIES_DIR}/libyuv/lib/libyuv.a
    ${LIBRARIES_DIR}/libjpeg/lib/libjpeg.a)
# ncnn
set(NCNN_DIR ${LIBRARIES_DIR}/ncnn)
include_directories(${NCNN_DIR}/include)
set(NCNN_STATIC ${NCNN_DIR}/lib/libncnn.a -fopenmp)
# app
add_executable(camera ${SRC_FILES})
target_link_libraries(camera PRIVATE
    Qt5::Widgets
    Qt5::Xml
    Qt5::Concurrent
    ${OpenCV_LIBS}
    ${LIBYUV_LIBS}
    ${NCNN_STATIC})
# test
add_executable(test ${TEST_FILES})
