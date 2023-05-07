#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

//连接redis
bool Redis::connect()
{
    //用于发布和订阅的上下文分别连接redis
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "redis connect error" << endl;
        return false;
    }
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        cerr << "redis connect error" << endl;
        return false;
    }

    //设置独立线程，用于监听特定通道的事件，若有消息，上报给业务层
    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis-server success" << endl;
    return true;
}

//向指定通道发布消息
bool Redis::publish(int channel, string message)
{
    // redisCommand三步：1.将命令缓存到本地(redisAppendCommand) 2.将命令发送到redis server(redisBufferWrite) 3.阻塞等待命令的执行结果
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "publish %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed" << endl;
        return false;
    }
    freeReplyObject(reply); // reply为动态生成的结构体，使用完要释放掉
    return true;
}

//订阅某个通道的消息
bool Redis::subscribe(int channel)
{
    //由于subscribe本身阻塞，所以只需要：1.将命令缓存到本地(redisAppendCommand) 2.将命令发送到redis server(redisBufferWrite)
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "subscribe %d", channel))
    {
        cerr << "subscribe command failed" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed" << endl;
            return false;
        }
    }
    return true;
}

//取消订阅某个通道的消息
bool Redis::unsubscribe(int channel)
{
    //与subscribe一样只需要前两步
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "unsubscribe %d"), channel)
    {
        cerr << "unsubscribe command failed" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed" << endl;
            return false;
        }
    }
    return true;
}

//在独立线程中接收订阅通道的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    //循环等待通道上有消息到来
    while ( redisGetReply(this->_subscribe_context, (void **)&reply) ==REDIS_OK )
    {
        // reply->element[0]="message"  reply->element[1]=通道号 reply->element[2]=具体内容
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            //将id号和消息通知业务层
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>>observer_channel_message quit<<<<<<<<<<<<<<" << endl;
}

//向业务层上报通知消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler=fn;
}