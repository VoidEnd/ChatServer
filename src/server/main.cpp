#include "chatserver.hpp"
#include <iostream>
#include <signal.h>
#include <chatservice.hpp>
using namespace std;

void resethandler(int){
    chatservice::instance()->reset();
    exit(0);
}

int main(int argc,char **argv)
{
    if(argc<3){
        cerr<<"command invalid! example:./chatserver 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip=argv[1];
    uint16_t port=atoi(argv[2]);
    //用于处理ctrl+C非正常退出
    signal(SIGINT,resethandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    chatserver server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();

    return 0;
}