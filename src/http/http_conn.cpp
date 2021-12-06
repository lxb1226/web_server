
/**
 *  @DATE: 2021/11/21 14:19
 *  @author: heyjude
 *  @email: 1944303766@qq.com
 */
//
// Created by heyjude on 2021/11/21.
//

#include "http_conn.h"

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

const char *doc_root = "/home/heyjude/workspace/projects/web_server/html";

// 定义HTTP响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested line.\n";

static constexpr int convert_hex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}

// 设置fd为非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将fd添加到epoll中
void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 将fd从epoll中移除
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 修改fd的事件
void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_STATUS::LINE_OK;
    HTTP_CODE ret = HTTP_CODE::NO_REQUEST;
    char *text;
    while (((m_check_state == CHECK_STATE::CHECK_CONTENT) && (line_status == LINE_STATUS::LINE_OK)) ||
           ((line_status = parse_line()) == LINE_STATUS::LINE_OK))
    {
        text = get_line();
        m_start_line = m_checked_idx;
        //        printf("got 1 http line: %s\n", text);
        // LOG_INFO("got 1 http line : %s\n", text);
        switch (m_check_state)
        {
        case CHECK_STATE::CHECK_REQUEST_LINE:
        {
            ret = parse_request_line(text);
            if (ret == HTTP_CODE::BAD_REQUEST)
            {
                return HTTP_CODE::BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE::CHECK_HEADERS:
        {
            ret = parse_headers(text);
            if (ret == HTTP_CODE::BAD_REQUEST)
            {
                return HTTP_CODE::BAD_REQUEST;
            }
            else if (ret == HTTP_CODE::GET_REQUEST)
            {
                return do_request();
            }
            break;
        }
        case CHECK_STATE::CHECK_CONTENT:
        {
            ret = parse_content(text);
            if (ret == HTTP_CODE::GET_REQUEST)
            {
                return do_request();
            }
            line_status = LINE_STATUS::LINE_OPEN;
            break;
        }
        default:
        {
            return HTTP_CODE::INTERNAL_ERROR;
        }
        }
    }
    return HTTP_CODE::NO_REQUEST;
}

/**
 * 解析请求行
 * 请求行格式： GET http://www.baidu.com/welcome.html HTTP/1.1
 * @param text
 * @return
 */
HTTP_CODE http_conn::parse_request_line(char *text)
{
    m_url = strpbrk(text, " \t");
    if (!m_url)
    {
        return HTTP_CODE::BAD_REQUEST;
    }
    *m_url++ = '\0';

    char *method = text;
    if (strcasecmp(method, "GET") == 0)
    {
        m_method = METHOD::GET;
        // LOG_INFO("method: %s", method);
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        m_method = METHOD::POST;
        // LOG_INFO("method: %s", method);
    }
    else
    {
        // LOG_INFO("暂时不处理其他请求方法");
        return HTTP_CODE::BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
    {
        return HTTP_CODE::BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    // LOG_DEBUG("version : %s", m_version);
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        return HTTP_CODE::BAD_REQUEST;
    }
    //    if (strncasecmp(m_url, "http://", 7)) {
    //        m_url += 7;
    //        m_url = strchr(m_url, '/');
    //    }
    m_url = strchr(m_url, '/');
    // LOG_DEBUG("url : %s", m_url);

    if (!m_url || m_url[0] != '/')
    {
        return HTTP_CODE::BAD_REQUEST;
    }
    m_check_state = CHECK_STATE::CHECK_HEADERS;
    return HTTP_CODE::GET_REQUEST;
}

/**
 * 解析行
 * @return
 */
LINE_STATUS http_conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx; m_checked_idx++)
    {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r')
        {
            if (m_checked_idx == m_read_idx - 1)
            {
                return LINE_STATUS::LINE_OPEN;
            }
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
    }

    return LINE_STATUS::LINE_OPEN;
}

/**
 * 解析请求头：
 * 请求头格式如下：
 * Accept: image/jpeg, application/x-ms-applicaiton\r\n
 * Host: localhost:8088
 * Content-Length: 112
 * Connection:Keep-Alive
 * ...
 * 在这里只暂时处理host, content-length, linger
 * @param text
 * @return
 */
HTTP_CODE http_conn::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        // 说明已经没有headers了
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE::CHECK_CONTENT;
            return HTTP_CODE::NO_REQUEST;
        }
        return HTTP_CODE::GET_REQUEST;
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        // 匹配到了host
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
        LOG_DEBUG("host : %s", m_host);
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "Keep-alive") == 0)
        {
            m_linger = true;
        }
        LOG_DEBUG("Connection : %s", text);
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
        LOG_DEBUG("content_length : %d", m_content_length);
    }
    else if (strncasecmp(text, "Content-Type:", 13) == 0)
    {
        text += 13;
        text += strspn(text, " \t");
        m_content_type = text;
        LOG_DEBUG("content_type : %s", m_content_type);
    }
    else
    {
        printf("暂时不处理其他HTTP请求头部\n");
    }

    return HTTP_CODE::NO_REQUEST;
}

/**
 * 解析post请求
 */
void http_conn::parse_post(char *text)
{
    if (m_method == METHOD::POST && strcasecmp(m_content_type, "application/x-www-form-urlencoded") == 0)
    {
        parse_from_url_encoded(text);
        // 判断是登录还是注册，登录则保存用户名和密码到数据库（这里暂时是文件），然后返回index页面，
        // 注册则查询用户名和密码对不对，错误则返回错误页面，正确则返回index页面
        bool isLogin = false;
        if (strcasecmp(m_url, "/login.html") == 0)
        {
            // 登录操作
            isLogin = true;
        }
        if (user_verify(m_posts["username"], m_posts["password"], isLogin))
        {
            LOG_DEBUG("登录或注册成功");
        }
        else
        {
            LOG_DEBUG("登录或注册失败");
        }
    }
}

