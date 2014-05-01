cmake_minimum_required(VERSION 2.8)
Project(OpenNI2_wrapper CXX)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") 

find_package(PkgConfig)
pkg_check_modules(OPENCV REQUIRED opencv)
include_directories( ${OPENCV_INCLUDE_DIRS} )

find_path(OPENNI_INCLUDE_DIR NAMES OpenNI.h)
include_directories(${OPENNI2_INCLUDE})

add_library(openni2_wrapper SHARED openni2_wrapper.cpp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libopenni2_wrapper.so DESTINATION /usr/local/lib/)
