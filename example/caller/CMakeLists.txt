# 将该目录下的全部源文件都添加到变量名中
# aux_source_directory(. SRC_LIST)
# aux_source_directory(../src PROTO_LIST)
# set(SRC_LIST calluserservice.cc ../user.pb.cc)
# set(SRC_LIST callfriendservice.cc ../friend.pb.cc)
set(SRC_LIST callRunningDataService.cc ../runningData.pb.cc)



# 生成可执行文件所需要的源文件
# add_executable(provider ${SRC_LIST} ${PROTO_LIST})

#生成可执行文件 provider
add_executable(consumer2 ${SRC_LIST})
# 需要链接的库
target_link_libraries(consumer2 mprpc protobuf) # 生成的provider动态库需要连接 mprpc库(还未生成, 在CMakeLists中找到相关的生成语句-> cmakelists.txt) 和 protobuf库(第三方已经准备好)