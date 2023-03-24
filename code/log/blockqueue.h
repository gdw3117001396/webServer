/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);  // 初始化capacity大小

    ~BlockDeque();

    void clear();  // 清空队列

    bool empty(); //看队列是否为空

    bool full();  // 看队列是否满了

    void Close();  // 通过将isClose设为true,释放拥有条件变量的线程

    size_t size(); // 获取队列长度

    size_t capacity(); // 获取队列容量

    T front(); // 获取队头

    T back(); // 获取队尾

    void push_back(const T &item); // 获取队尾

    void push_front(const T &item); // 获取队尾

    bool pop(T &item); // 从队头取出一个任务

    bool pop(T &item, int timeout); // 从队头取出一个任务，计时阻塞，没有用到
    
    void flush();  // 唤醒一个消费者，唤醒一个线程处理写日志

private:
    std::deque<T> deq_;   // 日志内容

    size_t capacity_;   // 队列容量

    std::mutex mtx_;    // 锁

    bool isClose_;     //  是否关闭

    std::condition_variable condConsumer_;   // 消费者条件变量

    std::condition_variable condProducer_;   //  生产者条件变量
};

// 初始化capacity大小
template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);
    isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
};
// 通过将isClose设为true,释放拥有条件变量的线程
template<class T>
void BlockDeque<T>::Close() {
    {   
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
};

// 唤醒一个生产者，表示有空间可以继续放入队列了
template<class T>
void BlockDeque<T>::flush() {
    condConsumer_.notify_one();
};
// 清空队列
template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}
// 获取队头
template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}
// 获取队尾
template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}
// 获取队列长度
template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}
// 获取队列容量
template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}
// 将任务放入队尾
template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 超过了容量就要阻塞等待
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    // 呼叫消费者可以拿了
    condConsumer_.notify_one();
}
// 将任务放入队头
template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 超过了容量就要阻塞等待
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    // 呼叫消费者可以拿了
    condConsumer_.notify_one();
}
// 看队列是否为空
template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}
// 看队列是否满了
template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}
//从队头取出一个任务
template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        condConsumer_.wait(locker);
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    // 唤醒生产者可以继续生产
    condProducer_.notify_one();
    return true;
}
//从队头取出一个任务，计时阻塞，没有用到
template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

#endif // BLOCKQUEUE_H