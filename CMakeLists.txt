cmake_minimum_required(VERSION 3.15)
project(FingerPrint)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)

add_executable(FingerPrint FingerPrint.cpp main.cpp)
target_link_libraries(FingerPrint PRIVATE libpqxx::pqxx)