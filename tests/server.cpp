//
// Created by heyjude on 2021/10/21.
//

#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "logger.h"

#define BUF_SIZE 4096
#define MAX_EVENT_NUMBER 10

enum class CHECK_STATE : int {
    CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER
};
enum class LINE_STATUS : int {
    LINE_OK = 0,
    LINE_BAD,
    LINE_OPEN
};

enum class HTTP_CODE : int {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    FORBIDDEN_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};
static const char *szret[] = {"I get a correct request\n", "something wrong\n"};

/**
 * 解析一行，即在buffer中找到\r\n，如果找到了则返回LINE_OK,
 * @param buffer
 * @param checked_index 指向当前buffer中正在解析的字节
 * @param read_index    指向buffer中数据的尾部的下一字节
 * @return
 */
LINE_STATUS parse_line(char *buffer, int &checked_index, int &read_index) {
    LOG_DEBUG("parse line : %s", buffer);
    char temp;
    for (; checked_index < read_index; ++checked_index) {
        temp = buffer[checked_index];
        if (temp == '\r') {
            if (checked_index == read_index - 1) {
                return LINE_STATUS::LINE_OPEN;
            } else if (buffer[checked_index + 1] == '\n') {
                // 说明解析到了一行
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;

        } else if (temp == '\n') {
            if ((checked_index > 1) && buffer[checked_index - 1] == '\r') {
                buffer[checked_index - 1] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
    }

    return LINE_STATUS::LINE_OPEN;
}

/**
 * 解析请求行 暂时只处理GET请求
 * 请求行格式: 请求方法 空格 URL 空格 协议版本\r\n
 * GET /welcome.html HTTP/1.1
 * @param temp
 * @param state
 * @return
 */
HTTP_CODE parse_request_line(char *temp, CHECK_STATE &state) {
    LOG_DEBUG("%s", temp);
    char *url = strpbrk(temp, " \t");
    if (!url) {
        return HTTP_CODE::BAD_REQUEST;
    }
    *url++ = '\0';
    char *method = temp;
    if (strcasecmp(method, "GET") == 0) {
        printf("the request method is GET\n");
    } else {
        return HTTP_CODE::BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char *version = strpbrk(url, " \t");
    if (!version) {
        return HTTP_CODE::BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    if (strcasecmp(version, "HTTP/1.1") != 0) {
        return HTTP_CODE::BAD_REQUEST;
    }

    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }

    if (!url || url[0] != '/') {
        return HTTP_CODE::BAD_REQUEST;
    }

    printf("The request url is : %s\n", url);
    state = CHECK_STATE::CHECK_STATE_HEADER;
    return HTTP_CODE::NO_REQUEST;
}

/**
 * 解析请求头
 * @param temp
 * @return
 */
HTTP_CODE parse_headers(char *temp) {
    LOG_DEBUG("%s", temp);
    if (temp[0] == '\0') {
        return HTTP_CODE::GET_REQUEST;
    } else if (strncasecmp(temp, "Host:", 5) == 0) {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is : %s\n", temp);
    } else {
        printf("I can not handle this header\n");
    }
    return HTTP_CODE::NO_REQUEST;
}

HTTP_CODE parse_content(char *buffer, int &checked_index, CHECK_STATE &check_state, int &read_index, int &start_line) {
    LINE_STATUS line_status = LINE_STATUS::LINE_OK;
    HTTP_CODE ret_code = HTTP_CODE::NO_REQUEST;
    while ((line_status = parse_line(buffer, checked_index, read_index)) == LINE_STATUS::LINE_OK) {
        char *temp = buffer + start_line;
        start_line = checked_index;
        switch (check_state) {
            case CHECK_STATE::CHECK_STATE_REQUESTLINE: {
                ret_code = parse_request_line(temp, check_state);
                if (ret_code == HTTP_CODE::BAD_REQUEST) {
                    return HTTP_CODE::BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE::CHECK_STATE_HEADER: {
                ret_code = parse_headers(temp);
                if (ret_code == HTTP_CODE::BAD_REQUEST) {
                    return HTTP_CODE::BAD_REQUEST;
                } else if (ret_code == HTTP_CODE::GET_REQUEST) {
                    return HTTP_CODE::GET_REQUEST;
                }
                break;
            }
            default: {
                return HTTP_CODE::INTERNAL_ERROR;
            }
        }
    }
    if (line_status == LINE_STATUS::LINE_OPEN) {
        return HTTP_CODE::NO_REQUEST;
    } else {
        return HTTP_CODE::BAD_REQUEST;
    }
}

int set_nonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void add_fd(int epoll_fd, int fd, bool enable_et) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    set_nonblocking(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage : %s ip port", basename(argv[0]));
        return 0;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    int ret = bind(listen_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    assert(ret != -1);

    ret = listen(listen_fd, 5);
    assert(ret != -1);
    // 使用io多路复用
    epoll_event events[MAX_EVENT_NUMBER];
    int epoll_fd = epoll_create(1024);
    assert(epoll_fd != -1);
    add_fd(epoll_fd, listen_fd, true);

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER + 1, -1);
        if (n < 0) {
            // 错误
            printf("epoll failure\n");
            break;
        } else {
            for (int i = 0; i < n; ++i) {
                int sock_fd = events[i].data.fd;
                if (sock_fd == listen_fd) {
                    // 接收到了连接
                    struct sockaddr_in client_address;
                    socklen_t client_addr_len;
                    int conn_fd = accept(sock_fd, (struct sockaddr *) &client_address, &client_addr_len);
                    if (conn_fd < 0) {
                        printf("连接失败\n");
                    } else {
                        add_fd(epoll_fd, conn_fd, true);    // true表示启用et模式
                    }

                } else if (events[i].events & EPOLLIN) {
                    // 读事件
                    // 解析数据
                    int data_read = 0;
                    int read_index = 0;
                    int checked_index = 0;
                    int start_line = 0;
                    CHECK_STATE check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
                    char buf[BUF_SIZE];
                    memset(buf, '\0', BUF_SIZE);
                    // et模式,这里的代码只触发依次，因此需要一次性读完
                    while (true) {
                        data_read = recv(sock_fd, buf + read_index, BUF_SIZE - read_index, 0);
                        if (data_read < 0) {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {

                                LOG_DEBUG("reading later\n");
                                break;
                            }
                            LOG_DEBUG("reading failed\n");
                            close(sock_fd);
                            break;
                        } else if (data_read == 0) {

                            printf("remote client has closed the connection\n");
                            break;
                        } else {
                            // 解析数据
                            LOG_DEBUG("got %d bytes of data : '%s'\n", data_read, buf);
                            read_index += data_read;
                            HTTP_CODE status_code = parse_content(buf, checked_index, check_state, read_index,
                                                                  start_line);
                            if (status_code == HTTP_CODE::GET_REQUEST) {
                                // 得到了请求
                                send(sock_fd, szret[0], strlen(szret[0]), 0);
                                break;
                            } else if (status_code == HTTP_CODE::NO_REQUEST) {
                                // 没有得到请求
                                continue;
                            } else {
                                send(sock_fd, szret[1], strlen(szret[1]), 0);
                                break;
                            }
                        }
                    }
                    close(sock_fd);
                } else if (events[i].events & EPOLLOUT) {
                    // 写事件
                    printf("写事件触发");
                } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    // 关闭连接事件
                    close(sock_fd);
                } else {
                    LOG_ERROR("Unexcepted event");
                }
            }
        }
    }
    close(listen_fd);
    return 0;
}
