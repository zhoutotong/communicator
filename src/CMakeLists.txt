#Author:      zhoutotong
cmake_minimum_required(VERSION 3.2)

# 设置模块名称
set(MODULE_NAME communicator)

# 开启 C++14支持
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 指定头文件搜索目录
INCLUDE_DIRECTORIES(
./
./include
)

# 子目录
add_subdirectory(utilities)

#4.source directory，源文件目录
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
AUX_SOURCE_DIRECTORY(./direct_trans DIRECT_TRANS_SRCS)

# 添加全部文件
SET(ALL_SRC
${DIR_SRCS}
${DIRECT_TRANS_SRCS}
)

add_library(${MODULE_NAME} SHARED ${ALL_SRC})

TARGET_LINK_LIBRARIES(${MODULE_NAME}
pthread
utilities
)