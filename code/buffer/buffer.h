/*
 * @Author       : mark
 * @Date         : 2020-06-26
 * @copyleft Apache 2.0
 */ 

#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>
class Buffer {
public:
    Buffer(int initBuffSize = 1024);  // 初始化一开始可以装字符的数量，默认1024
    ~Buffer() = default;

    size_t WritableBytes() const;   // 还可以写的字节数
          
    size_t ReadableBytes() const ; // 还可以读的字节数
    
    size_t PrependableBytes() const; // 前面还可以拓展的字节数，就是已经读了的但还在缓冲区

    const char* Peek() const;       // 读到了哪一个字符，从readPos_下标的那个字符开始读
    void EnsureWriteable(size_t len);  // 确保可以写，先看能写的位置够不够,不够就要获取新空间
    void HasWritten(size_t len);  // 又写了Len长度的字符，将writePos后移

    void Retrieve(size_t len);  // 将读指针往后移动len,表示读取了len个字节
    void RetrieveUntil(const char* end); // 将读指针往后移动到end位置

    void RetrieveAll() ;     // 重置缓冲区
    std::string RetrieveAllToStr();  // 返回还没有读完的数据，然后再重置缓冲区

    const char* BeginWriteConst() const;  // 写到了哪一个字符，从writePos_下标的那个字符地址开始写，const版本
    char* BeginWrite();  // 写到了哪一个字符，从writePos_下标的那个字符地址开始写

    void Append(const std::string& str);
    void Append(const char* str, size_t len); //给缓冲区添加内容的函数
    void Append(const void* data, size_t len); 
    void Append(const Buffer& buff);  // 未用到,将buff还没有读完的数据拷贝给当前的buffer_

    ssize_t ReadFd(int fd, int* Errno);  // 真正的读取数据,把数据放到buffer_缓冲区当中
    ssize_t WriteFd(int fd, int* Errno); // 未用到

private:
    char* BeginPtr_(); //返回buffer_首字符的地址
    const char* BeginPtr_() const; //返回buffer_首字符的地址，const版本
    void MakeSpace_(size_t len);  // 自动增长新空间,先看覆盖前面已经读了的够不够，不够再扩展空间

    std::vector<char> buffer_;   // 具体装数据的vector
    std::atomic<std::size_t> readPos_;  // 读的位置，std::atomic保证是原子操作的，相当于系统在操作该变量时给他上了一个锁
    std::atomic<std::size_t> writePos_; // 写的位置
};

#endif //BUFFER_H