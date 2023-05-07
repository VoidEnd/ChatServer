#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"

//对user类的操作类
class UserModel{
public:
    //在user表中新插入一行信息
    bool insert(User &user);

    //在user表中根据用户id查找用户信息
    User query(int id);

    //更新数据表中user信息
    bool updatestate(User user);

    //重置用户状态信息
    void resetstate();
};
#endif
