#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <vector>
#include <string>
using namespace std;

class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);

    //加入群组
    void addGroup(int userid, int groupid, string role);

    //显示用户加入的所有群组
    vector<Group> queryGroups(int userid);

    //显示群组的所有组员（除自己以外，用于发送消息）
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif