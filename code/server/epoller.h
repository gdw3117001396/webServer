/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    // 初始化epollFd_, 和events检测到的事件集合
    explicit Epoller(int maxEvent = 1024);
    //关闭epollFd_
    ~Epoller();
    // 往epollFd_添加fd事件为events
    bool AddFd(int fd, uint32_t events);
    // 修改fd
    bool ModFd(int fd, uint32_t events);
    // 删除fd
    bool DelFd(int fd);
    // 调用epoll_wait
    int Wait(int timeoutMs = -1);
    // 根据索引获取事件集合中对应下标的fd
    int GetEventFd(size_t i) const;
    // 根据索引获取事件集合中对应下标的events
    uint32_t GetEvents(size_t i) const;
        
private:
    int epollFd_;  // epoll_create()创建一个epoll对象，返回值就是epollFd

    std::vector<struct epoll_event> events_;    // 检测到的事件集合
};

#endif //EPOLLER_H