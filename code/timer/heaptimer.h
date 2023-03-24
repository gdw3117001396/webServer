/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack; // 超时回调函数
typedef std::chrono::high_resolution_clock Clock;  // 时钟
typedef std::chrono::milliseconds MS;  // 毫秒
typedef Clock::time_point TimeStamp;  // 表示时钟当前值的，具体时间

struct TimerNode {
    int id;     //  文件描述符
    TimeStamp expires;  // 超时时间
    TimeoutCallBack cb;  // 回调函数
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};
class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }  // 初始化小根堆大小是64

    ~HeapTimer() { clear(); }   
    
    void adjust(int id, int newExpires);  // 调整对应文件描述符定时器的超时时间

    void add(int id, int timeOut, const TimeoutCallBack& cb);  // 添加一个定时器（分已有和没有）

    void doWork(int id);  // 触发回调函数，并删除指定id结点

    void clear();  // 清空定时器内容

    void tick();   // 处理超时节点

    void pop();    // 就是删除根

    int GetNextTick();  // 还有多久下一个就会超时

private:
    // 删除
    void del_(size_t i);
    // 上溯
    void siftup_(size_t i);
    // 下移
    bool siftdown_(size_t index, size_t n);
    // 交换两个节点
    void SwapNode_(size_t i, size_t j);
    // 小根堆容器
    std::vector<TimerNode> heap_;
    // 文件描述符和小根堆索引的对应关系
    std::unordered_map<int, size_t> ref_;
};

#endif //HEAP_TIMER_H