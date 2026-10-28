cmake_minimum_required(VERSION 3.16)
project(adas_fcw LANGUAGES CXX)

find_package(Threads REQUIRED)
find_path(OPENCV_INCLUDE_DIR NAMES opencv2/core.hpp PATH_SUFFIXES opencv4)
find_library(OPENCV_CORE_LIBRARY NAMES opencv_core)
find_library(OPENCV_VIDEOIO_LIBRARY NAMES opencv_videoio)
if(NOT OPENCV_INCLUDE_DIR OR NOT OPENCV_CORE_LIBRARY OR
   NOT OPENCV_VIDEOIO_LIBRARY)
  message(FATAL_ERROR "OpenCV core and videoio development files are required")
endif()

add_library(fcw_foundation INTERFACE)
target_compile_features(fcw_foundation INTERFACE cxx_std_17)
target_include_directories(fcw_foundation INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${OPENCV_INCLUDE_DIR}"
)
target_link_libraries(fcw_foundation INTERFACE
  Threads::Threads "${OPENCV_VIDEOIO_LIBRARY}" "${OPENCV_CORE_LIBRARY}"
)

add_library(fcw_capture STATIC src/capture/video_capture.cpp)
target_link_libraries(fcw_capture PUBLIC fcw_foundation)
target_compile_options(fcw_capture PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>
)
