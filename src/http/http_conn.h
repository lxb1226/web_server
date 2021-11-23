
/**
*  @DATE: 2021/11/21 14:19
*  @author: heyjude
*  @email: 1944303766@qq.com
*/
//
// Created by heyjude on 2021/11/21.
//

#ifndef WEB_SERVER_HTTP_CONN_H
#define WEB_SERVER_HTTP_CONN_H

#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>   // stat
#include <sys/mman.h>   // mmap
#include <unistd.h> // close
#include <cerrno>   // errno
#include <cstdarg> // va_start
#include <sys/epoll.h> // EPOLLIN

#include <unordered_map>

#include "../logger.h"

using namespace std;

#define READ_BUFFER_SIZE 4096   // 读缓冲区大小
#define WRITE_BUFFER_SIZE 2048  // 写缓冲区大小


// 解析HTTP请求状态
enum class CHECK_STATE : int {
    CHECK_REQUEST_LINE = 0,
    CHECK_HEADERS,
    CHECK_CONTENT
};

// HTTP1.1请求方法，目前仅支持GET
enum class METHOD : int {
    GET = 0,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH,
    UNKNOWN,
};

// 服务器处理HTTP请求时的可能结果
enum class HTTP_CODE : int {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
};

// 行的读取状态
enum class LINE_STATUS : int {
    LINE_OK = 0,
    LINE_OPEN,
    LINE_BAD
};


class http_conn {
public:
    http_conn() = default;

    ~http_conn() = default;

    static int m_epollfd;   // http_conn所属的epollfd
    static int m_user_count;    // 一共有多少个http_conn
    static const int FILENAME_LEN = 200; // 文件名的最大长度

public:
    // 初始化新接收的连接
    void init(int sockfd, sockaddr_in &addr);

    // 非阻塞读操作
    bool read();

    // 非阻塞写操作
    bool write();

    // 关闭连接
    void close_conn(bool real_close);

    // 处理客户请求
    void process();

private:
    // 初始化连接
    void init();


private:
    int m_sockfd;   // 读HTTP连接的socket和对方的socket地址
    sockaddr_in m_sockaddr;


    char m_read_buf[READ_BUFFER_SIZE];  // 读缓冲区
    int m_read_idx; // 标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置
    int m_checked_idx;  // 当前正在分析的字符在读缓冲区中的位置
    int m_start_line;   // 当前正在解析的行的起始位置

    CHECK_STATE m_check_state;  // 主状态机当前所处的状态
    METHOD m_method;    // 请求方法，目前仅支持GET
    char *m_url;    // 客户请求的目标文件的文件名
    char *m_version;    // HTTP协议版本号 目前仅支持HTTP/1.1
    char *m_host;   // 主机名
    bool m_linger;  // HTTP请求是否要求保持连接
    int m_content_length;   // HTTP请求的消息体的长度
    char *m_content_type; // 表示content_type

    char m_real_file[FILENAME_LEN]; // 客户请求的目标文件的完整路径，其内容等于doc_root + m_url,doc_root是网站根目录
    char *m_file_address;   // 客户请求的目标文件被mmap到内存中的起始位置
    // 采用writev来执行写操作，需要以下两个参数
    struct iovec m_iv[2];
    int m_iv_count;

    struct stat m_file_stat;    // 目标文件的状态。通过它可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息。

    char m_write_buf[WRITE_BUFFER_SIZE];    // 写缓冲区
    int m_write_idx;    // 写缓冲区中待发送的字节数

    std::unordered_map<string, string> m_posts; // post请求体



private:
    HTTP_CODE process_read();   // 解析HTTP请求
    LINE_STATUS parse_line();   // 解析行
    HTTP_CODE parse_request_line(char *text);   // 解析请求行
    HTTP_CODE parse_headers(char *text);    // 解析请求头
    HTTP_CODE parse_content(char *text);    // 解析内容体
    HTTP_CODE do_request(); // 进行请求
    char *get_line() { return m_read_buf + m_start_line; };

    bool process_write(HTTP_CODE ret);  // 填充HTTP应答

    void unmap();   // 取消映射
    bool add_response(const char *format, ...); // 往写缓冲区添加指定格式的内容
    bool add_content(const char *content);  // 填充内容体
    bool add_headers(int content_len);  // 填充消息头
    bool add_status_line(int status, const char *title);    // 填充状态行
    bool add_content_length(int content_length);    // 填充内容长度
    bool add_linger();  // 填充Connection状态
    bool add_blank_line();  // 填充空行
    void parse_post(char *text);    // 解析post请求
    void parse_from_url_encoded(char *text);

    bool user_verify(string &username, string &password, bool isLogin);
};


#endif //WEB_SERVER_HTTP_CONN_H
