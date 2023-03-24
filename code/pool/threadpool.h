/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
public:
    // 防止构造函数会隐式转换
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);

            // 创建threadCount个子线程
            for(size_t i = 0; i < threadCount; i++) {
                // 传递的是一个lambam表达式，pool是一个临时变量使表达式里面可以使用pool_相当于函数传参了
                std::thread([pool = pool_] {  // 相当于创建了threadCount个pool指针，但是指向都是pool_也就是类似指针的值传递
                    // unique_lock使用了RAII技术，在构造函数就枷锁了,析构函数才解锁，确保工作队列的取出使互斥的
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) {
                        if(!pool->tasks.empty()) {
                            // 从任务队列中取一个任务
                            auto task = std::move(pool->tasks.front());
                            // 移除掉
                            pool->tasks.pop();
                            // 解锁开始执行任务
                            locker.unlock();
                            task();
                            // 重新加锁准备下一次取任务
                            locker.lock();
                        } 
                        else if(pool->isClosed){
                            break;
                        }
                        else{
                            // 当任务队列为空且不退出的时候，就会阻塞等待当有队列加入时候唤醒,调用wait的时候会给locker解锁
                            pool->cond.wait(locker);
                        }    
                    }
                }).detach(); // 线程分离
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        // 如果线程池不为空的话，才需要
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            // 唤醒所有线程关闭线程,让线程走到break那里去退出
            pool_->cond.notify_all();
        }
    }

    // 完美转发
    template<class F>
    void AddTask(F&& task) {
        {
            // 使用锁添加，此时不需要临时解锁所以用lock_guard更快
            std::lock_guard<std::mutex> locker(pool_->mtx);
            // 添加任务队列
            pool_->tasks.emplace(std::forward<F>(task));
        }
        // 唤醒一个睡眠的线程,表示当前有了新的工作队列
        pool_->cond.notify_one();
    }

private:
    // 结构体, 池子
    struct Pool {
        // C++11互斥锁
        std::mutex mtx;
        // C++11条件变量
        std::condition_variable cond;
        // 是否关闭
        bool isClosed;
        // 队列（保存的是任务）
        std::queue<std::function<void()>> tasks;
    };
    // 线程池
    std::shared_ptr<Pool> pool_;
};


#endif //THREADPOOL_H