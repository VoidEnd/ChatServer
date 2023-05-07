#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include <vector>
#include "user.hpp"
using namespace std;

//对friend表进行操作
class FriendModel
{
public:
    //添加好友
    void insert(int userid, int friendid);

    //显示用户好友列表
    vector<User> query(int userid);
};

#endif