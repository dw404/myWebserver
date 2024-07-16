#ifndef _CONNECTION_POOL_
#define _CONEECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

class connection_pool
{
private:
    int m_MaxConn; //最大连接数
    int m_CurConn; //当前已使用的连接数
    int m_FreeConn;//当前空闲的连接数
    locker lock;
    std::list<MYSQL *> connList; // 连接池
    sem reserve;//信号量记录可用资源

    connection_pool();
    ~connection_pool();

public:
    std::string m_url;   //主机地址
    std::string m_Port;  //数据库端口号
    std::string m_User;  //数据库登录用户名
    std::string m_PassWord;//数据库登录密码
    std::string m_DatabaseName;//使用的数据库名
    int m_close_log;    //日志开关

    MYSQL *GetConnection();//获取数据库连接
    bool ReleaseConnection(MYSQL *conn);//释放数据库连接
    int GetFreeConn();      //获取当前空闲连接
    void DestroyPool();     //销毁连接池

    //单例模式
    static connection_pool *GetInstance() {
        static connection_pool connPool;
        return &connPool;
    }

    void init(std::string url, std::string User, std::string PassWord, std::string DatabaseName, int Port, int MaxConn, int close_log);

};

//这个类的唯一作用就是与数据库连接池的资源进行绑定；
//可以看到这个类中只有构造函数和析构函数。
class connectionRAII
{
private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
public:
    connectionRAII(MYSQL **con, connection_pool *connPool);
    ~connectionRAII();
};
#endif