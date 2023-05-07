#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType{
    LOGIN_MSG=1,     //登录
    LOGIN_MSG_ACK,   //登录响应
    LOGINOUT_MSG,    //注销该用户
    REG_MSG,         //注册
    REG_MSG_ACK,     //注册响应

    ONE_CHAT_MSG,    //一对一聊天消息
    ADD_FRIEND_MSG,  //添加好友信息

    CREATE_GROUP_MSG,  //创建群组聊天
    ADD_GROUP_MSG,     //加入群组聊天
    GROUP_CHAT_MSG     //发送群组消息
};
#endif