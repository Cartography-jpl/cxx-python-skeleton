# A project using the installed skeleton package (see run.cmake).
cmake_minimum_required(VERSION 3.27)
project(skeleton_consumer LANGUAGES CXX)
find_package(skeleton REQUIRED)
add_executable(consumer main.cc)
target_link_libraries(consumer PRIVATE skeleton::skeleton)
