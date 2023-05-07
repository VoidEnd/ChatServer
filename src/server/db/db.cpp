#include"db.hpp"
#include<muduo/base/Logging.h>

static string server="127.0.0.1";
static string user="root";
static string password="123456";
static string dbname="chat";

MySQL::MySQL(){
   MySQL::_conn=mysql_init(nullptr);
}
MySQL::~MySQL(){
    if(_conn!=nullptr){
        mysql_close(_conn);
    }
}
//连接mysql
bool MySQL::connect(){
    MYSQL* p=mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0);
    if(p!=nullptr){
        //在代码上支持中文
        mysql_query(_conn,"set name gbk");
        LOG_INFO<<"mysql connect success!";
    }
    else{
        LOG_INFO<<"mysql connect fail!";
    }
    return p;
}
//更新操作
bool MySQL::update(string sql){
    if(mysql_query(_conn,sql.c_str())){
        //返回0 查询成功，返回1查询失败 
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"更新失败！";
        return false;
    }
    return true;
}
//查询操作
MYSQL_RES* MySQL::query(string sql){
    if(mysql_query(_conn,sql.c_str())){
        //返回0 查询成功，返回1查询失败
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"查询失败！";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
MYSQL* MySQL::getconnection(){
    return _conn;
}