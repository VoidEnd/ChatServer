#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class chatserver{
public:
    chatserver(EventLoop* loop,
                const InetAddress &listenaddr,
                const string &name)
                :_server(loop,listenaddr,name),_loop(loop){
                    _server.setConnectionCallback(std::bind(&chatserver::onconnection,this,_1));
                    _server.setMessageCallback(std::bind(&chatserver::onmessage,this,_1,_2,_3));
                    _server.setThreadNum(4);
                }
    void start(){
        _server.start();
    }
private:
    //处理连接和断开的回调函数
    void onconnection(const TcpConnectionPtr & conn){
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"state:online"<<endl;
        }
        else{
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"state:offline"<<endl;
        }
    }
    //处理读写的回调函数
    void onmessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp time){
        string buf=buffer->retrieveAllAsString();
        cout<<"recv data:"<<buf<<"time:"<<time.toFormattedString()<<endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop * _loop;

};
int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1",6000);
    chatserver server(&loop,addr,"chatserver");
    server.start();
    loop.loop();
    return 0;
}