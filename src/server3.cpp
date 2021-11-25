/**
*  @DATE: 2021/11/8 22:37
*  @author: heyjude
*  @email: 1944303766@qq.com
*/
//
// Created by heyjude on 2021/11/8.
//
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <sys/epoll.h>
#include <csignal>

#include "http_conn.h"
#include "logger.h"
#include "thread_pool.h"
#include "lst_timer.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

extern int addfd(int epollfd, int fd, bool one_host);

extern int removefd(int epollfd, int fd);

void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const char *info) {
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

void deal_listen(http_conn *users, int listenfd, sort_timer_lst *timer, int timeoutMs) {
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
        return;
    }
    if (http_conn::m_user_count >= MAX_FD) {
        show_error(connfd, "Internal server busy");
        return;
    }
    // 初始化客户连接
    LOG_DEBUG("start init client. connfd: %d, ip: %s", connfd, inet_ntoa(client_address.sin_addr));
    users[connfd].init(connfd, client_address);
    // 添加定时结点
}

void deal_read(http_conn *client) {
    if (client->read()) {
        client->process();
    } else {
        client->close_conn(true);
    }
}

void deal_write(http_conn *client) {
    if (!client->write()) {
        client->close_conn(true);
    }
}


int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number \n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    // 忽略SIGPIPE信号
    addsig(SIGPIPE, SIG_IGN);

    // 预先为每个可能的客户连接分配一个http_conn对象
    http_conn *users = new http_conn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr *) &address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);


    // 创建线程池
    ThreadPool *threadPool = nullptr;
    try {
        threadPool = new ThreadPool(8);
    } catch (...) {
        LOG_DEBUG("创建线程池失败");
        return 0;
    }

    int timeoutMs = 60000;
    // 创建定时器
    sort_timer_lst *timer = new sort_timer_lst();

    epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    LOG_DEBUG("start server on port : %d, ip : %s, listenfd : %d", port, inet_ntoa(address.sin_addr), listenfd);
    while (true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }
        LOG_DEBUG("start epoll number : %d\n", number);
        LOG_DEBUG("用户数量为 : %d", users->m_user_count);
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                // 处理监听事件
                deal_listen(users, listenfd);
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 如果有异常，直接关闭连接
                LOG_DEBUG("这里出现了异常，关闭连接");
                users[sockfd].close_conn(true);
            } else if (events[i].events & EPOLLIN) {
                // 根据读的结果，决定是否将任务添加到线程池还是关闭连接
                threadPool->AddTask(std::bind(&deal_read, &users[sockfd]));
//                if (users[sockfd].read()) {
//                    LOG_DEBUG("读取请求成功，处理请求");
////                    users[sockfd].process();
//
////                    threadPool->AddTask(std::bind(users[sockfd].process(), users[sockfd]));
//                } else {
//                    LOG_DEBUG("读取请求失败，关闭连接");
//                    users[sockfd].close_conn(true);
//                }

            } else if (events[i].events & EPOLLOUT) {
                // 根据写的结果，决定是否关闭连接
                threadPool->AddTask(std::bind(&deal_write, &users[sockfd]));
//                if (!users[sockfd].write()) {
//                    users[sockfd].close_conn(true);
//                }
            } else {
                LOG_DEBUG("不做处理");
//                printf("不做处理\n");
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] users;
//    delete pool;
    return 0;
}
