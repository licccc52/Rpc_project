# aux_source_directory(. SRC_LIST)
set(SRC_LIST mprpcapplication.cc mprpcconfig.cc rpcheader.pb.cc rpcprovider.cc mprpcchannel.cc mprpccontroller.cc logger.cc zookeeperutil.cc)

add_library(mprpc ${SRC_LIST}) # 生成mprpc库
target_link_libraries(mprpc libtcpepoll.a pthread zookeeper_mt)#多线程版本的库