#ifndef FD_H
#define FD_H
#include "http_conn.h"
// #define connfdET  // 边缘触发非阻塞
#define connfdLT  // 水平触发阻塞

// #define listenfdET // 边缘触发非阻塞
#define listenfdLT  // 水平触发阻塞

// 对文件描述符设置非阻塞
void set_nonbolcking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}

// 添加需要监听的文件描述符到epoll中
void add_fd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;

#ifdef connfdET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;  // EPOLLRDHUP: 判断对端是否已经关闭
#endif

#ifdef connfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef listenfdET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef listenfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    set_nonbolcking(fd);  // 设置非阻塞
}

#endif