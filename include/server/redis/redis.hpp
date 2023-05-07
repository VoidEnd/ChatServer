#ifndef REDIS_H
#define REDIS_H
#include <string>
#include <hiredis/hiredis.h>
#include <functional>
#include <thread>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    //连接redis
    bool connect();

    //向指定通道发布消息
    bool publish(int channel, string message);

    //订阅某个通道的消息
    bool subscribe(int channel);

    //取消订阅某个通道的消息
    bool unsubscribe(int channel);

    //接收订阅通道的消息
    void observer_channel_message();

    //向业务层上报通知消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    redisContext *_publish_context;                      //专门用于发布消息的上下文
    redisContext *_subscribe_context;                    //专门用于订阅消息的上下文，因为订阅后就阻塞了，所以需要两个上下文
    function<void(int, string)> _notify_message_handler; //回调函数，收到消息上报给业务层
};

#endif