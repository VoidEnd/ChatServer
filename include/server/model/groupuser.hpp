#ifndef GROUPUSER_H
#define GROUPUSER_H
#include "user.hpp"

// groupuser相比于user多一个role
class GroupUser : public User
{
public:
    void setrole(string role)
    {
        this->role = role;
    }
    string getrole()
    {
        return this->role;
    }

private:
    string role;
};

#endif