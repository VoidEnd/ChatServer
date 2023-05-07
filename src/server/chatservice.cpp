#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

//创建一个单例对象的接口函数
chatservice *chatservice::instance()
{
    static chatservice service;
    return &service;
}

//在构造函数中，将业务类型与业务处理方法插入到unordered_map中，且将不同业务的回调函数与业务处理器进行绑定
chatservice::chatservice()
{
    _msgHandlermap.insert({LOGIN_MSG, std::bind(&chatservice::login, this, _1, _2, _3)});       //登录业务与处理器绑定
    _msgHandlermap.insert({REG_MSG, std::bind(&chatservice::reg, this, _1, _2, _3)});           //注册业务与处理器绑定
    _msgHandlermap.insert({LOGINOUT_MSG, std::bind(&chatservice::loginout, this, _1, _2, _3)}); //注销业务与处理器绑定

    _msgHandlermap.insert({ONE_CHAT_MSG, std::bind(&chatservice::oneChat, this, _1, _2, _3)});     //一对一聊天业务与处理器绑定
    _msgHandlermap.insert({ADD_FRIEND_MSG, std::bind(&chatservice::addFriend, this, _1, _2, _3)}); //添加好友业务与处理器绑定

    _msgHandlermap.insert({CREATE_GROUP_MSG, std::bind(&chatservice::createGroup, this, _1, _2, _3)}); //创建群组业务与处理器绑定
    _msgHandlermap.insert({ADD_GROUP_MSG, std::bind(&chatservice::addGroup, this, _1, _2, _3)});       //加入群组业务与处理器绑定
    _msgHandlermap.insert({GROUP_CHAT_MSG, std::bind(&chatservice::groupChat, this, _1, _2, _3)});     //群组聊天与处理器绑定

    //连接redis
    if (_redis.connect())
    {
        //设置上报消息的回调函数
        _redis.init_notify_handler(std::bind(&chatservice::handleRedisSubscribeMesaage, this, _1, _2));
    }
}

//处理登录业务
void chatservice::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _usermodel.query(id);

    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            //用户已登录，不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "The user has logged in. Please re-enter the account and password!";
            conn->send(response.dump());
        }
        else
        {
            //验证成功

            //记录用户的通信连接
            {
                //用mutex保证线程安全，用{}保证锁的作用域小
                lock_guard<mutex> lock(_ConnMutex);
                _userConnMap.insert({id, conn});
            }

            //登陆成功后，向redis订阅本id的channel
            _redis.subscribe(id);

            //更新用户状态从offline变为online
            user.setState("online");
            _usermodel.updatestate(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            //查找用户的离线储存信息
            vector<string> vec;
            vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取离线消息后，把之前离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            //登录成功后显示用户的好友信息
            vector<User> uservec;
            uservec = _friendmodel.query(id);
            if (!uservec.empty())
            {
                vector<string> vec2;
                for (User &user : uservec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            //登陆成功后显示用户群组信息
            vector<Group> groupvec = _groupmodel.queryGroups(id);
            if (!groupvec.empty())
            {
                vector<string> vec3;
                for (Group &group : groupvec)
                {
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    vector<string> uservec;
                    for (GroupUser &user : group.getUsers())
                    {
                        json userjs;
                        userjs["id"] = user.getId();
                        userjs["name"] = user.getName();
                        userjs["state"] = user.getState();
                        userjs["role"] = user.getrole();
                        uservec.push_back(userjs.dump());
                    }
                    groupjs["users"] = uservec;
                    vec3.push_back(groupjs.dump());
                }
                response["groups"] = vec3;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        //该用户不存在，或者密码错误登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "ID or Password is invalid!";
        conn->send(response.dump());
    }
}

//处理注册业务
void chatservice::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _usermodel.insert(user);
    if (state)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

//获取某个业务（msgid）对应的处理方式
MsgHandler chatservice::gethandler(int msgid)
{
    auto it = _msgHandlermap.find(msgid);
    if (it == _msgHandlermap.end())
    {
        //没有找到相应消息类型的处理器
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << " can not find handler";
        };
    }
    return _msgHandlermap[msgid];
}

//处理注销业务
void chatservice::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_ConnMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(userid);
        }
    }

    //下线，从redis中取消订阅该id的channel
    _redis.unsubscribe(userid);
    User user(userid, "", "", "offline");
    _usermodel.updatestate(user);
}

//处理客户端异常退出
void chatservice::clientCloseException(const TcpConnectionPtr &conn)
{
    //遍历保存用户连接信息的_userConnMap,找到连接对应的用户id，将该链接从哈希表中删除，将该id的state置为offline
    User user;
    {
        lock_guard<mutex> lock(_ConnMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //下线，从redis中取消订阅该id的channel
    _redis.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        user.setState("offline");
        _usermodel.updatestate(user);
    }
}

//处理一对一聊天业务
void chatservice::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    /*msgid:
      id:
      name:
      toid:
      msg:   */
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_ConnMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            //接收的用户已上线，且连接在本服务器上，直接转发
            //因为在发送消息的时候对方也有可能会离线，所以需要加锁保护线程安全
            it->second->send(js.dump());
            return;
        }
    }

    //查找要发送的id，如果在线，说明在其他服务器登录，此时publish
    User user = _usermodel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    //接收的用户不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

//处理客户端ctrl+c退出
void chatservice::reset()
{
    //把状态为online的用户状态设为offline
    _usermodel.resetstate();
}

//处理添加好友业务 js中包含msgid id friendid
void chatservice::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //添加好友信息
    _friendmodel.insert(userid, friendid);
}

//创建群组业务
void chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    Group group(-1, groupname, groupdesc);
    if (_groupmodel.createGroup(group))
    {
        //群组创建好后将创建者信息加入groupuser表
        _groupmodel.addGroup(userid, group.getId(), "creator");
    }
}
//加入群组业务
void chatservice::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int groupid = js["groupid"].get<int>();
    int userid = js["id"].get<int>();
    _groupmodel.addGroup(userid, groupid, "normal");
}
//群组聊天业务
void chatservice::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridvec = _groupmodel.queryGroupUsers(userid, groupid);
    {
        lock_guard<mutex> lock(_ConnMutex);
        for (int id : useridvec)
        {
            auto it = _userConnMap.find(id);
            if (it != _userConnMap.end())
            {
                //对方在线，且在本服务器上登陆，直接转发
                it->second->send(js.dump());
            }
            else
            {
                //对方在线但不在本服务器上登录
                User user = _usermodel.query(id);
                if (user.getState() == "online")
                {
                    _redis.publish(id, js.dump());
                }
                //对方不在线，发离线消息
                else
                {
                    _offlineMsgModel.insert(id, js.dump());
                }
            }
        }
    }
}

void chatservice::handleRedisSubscribeMesaage(int id, string message){
    //从redis消息队列获取到本服务器订阅的消息
    {
        lock_guard<mutex>lock(_ConnMutex);
        auto it=_userConnMap.find(id);
        if(it!=_userConnMap.end()){
            //找到服务器上id对应的连接，服务器向该id转发message
            it->second->send(message);
            return;
        }
    }
    _offlineMsgModel.insert(id,message);
}