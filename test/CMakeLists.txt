cmake_minimum_required(VERSION 3.0)
project(demo)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(server muduo_server.cpp)

target_link_libraries(server muduo_net muduo_base pthread)