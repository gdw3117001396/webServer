/*
 * @Author       : mark
 * @Date         : 2020-06-26
 * @copyleft Apache 2.0
 */ 
#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

// 还可以读的数据
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}
// 还可以写的字节数
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}
// 前面还可以拓展的字节数，就是已经读了的但还在缓冲区
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 读到了哪一个字符，从readPos_下标的那个字符开始读
const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}
// 将读指针往后移动
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}
// 将读指针往后移动到end位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek());
}
// 重置缓冲区
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}
// 返回还没有读完的数据，然后再重置缓冲区
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}
// 写到了哪一个字符，从writePos_下标的那个字符地址开始写，const版本
const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}
// 写到了哪一个字符，从writePos_下标的那个字符地址开始写
char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

//又写了Len长度的字符，将writePos后移
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
} 

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}
//给缓冲区添加内容的函数
// Append(buff, len - writable)  buff临时数组， len - writable是临时数组中的字符个数
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    // 将临时数据拷贝到可以开始写的地方
    std::copy(str, str + len, BeginWrite());
    // 修改writePos
    HasWritten(len);
}
// 未用到,将buff还没有读完的数据拷贝给当前的buffer_
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 确保可以写，先看能写的位置够不够,不够就要获取新空间
void Buffer::EnsureWriteable(size_t len) {
    // WritableBytes()后这里会返回0，然后就要获取新空间了
    if(WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}
// 真正的读取数据,把数据放到buffer_缓冲区当中
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535];  // 临时的数组，保证能够把所有的数据都读出来
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    // readv总是会先充满一个缓冲区再往下一个写
    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    }
    else {
        // 缓冲区已经读满了，就要把临时buff里面的数据拷贝进buffer中
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}
// 未用到，相当于把缓冲区的数据写给fd，也就是相当于缓冲区前面那些可重用了
ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

// 新增长空间
void Buffer::MakeSpace_(size_t len) {
    // 如果加上前面可拓展的空间还是不够，就要扩展缓冲区了
    if(WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } 
    else {
        size_t readable = ReadableBytes();
        // 数据分为三段 0- readPos_ - writePos_ , readPos_ - writePos_这一段是还没有读的，而0 - readPos_这一段是已经读完了，可以给覆盖了
        // 这样就可以把临时的buff数据放到后面空余的位置了，因为此时位置必然是够的
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}