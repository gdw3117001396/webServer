/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    // 初始化日志系统
    void init(int level, const char* path = "./log", 
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    static Log* Instance();  // 单例模式
    static void FlushLogThread(); // 将日志线程原来还剩的内容写入文件中

    void write(int level, const char *format,...); // 日志写操作
    void flush();  // 刷新缓冲区 

    int GetLevel(); // 获取日志系统等级
    void SetLevel(int level); // 设置日志系统等级
    bool IsOpen() { return isOpen_; }  // 日志系统是否打开
    
private:
    Log(); // 初始化一些变量
    void AppendLogLevelTitle_(int level); // 添加日志等级题目
    virtual ~Log();  // 处理关闭写线程，关闭文件描述符等操作
    void AsyncWrite_();   // 异步写

private:
    static const int LOG_PATH_LEN = 256; // 日志路径名长度
    static const int LOG_NAME_LEN = 256; // 日志文件名长度
    static const int MAX_LINES = 50000;  // 日志内容的最大行

    const char* path_;      // 路径
    const char* suffix_; // 后缀名

    int MAX_LINES_;   // 没用到

    int lineCount_;  // 写了多少行了
    int toDay_;     // 记录日期

    bool isOpen_;   // 日志系统是否打开
 
    Buffer buff_;    // 用缓冲区来写
    int level_;     // 日志等级
    bool isAsync_;  // 是否异步

    FILE* fp_;   // 文件指针
    std::unique_ptr<BlockDeque<std::string>> deque_; // 指向日志内容的指针
    std::unique_ptr<std::thread> writeThread_;  // 开启子线程去写
    std::mutex mtx_; // 锁
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H