#把server中所有文件放进SRC_LIST
aux_source_directory(. SRC_LIST)

#生成可执行文件
add_executable(chatclient ${SRC_LIST})

#需要链接的库
target_link_libraries(chatclient pthread)