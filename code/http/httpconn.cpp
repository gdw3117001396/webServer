/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 
#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    Close(); 
};
// 初始化http连接,重置writeBuff_和readBuff_缓冲区
void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}
// 关闭连接
void HttpConn::Close() {
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}
// 获取fd_

int HttpConn::GetFd() const {
    return fd_;
};
//获取addr_
struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}
//
const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}
// 获取端口号
int HttpConn::GetPort() const {
    return addr_.sin_port;
}

// 从（请求）缓冲区读数据，读到readBuff_中
ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        // 从（请求）缓冲区读数据，读到readBuff_中
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

// 将http响应信息写入到响应缓冲区，给客户端
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        // 表示iov_[0]这块缓冲区的内容传完了，但是iov_[1]这块缓冲区传了一些华美传完
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            // 清空iov_[0]，避免下次会再传
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            // 表示连iov_[0]都没有传完，只传了一些
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while(isET || ToWriteBytes() > 10240);
    return len;
}

// 处理业务逻辑，这个时候数据已经写入readBuff_里面了
bool HttpConn::process() {
    // 初始化request_对象
    request_.Init();
    // 判断readBuff_是否有数据可读
    if(readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    // 用request来解析readBuff_中的数据
    else if(request_.parse(readBuff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        //解析完后就开始初始化封装response了
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(srcDir, request_.path(), false, 400);
    }
    // 生成的相应数据都在这,都保存再writeBuff_里面
    response_.MakeResponse(writeBuff_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    // 这个时候响应头和响应数据封装好了，但是还没有写回给客户端
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}
