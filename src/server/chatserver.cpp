#include "chatserver.hpp"
#include "chatservice.hpp"
#include<string>
#include<json.hpp>
#include <functional>

using namespace std;
using namespace placeholders;
using json= nlohmann::json;

chatserver::chatserver(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, "ChatServer"), _loop(loop)
{
    _server.setConnectionCallback(std::bind(&chatserver::onConnection, this, _1));
    _server.setMessageCallback(std::bind(&chatserver::onMessage, this, _1, _2, _3));
    _server.setThreadNum(4);
}
void chatserver::start()
{
    _server.start();
}


void chatserver::onConnection(const TcpConnectionPtr &conn){
    if(!conn->connected()){
        //连接失败
        //处理客户端异常退出
        chatservice::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
void chatserver::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time){
    string buf=buffer->retrieveAllAsString();
    //反序列化，解析收到的信息
    json js=json::parse(buf);

    auto msghandler=chatservice::instance()->gethandler(js["msgid"].get<int>());
    msghandler(conn,js,time);
}