void http_conn::parse_from_url_encoded(char *text)
{
    int idx = m_checked_idx;
    //    string body = string(text);
    LOG_DEBUG("url_encode : %s", text);
    string key, value;
    char ch;
    int num = 0;
    int j = 0, i = 0;
    for (; i < m_content_length; ++i)
    {
        ch = text[i];
        switch (ch)
        {
        case '=':
        {
            key = string(text, j, i - j);
            j = i + 1;
            break;
        }
        case '+':
        {
            text[i] = ' ';
            break;
        }
        case '%':
        {
            num = convert_hex(text[i + 1]) % 16 + convert_hex(text[i + 2]);
            text[i + 1] = num % 10 + '0';
            text[i + 2] = num / 10 + '0';
            i += 2;
        }
        case '&':
        {
            value = string(text, j, i - j);
            j = i + 1;
            m_posts[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        }
        }
    }

    if (m_posts.count(key) == 0 && j < i)
    {
        value = string(text, j, i - j);
        m_posts[key] = value;
        LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
    }
}

/**
 * 处理HTTP请求的content报文体
 * 这里仅仅做一个验证，不做处理
 * @param text
 * @return
 */
HTTP_CODE http_conn::parse_content(char *text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        if (m_method == METHOD::POST && strcasecmp(m_content_type, "application/x-www-form-urlencoded") == 0)
        {
            // 解析POST请求体
            // TODO(heyjude):解析POST请求体以及用户验证
            // 这里仅仅只是模拟登录和注册
            parse_post(text);
        }
        text[m_content_length] = '\0';
        return HTTP_CODE::GET_REQUEST;
    }
    return HTTP_CODE::NO_REQUEST;
}

// 当解析完一个HTTP请求时，就可以获取到一个网址，这个时候需要检查它的属性并将其映射到内存中
HTTP_CODE http_conn::do_request()
{
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    LOG_DEBUG("real_file path : %s", m_real_file);
    // 判断文件是否存在
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        return HTTP_CODE::NO_RESOURCE;
    }
    // 判断是否有权限
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return HTTP_CODE::FORBIDDEN_REQUEST;
    }
    // 判断是否是文件而不是目录
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return HTTP_CODE::BAD_REQUEST;
    }

    int fd = open(m_real_file, O_RDONLY); // 只读
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return HTTP_CODE::FILE_REQUEST;
}

// 非阻塞读
bool http_conn::read()
{

    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }

    int bytes_read = 0;
    while (true)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

// 分散写
bool http_conn::write()
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_to_send = m_write_idx;
    int bytes_have_send = 0;
    int len = 0;
    if (bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }

    while (true)
    {
        len = writev(m_sockfd, m_iv, m_iv_count);
        if (len <= -1)
        {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件。虽然在此期间，服务器无法立即接收到同一个客户的下一个请求，但这可以保证连接的完整性
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        bytes_to_send -= len;
        bytes_have_send += len;
        if (bytes_have_send == 0)
        {
            // 发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否立即关闭连接
            unmap();
            if (m_linger)
            {
                init();
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
            else
            {
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return false;
            }
        }
    }
}

// 根据解析的HTTP请求来进行处理
bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case HTTP_CODE::BAD_REQUEST:
    {
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    }
    case HTTP_CODE::FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
        {
            return false;
        }
        break;
    }
    case HTTP_CODE::INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    }
    case HTTP_CODE::NO_RESOURCE:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    }
    case HTTP_CODE::FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
            {
                return false;
            }
        }
    }
    default:
    {
        return false;
    }
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    return true;
}

void http_conn::unmap()
{
    if (m_file_address != nullptr)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = nullptr;
    }
}

bool http_conn::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list args;
    va_start(args, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx - 1, format, args);
    if (len >= WRITE_BUFFER_SIZE - m_write_idx - 1)
    {
        return false;
    }
    m_write_idx += len;
    va_end(args);
    return true;
}

bool http_conn::add_content(const char *content)
{

    return add_response("%s", content);
}

bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) & add_linger() & add_blank_line();
}

bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_content_length(int content_length)
{
    return add_response("Content-Length: %d\r\n", content_length);
}

bool http_conn::add_linger()
{
    return add_response("Connection: %s\r\n", m_linger ? "keep-alive" : "close");
}

bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

void http_conn::init(int sockfd, sockaddr_in &addr)
{
    m_sockfd = sockfd;
    m_sockaddr = addr;

    // 以下是为了避免TIME_WAIT状态，仅用于调试
    //    int reuse = 1;
    //    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, sockfd, true);

    init();
    m_user_count++;
}

void http_conn::close_conn(bool real_close)
{
    if (real_close && (m_sockfd != -1))
    {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

void http_conn::init()
{
    m_read_idx = 0;
    m_checked_idx = 0;
    m_start_line = 0;

    m_check_state = CHECK_STATE::CHECK_REQUEST_LINE;
    m_method = METHOD::GET;
    m_url = nullptr;
    m_version = nullptr;
    m_host = nullptr;
    m_content_type = nullptr;
    m_linger = false;
    m_content_length = 0;

    m_file_address = nullptr;
    m_write_idx = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if (read_ret == HTTP_CODE::NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn(true);
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

bool http_conn::user_verify(string &username, string &password, bool isLogin)
{
    // 直接模拟登录注册
    if (username == "" || password == "")
    {
        return false;
    }
    else if (username == "username" && password == "password")
    {
        return true;
    }
    return false;
}
