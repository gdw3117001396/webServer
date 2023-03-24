/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn(); // 主要是将fd_初始化为-1，表示这个是未连接的

    ~HttpConn();  // 

    void init(int sockFd, const sockaddr_in& addr);  // 初始化http连接

    ssize_t read(int* saveErrno);  // 从（请求）缓冲区读数据，读到readBuff_中

    ssize_t write(int* saveErrno);  // 将http响应信息写入到响应缓冲区，给客户端

    void Close(); // 关闭连接，关闭http响应的内存映射和关闭fd_

    int GetFd() const; // 获取fd_

    int GetPort() const; // 获取端口号

    const char* GetIP() const; // 获取ip地址,字符串类型的
    
    sockaddr_in GetAddr() const; // 获取addr_
    
    bool process();
    // 需要写的字节数
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;                   // 是否是ET模式
    static const char* srcDir;          // 资源的目录
    static std::atomic<int> userCount;  // 总共的客户端的连接数
    
private:
   
    int fd_;  // 客户端的文件描述符
    struct  sockaddr_in addr_;  // 客户端ip地址和端口号

    bool isClose_;  // 是否关闭
    
    int iovCnt_;   // 表示iov_[2]中有几个缓冲区是有东西的
    struct iovec iov_[2]; // 两个缓冲区用于writev集中写
    
    Buffer readBuff_; // 读（请求）缓冲区，保存请求数据的内容
    Buffer writeBuff_; // 写（响应）缓冲区，保存响应数据的内容

    HttpRequest request_;  // 处理http请求
    HttpResponse response_; // 处理http响应
};


#endif //HTTP_CONN_H