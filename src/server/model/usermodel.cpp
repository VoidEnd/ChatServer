#include "usermodel.hpp"
#include "db.hpp" //数据库基础操作（connect update query）
#include <iostream>
using namespace std;

//向数据库表中插入一条新的用户信息
bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    //拼接mysql语句
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')", user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySQL mysql; //创建一个MySQL类的对象
    if (mysql.connect())
    { //连接到数据库
        if (mysql.update(sql))
        {                                                       //利用之前拼接的sql语句，将新用户数据插入表中
            user.setId(mysql_insert_id(mysql.getconnection())); //获取数据库自动生成的userid，并将这个userid设置给该user对象
            return true;
        }
    }
    return false;
}

//根据用户id查找用户信息
User UserModel::query(int id)
{
    char sql[1024] = {0};
    //拼接mysql语句
    sprintf(sql, "select * from user where id=%d", id);
    MySQL mysql;
    if (mysql.connect())
    {
        //连接上数据库
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            //查找到了用户信息
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                return user;
            }
        }
    }
    //查找失败
    return User();
}

//更新数据库中user的状态信息
bool UserModel::updatestate(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state='%s' where id='%d'", user.getState().c_str(), user.getId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

//重置用户状态信息
    void UserModel::resetstate(){
        char sql[1024]={0};
        sprintf(sql,"update user set state='offline' where state='online'");
        MySQL mysql;
        if(mysql.connect()){
            mysql.update(sql);
        }
    }
