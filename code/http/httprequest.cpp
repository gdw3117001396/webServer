/*
 * @Author       : mark
 * @Date         : 2020-06-26
 * @copyleft Apache 2.0
 */ 
#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

// 用有限状态机解析读（请求）缓冲区的数据
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";  // 判断结束的标识符
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && state_ != FINISH) {
        // 获取一行数据，根据\r\n为结束标志，就是在还未读的地方开始，然后到写完的地方，查找\r\n这个标志
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd); // 获取读到的一行数据
        switch(state_)
        {
        case REQUEST_LINE:
            // 解析请求行
            if(!ParseRequestLine_(line)) {
                return false;
            }
            // 成功了就会解析请求地址
            ParsePath_();
            break;    
        case HEADERS:
            // 解析请求头
            ParseHeader_(line);
            // 表示没有请求体了，可以直接结束
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            // 解析请求体
            ParseBody_(line);
            break;
        default:
            break;
        }
        // 行的末尾和开始写的指针一样的时候就表示已经读完了
        if(lineEnd == buff.BeginWrite()) { break; }
        // 将读指针readPos_往后移动，移动到\r\n后面那个位置
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

// 解析请求路径
void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

// 用正则表达式解析请求首行
bool HttpRequest::ParseRequestLine_(const string& line) {
    // GET / HTTP/1.1
    // 一个()表示要匹配的一个东西
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1]; // 请求方法:get,post等
        path_ = subMatch[2]; // url
        version_ = subMatch[3]; // 请求版本HTTP/1.1
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}
// 用正则表达式解析请求头
void HttpRequest::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = BODY;
    }
}

// 解析请求体
void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    // 处理Post请求
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}
// 处理Post请求
void HttpRequest::ParsePost_() {
    // 查看是否是表单提交的，如果是的话，就会是"application/x-www-form-urlencoded"类型
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        //解析表单信息
        ParseFromUrlencoded_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            // 根据url中是register还是login来判断是登录还是注册
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                // 验证用户
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}
// 解析表单数据
void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    // username=hello&password=hello
    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            // 输入中文的时候就会出现
            // 简单的加密的操作，编码
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    // 最后一对表单数据
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// 用户验证
bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    // 从连接池中获取一个连接
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);
    
    //是否验证成功
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    // 注册情况下会直接成功
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    // 拼接一下sql语句
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    // 执行sql语句，成功返回0，继续往下走，不成功返回非0直接释放res返回
    if(mysql_query(sql, order)) { 
        // 如果不成功了释放结果
        mysql_free_result(res);
        return false; 
    }
    // 保存结果
    res = mysql_store_result(sql);
    //返回结果集中的列的数目  
    j = mysql_num_fields(res);
    // 返回一个所有字段结构的数组。
    fields = mysql_fetch_fields(res);

    // 从结果集合中取得下一行
    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        /* 登录行为*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        }
        // 注册情况下也要看用户名是否会重复 
        else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    // TODO:把数据库连接放回连接池，感觉应该不用，不然RAII没有意义，应该是自动释放的
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}
// 获取请求路径
std::string HttpRequest::path() const{
    return path_;
}
// 获取请求路径
std::string& HttpRequest::path(){
    return path_;
}
// 获取请求方法
std::string HttpRequest::method() const {
    return method_;
}
// 获取请求版本
std::string HttpRequest::version() const {
    return version_;
}
// 查找post提交的表单数据对应键值的value
std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
// 查找post提交的表单数据对应键值的value
std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}