#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>

#include "./http/http_conn.h"
#include "./http/fd.h"
#include "./log/log.h"

#define MAX_EVENT_NUMBER 10000   // 最大事件数

#define SYNLOG  // 同步写日志
// #define ASYNLOG // 异步写日志


int main(int argc, char* argv[]) {
#ifdef ASYNLOG
    Log::get_instance()->init("ServerLog", 2000, 800000, 8);
#endif

#ifdef SYNLOG
    Log::get_instance()->init("ServerLog", 2000, 800000, 0);
#endif

    // if (argc <= 1) {
    //     printf("usage: %s ip_address port_number\n", basename(argv[0]));
    //     return 1;
    // }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = 0;
    struct sockaddr_in address;  // #include <arpa/inet.h>
    bzero(&address, sizeof(address));  // #include <string.h>
    address.sin_family = AF_INET;  // 定义地址协议族为TCP/IPv4
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(9999);
    
    // 设置端口复用
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    add_fd(epollfd, listenfd, false);

    bool stop_server = false;

    struct epoll_event epev;
    epev.events = EPOLLIN;

    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            // 处理新连接
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addr_length = sizeof(client_address);
#ifdef listenfdLT
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addr_length);
                if (connfd < 0) {
                    LOG_ERROR("%s: errno is: %d", "accept error", errno);
                    continue;
                }
                epev.data.fd = connfd;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &epev);
#endif
            } else {
                // 处理数据
                char buf[1024] = {0};

                int len = read(sockfd, buf, sizeof(buf));

                if (len == -1) {
                    perror("read");
                    printf("\n");
                    exit(-1);
                } else if (len == 0) {
                    printf("client closed...\n");
                    close(sockfd);
                } else if (len > 0) {
                    printf("read buf = %s\n", buf);
                    write(sockfd, buf, strlen(buf) + 1);
                }
            }

        }
    }

    close(epollfd);
    close(listenfd);

    return 0;
}