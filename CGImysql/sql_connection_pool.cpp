#include "sql_connection_pool.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>

connection_pool::connection_pool()
{
    m_CurConn = 0;
    m_FreeConn = 0;
}

connection_pool::~connection_pool()
{
    DestroyPool();
}

void connection_pool::init(std::string url, std::string User, std::string PassWord, std::string DatabaseName, int Port, int MaxConn, int close_log)
{
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DatabaseName;
    m_close_log = close_log;

    for(int i = 0; i < MaxConn; i++) 
    {
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con == nullptr) {
            LOG_ERROR("MySQL Error : mysql_init");
            exit(1);
        }

        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DatabaseName.c_str(), Port, NULL, 0);

        if(con == nullptr) {
            LOG_ERROR("MySQL Error : mysql_real_connect");
            exit(1);
        }

        connList.push_back(con);
        m_FreeConn++;
    }

    reserve = sem(m_FreeConn);

    m_MaxConn = m_FreeConn;
}

//获取数据库连接
//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
    MYSQL *conn = nullptr;
    if(connList.size() == 0) {
        return nullptr;
    }

    ////取出连接，信号量原子减1，为0则等待
    reserve.wait();
    lock.lock();

    conn = connList.front();
    connList.pop_front();

    ++m_CurConn;
    --m_FreeConn;

    lock.unlock();
    return conn;
}

//释放数据库连接
bool connection_pool::ReleaseConnection(MYSQL *conn) 
{
    if(conn == nullptr) return false;
    
    //对资源上锁
    lock.lock();

    connList.push_back(conn);
    --m_CurConn;
    ++m_FreeConn;

    lock.unlock();
    //释放连接，原子量+1；
    reserve.post();
    return true;
}

//获取当前空闲连接数量
int connection_pool::GetFreeConn()
{
    return this->m_FreeConn;
}

//销毁连接池
void connection_pool::DestroyPool()
{
    ////对资源上锁
    lock.lock();

    if(connList.size() > 0) {
        //这地方用了auto，到时候测试的时候看看会不会有问题
        for(auto &conn : connList) {
            mysql_close(conn);
        }

        m_CurConn = 0;
        m_FreeConn = 0;

        //清空list
        connList.clear();
    }
    lock.unlock();
}

//不直接调用获取和释放连接的接口，将其封装起来，通过RAII机制进行获取和释放。
connectionRAII::connectionRAII(MYSQL **con, connection_pool *connPool)
{
    *con = connPool->GetConnection();
    conRAII = *con;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
}