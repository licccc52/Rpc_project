# 设置cmake的最低版本要求和项目名称
cmake_minimum_required(VERSION 3.0)
project(mprpc)

# 设置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置项目可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置项目库文件的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置项目编译头文件的搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/src/include/tcpproject)
include_directories(${PROJECT_SOURCE_DIR}/example)

# 设置项目库文件的搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

# src包含了mprpc框架的所有相关代码
add_subdirectory(src)
# example包含了mprpc框架的使用示例代码
add_subdirectory(example)