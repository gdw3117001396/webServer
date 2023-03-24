/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse(); // 初始化http响应信息
    ~HttpResponse(); 
    // 初始化资源的路径，资源的目录，是否长连接，响应状态码，内存映射相关
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff); //把http响应信息封装进writeBuff_中
    void UnmapFile();  // 解除内存映射
    char* File();   // 返回文件指针
    size_t FileLen() const;  // 返回文件长度
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const { return code_; } // 返回响应状态码

private:
    void AddStateLine_(Buffer &buff); // 添加响应首行
    void AddHeader_(Buffer &buff);   // 添加响应头
    void AddContent_(Buffer &buff);  // // 添加文件映射，也是在添加响应头Content-length:字段

    void ErrorHtml_();  // 看看有没有错误码，就有添加错误码的资源路径
    std::string GetFileType_();  // 获取当前文件的类型

    int code_;      // 响应状态码
    bool isKeepAlive_;  //  是否保持连接

    std::string path_;    // 资源的路径
    std::string srcDir_;  // 资源的目录
    
    char* mmFile_;   // 内存映射的文件
    struct stat mmFileStat_;  // 文件的状态信息

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  // 后缀 - 类型
    static const std::unordered_map<int, std::string> CODE_STATUS;          //  状态码 - 描述
    static const std::unordered_map<int, std::string> CODE_PATH;            // 状态米 - 路径
};


#endif //HTTP_RESPONSE_H