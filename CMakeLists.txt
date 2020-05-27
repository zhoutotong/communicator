# author:>---zhoutotong

cmake_minimum_required(VERSION 3.10)    #cmake 最低版本要求，低于 3.10 构建过程会被终止。

PROJECT(comm)                   #定义工程名称

# 打开C++14支持
set(CMAKE_BUILE_TYPE DEBUG)            
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(current_dir ${CMAKE_CURRENT_SOURCE_DIR})

# 添加查找目录
set(CMAKE_INCLUDE_PATH)
set(CMAKE_LIBRARY_PATH)

# 指定编译选项
set(CMAKE_C_FLAGS_DEBUG "-g -Wall")

# 指定头文件搜索目录
INCLUDE_DIRECTORIES(
./
./src
./src/include
)

# 子目录
add_subdirectory(src)

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} 
communicator
)
