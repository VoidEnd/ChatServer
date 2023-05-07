#ifndef DB_H
#define DB_H
#include<mysql/mysql.h>
#include<string>

using namespace std;

class MySQL{
public:
    MySQL();
    ~MySQL();
    //连接数据库
    bool connect();
    //数据库更新
    bool update(string sql);
    //数据库查找
    MYSQL_RES* query(string sql);
    MYSQL* getconnection();
private:
   MYSQL *_conn;
};
#endif