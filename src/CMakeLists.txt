cmake_minimum_required(VERSION 3.10)

project( plane_sweep )

find_package( OpenCV REQUIRED )

find_package(OpenMP)

add_executable( depth_fusion depth_fusion.cpp util.cpp )

target_link_libraries(depth_fusion PUBLIC OpenMP::OpenMP_CXX ${OpenCV_LIBS} )
