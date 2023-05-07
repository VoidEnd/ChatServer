#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

//记录当前登录用户信息
User g_currentUser;
//记录当前登录用户好友信息
vector<User> g_currentUserfriendList;
//记录当前登录用户群组信息
vector<Group> g_currentUsergroupList;
//是否成功进入用户主菜单
bool isMainMenuRunning = false;
// 登陆是否成功
atomic_bool g_isLoginSuccess{false};
//用于读写线程通信的信号量
sem_t rwsem;

//主聊天页面
void mainMenu(int);
//显示当前用户基本信息
void showCurrentUserData();
//获取当前时间
string getCurrentTime();
// 接收线程
void readTaskHandler(int clientfd);

//连接服务器与显示注册、登录主菜单
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example:./chatclient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        // client端socket创建错误
        cerr << "client socket create error" << endl;
        exit(-1);
    }

    //客户端connect到服务器
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip); //将IP地址由string转为int
    // inet_pton(AF_INET,ip,&server.sin_addr.s_addr);
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        //连接失败
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    //初始化读写线程的信号量
    sem_init(&rwsem, 0, 0);

    //连接服务器成功，创建子线程，用于接收来自服务器的响应
    std::thread readTask(readTaskHandler, clientfd); // 创建子线程
    readTask.detach();                               // 设置线程分离

    //主菜单界面
    while (1)
    {
        //主菜单
        cout << "===============================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "===============================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); //读取输入choice后的回车，避免读取错误

        switch (choice)
        {
        case 1:
        {
            //登录业务

            //用户输入登录id和密码
            int id = 0;
            char password[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();
            cout << "userpassword:";
            cin.getline(password, 50); //用getline只有到回车才结束

            //组装json
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = password;
            string request = js.dump();
            g_isLoginSuccess = false;

            //发送登陆消息
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login message error:" << request << endl;
            }
            sem_wait(&rwsem); //等待信号量，服务器处理完登陆操作通知
            if (g_isLoginSuccess)
            {
                //登录成功进入用户主菜单
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2:
        {
            //注册业务
            char name[50] = {0};
            char password[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "password:";
            cin.getline(password, 50);

            //组装json
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = password;
            string request = js.dump();

            //发送注册消息
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send register message error:" << request << endl;
            }
            sem_wait(&rwsem); //等待信号量，服务器注册完成之后会通知
        }
        break;
        case 3:

            //退出业务
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);

        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
    return 0;
}

//在主菜单界面选择注册请求，服务器注册完之后把注册结果响应给客户端
//处理注册响应
void doRegResponse(json &regresponse)
{
    if (regresponse["errno"].get<int>() != 0)
    {
        //注册失败
        cerr << "name is already exist,please change a name" << endl;
    }
    else
    {
        //注册成功
        cout << "name register success, userid is " << regresponse["id"] << ", do not forget it!" << endl;
    }
}

//在主菜单界面选择登录请求，服务器登录完之后把登录结果，包括好友列表、群组、离线消息响应给客户端
//处理登录响应
void doLoginResponse(json &loginresponse)
{
    if (loginresponse["errno"].get<int>() != 0)
    {
        //登陆失败
        cerr << loginresponse["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else
    {
        //登陆成功

        //记录当前用户id和name
        g_currentUser.setId(loginresponse["id"].get<int>());
        g_currentUser.setName(loginresponse["name"]);

        //将该用户好友解析出来

        //初始化
        g_currentUserfriendList.clear();

        if (loginresponse.contains("friends"))
        {
            vector<string> vec = loginresponse["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserfriendList.push_back(user);
            }
        }

        //将该用户群组和群组内成员信息解析出来

        //初始化，防止loginout后紧接着login拉两份grouplist
        g_currentUsergroupList.clear();

        if (loginresponse.contains("groups"))
        {
            //解析该用户所加入的群组
            vector<string> vec1 = loginresponse["groups"];
            for (string &str : vec1)
            {
                json js = json::parse(str);
                Group group;
                group.setId(js["id"]);
                group.setName(js["groupname"]);
                group.setDesc(js["groupdesc"]);

                //解析该群组所包含的用户
                vector<string> vec2 = js["users"];
                for (string &userstr : vec2)
                {
                    json js1 = json::parse(userstr);
                    GroupUser grpuser;
                    grpuser.setId(js1["id"].get<int>());
                    grpuser.setName(js1["name"]);
                    grpuser.setState(js1["state"]);
                    grpuser.setrole(js1["role"]);

                    group.getUsers().push_back(grpuser);
                }
                g_currentUsergroupList.push_back(group);
            }
        }

        //显示当前用户基本信息
        showCurrentUserData();

        //显示当前用户离线信息
        if (loginresponse.contains("offlinemsg"))
        {
            vector<string> vec = loginresponse["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);

                if (js["msgid"].get<int>() == ONE_CHAT_MSG)
                {
                    //点对点离线消息 time + [id] + name + " said: " + xxx
                    cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    //群组聊天离线消息 group message [groupid] time [id] name said:xxx
                    cout << "group message [" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

//接收子线程，接收服务器发过来的信息
void readTaskHandler(int clientfd)
{
    while (1)
    {
        //循环读取数据
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == 0 || len == -1)
        { //断开连接或者出错了
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            //在线点对点聊天
            cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == GROUP_CHAT_MSG)
        {
            //在线群组聊天
            cout << "group message [" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == REG_MSG_ACK)
        {
            //注册响应
            doRegResponse(js); //根据服务器传来的json判断注册是否成功
            sem_post(&rwsem);  //用信号告诉主线程，注册结果处理完成
            continue;
        }
        if (msgtype == LOGIN_MSG_ACK)
        {
            //登录响应
            doLoginResponse(js); //处理登录响应，包括好友列表，群组列表，离线信息
            sem_post(&rwsem);    //用信号告诉主线程，登录结果处理完成
            continue;
        }
    }
}

//显示当前用户基本信息
void showCurrentUserData()
{
    cout << "===================Login User===================" << endl;
    cout << "current login user id=>" << g_currentUser.getId() << "  name=>" << g_currentUser.getName() << endl;
    cout << "------------------Friends List------------------" << endl;
    if (!g_currentUserfriendList.empty())
    {
        for (User &user : g_currentUserfriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }

    cout << "-------------------Groups List-------------------" << endl;
    if (!g_currentUsergroupList.empty())
    {
        for (Group &group : g_currentUsergroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &groupuser : group.getUsers())
            {
                cout << "  " << groupuser.getId() << " " << groupuser.getName() << " " << groupuser.getState() << " " << groupuser.getrole() << endl;
            }
        }
    }

    cout << "================================================" << endl;
}

//客户端支持指令的处理函数
void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

//客户端支持的命令列表
unordered_map<string, string> commandmap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群组聊天,格式groupchat:groupid:message"},
    {"loginout", "注销,格式loginout"}};

//客户端的命令与处理函数
unordered_map<string, function<void(int, string)>> commandhandlerMap = { // int为clientfd,string为用户输入的命令
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

//用户主菜单页面，用户通过输入指令完成特定功能
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; //存储命令
        int index = commandbuf.find(":");
        if (index == -1)
        {
            //没找到，说明是help，loginout,或者错误
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, index);
        }
        auto it = commandhandlerMap.find(command);
        if (it == commandhandlerMap.end())
        {
            cerr << "invalid input command,please input again!" << endl;
            continue;
        }
        it->second(clientfd, commandbuf.substr(index + 1, commandbuf.size() - index));
    }
}

// help处理器
void help(int fd, string str)
{
    cout << "show command list" << endl;
    for (auto &p : commandmap)
    {
        cout << p.first << " " << p.second << endl;
    }
    cout << endl;
}

// chat处理器
void chat(int clientfd, string str)
{
    // friendid:message
    int index = str.find(":");
    if (index == -1)
    {
        //没找到"："
        cerr << "chat command invalid -> " << str << endl;
        return;
    }
    int friendid = atoi((str.substr(0, index)).c_str());
    string message = str.substr(index + 1, str.size() - index);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat message error -> " << buffer << endl;
    }
}

// addfriend处理器
void addfriend(int clientfd, string str)
{
    // friendid
    int friendid = atoi(str.c_str());
    int userid = g_currentUser.getId();
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = userid;
    js["friendid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addfriend message error -> " << buffer << endl;
    }
}

// creategroup处理器 groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int index = str.find(":");
    if (index == -1)
    {
        cerr << "creategroup command invalid -> " << str << endl;
        return;
    }
    string groupname = str.substr(0, index);
    string groupdesc = str.substr(index + 1, str.size() - index);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send creategroup message error -> " << buffer << endl;
    }
}

// addgroup处理器 groupid
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup message error -> " << buffer << endl;
    }
}

// groupchat处理器 groupid:message
void groupchat(int clientfd, string str)
{
    int index = str.find(":");
    if (index == -1)
    {
        cerr << "groupchat command invalid -> " << str << endl;
        return;
    }
    int groupid = atoi(str.substr(0, index).c_str());
    string message = str.substr(index + 1, str.size() - index);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupchat message error -> " << buffer << endl;
    }
}

// loginout处理器
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send loginout message error -> " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false; //关闭用户主菜单
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}