#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include<string>
#include<vector>
using namespace std;

//对offlinemessage数据表的操作
class offlinemsgmodel{
public:

    //插入用户的离线数据
    void insert(int userid,string msg);

    //删除用户的离线数据
    void remove(int userid);

    //查询用户的离线数据
    vector<string>query(int userid);
};
#endif