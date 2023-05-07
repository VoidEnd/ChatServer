#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include <muduo/net/TcpServer.h>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
using namespace std;
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

class chatservice
{
public:
    //创建一个单例对象的接口函数
    static chatservice *instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //获取某个业务对应的处理方式
    MsgHandler gethandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //处理客户端ctrl+c退出
    void reset();
    //redis的回调函数
    void handleRedisSubscribeMesaage(int id,string message);

private:
    chatservice();
    //用一个unordered_map来实现业务与业务处理器的一一映射关系
    unordered_map<int, MsgHandler> _msgHandlermap;

    //记录用户的通信连接（因为用户登录有多个线程，有可能同一时间有多个线程同时操作_userconnmap表，所以需要考虑线程安全问题）
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //用一把互斥锁保证_userconnmap表的线程安全
    mutex _ConnMutex;

    UserModel _usermodel;

    offlinemsgmodel _offlineMsgModel;

    FriendModel _friendmodel;

    GroupModel _groupmodel;

    Redis _redis;
};

#endif