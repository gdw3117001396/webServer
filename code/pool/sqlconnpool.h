/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    // 单例模式，获取数据库连接池
    static SqlConnPool *Instance(); 
    // 获取一个连接
    MYSQL *GetConn();
    // 释放一个连接，放回池子
    void FreeConn(MYSQL * conn);
    // 返回可用的连接数量
    int GetFreeConnCount();
    //初始化数据库连接池，初始化信号量也就是最大连接数
    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    //关闭数据库连接池
    void ClosePool();

private:
    SqlConnPool();   // 初始化useCount_和freeCount_
    ~SqlConnPool();

    int MAX_CONN_;   // 最大的连接数
    int useCount_;   //  当前的用户数
    int freeCount_;  //  空闲的用户数，没用上

    std::queue<MYSQL *> connQue_;  // 队列(MSQL *)
    std::mutex mtx_;   // 互斥锁
    sem_t semId_;      // 信号量
};


#endif // SQLCONNPOOL_H