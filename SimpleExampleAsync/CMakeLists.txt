project(SimpleAsioAsyncExample)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} SimpleExampleAsync.cpp)

target_include_directories(
    ${PROJECT_NAME} 
    PUBLIC 
      ../asio/asio/include
)

target_link_libraries(${PROJECT_NAME} pthread)
