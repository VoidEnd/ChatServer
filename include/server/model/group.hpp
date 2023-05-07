#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <vector>
#include <string>
using namespace std;
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    //获取群组信息接口
    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser>& getUsers() { return this->users; }

    //设置群组信息接口
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

private:
    int id;                     //组id
    string name;                //组名
    string desc;                //组描述
    vector<GroupUser> users;    //组内成员
};

#endif