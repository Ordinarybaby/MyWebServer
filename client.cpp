#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main() {
    int connfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, "172.24.28.226", &serveraddr.sin_addr.s_addr);
    serveraddr.sin_port = htons(9999);
    int ret = connect(connfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    if (ret == -1) {
        perror("connect");
        exit(0);
    }
    char recvBuf[1024];
    int i = 0;
    while (true) {
        sprintf(recvBuf, "send data %d\n", i++);
        write(connfd, recvBuf, strlen(recvBuf) + 1);
        int len = read(connfd, recvBuf, sizeof(recvBuf));
        if (len == -1) {
            perror("read");
            exit(-1);
        } else if (len > 0) {
            printf("recv server %s\n", recvBuf);
        } else if (len == 0) {
            printf("server closed...\n");
            break;
        }
        usleep(1000);
    }
    close(connfd);
}