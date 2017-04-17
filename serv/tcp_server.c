#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <sys/select.h>

#define MAXLINE 2048
#define LISTENQ 1024

typedef void Sigfunc(int);

Sigfunc *signal(int signo, Sigfunc *handler)
{
    struct sigaction act, oact;
    act.sa_flags = 0;
    act.sa_handler = handler;

    if(signo == SIGALRM)
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
    else
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART;
#endif
    if(sigaction(signo, &act, &oact)<0)
    return SIG_ERR;
    else
    return oact.sa_handler;
}

void print_msg(const char *msg)
{
    fputs(msg, stderr);
    exit(errno);
}

void print_err(const char *msg)
{
    perror(msg);
    exit(errno);
}

int tcp_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd > 0)
    return sockfd;
    else
    print_err("socket error");
}

void tcp_bind(int sockfd, struct sockaddr *pservaddr, socklen_t servlen)
{
    if(bind(sockfd, pservaddr, servlen) < 0)
    print_err("bind error");
}

void tcp_listen(int sockfd)
{
    if(listen(sockfd, LISTENQ)<0)
    print_err("listen error");
}

int tcp_accept(int sockfd)
{
    int connfd = accept(sockfd, NULL, NULL);
    if(connfd < 0)
    print_err("accept error");
    return connfd;
}

ssize_t writen(int fd, char *buf, size_t n)
{
    size_t left = n, nwritten;
    char *ptr = buf;

    while(left > 0)
    {
        if((nwritten = write(fd, ptr, left))>0)
        {
            left -= nwritten;
            ptr += nwritten;
        }
        else if(errno == EINTR)
        {
            nwritten = 0;
            continue;
        }
        else
        return -1;
    }
    return n;
}

int select_read(int maxfdp1, fd_set *rset)
{
    int nready;
    if((nready= select(maxfdp1, rset, NULL, NULL, NULL)) <=0)
    print_err("select error");
    return nready;
}


void *str_cli_base(void *pfd)
{
    ssize_t n;
    char buf[MAXLINE];
    int sockfd = *(int*)pfd;

    for(;;)
    {
        if((n = read(sockfd, buf, MAXLINE-1)) > 0)
        {
            buf[n] = 0;
            printf("socket: %d get msg: %s\n", sockfd, buf);
            if(writen(sockfd, buf, n)==-1)
            print_err("write to sockfd error");
        }
        else if(n < 0 && errno == EINTR)
        continue;
        else
        break;
    }
    printf("close socket number: %d\n", sockfd);
    close(sockfd);
}

int main(int argc, char *argv[])
{
    int choice, i, maxi, maxfd, nready, n;
    int listenfd, connfd;
    int client[FD_SETSIZE];
    char buf[MAXLINE];
    pid_t pid;
    pthread_t tid;
    fd_set rset, allset;
    struct sockaddr_in servaddr;

    if(argc != 2)
    print_msg("usage: ./filename portnum\n");

    listenfd = tcp_socket();
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    tcp_listen(listenfd);

    printf("输入你想使用的tcp服务器方式:\n\t1 一连接一进程 \n\t2 单线程select监测 \n\t3 一连接一线程\n");
    scanf("%d", &choice);
    printf("serv starting...\n");

    switch(choice)
    {
    case 1:
        for(;;)
        {
            connfd = tcp_accept(listenfd);
            printf("get a connect, socket number: %d\t", connfd);

            if((pid = fork()) == 0)
            {
                printf("fork a child progress %d\n", getpid());
                close(listenfd);
                str_cli_base(&connfd);
                printf("child progress %d exit\n", getpid());
                exit(0);
            }
            else if(pid < 0)
            print_err("fork error");

            close(connfd);
        }
    case 2:
        maxi = -1;
        maxfd = listenfd;
        for(i = 0; i < FD_SETSIZE; ++i)
        client[i] = -1;

        FD_ZERO(&allset);
        FD_SET(listenfd, &allset);

        for(;;)
        {
            rset = allset;
            nready = select_read(maxfd+1, &rset);
            if(FD_ISSET(listenfd, &rset))
            {
                connfd = tcp_accept(listenfd);
                for(i = 0; i < FD_SETSIZE; ++i)
                {
                    if(client[i] == -1)
                    {
                        printf("sockfd: %d connect\n", connfd);
                        client[i] = connfd;
                        break;
                    }
                }

                if(i == FD_SETSIZE)
                print_msg("too many clients");

                FD_SET(connfd, &allset);
                if(connfd > maxfd)
                maxfd = connfd;
                if(i > maxi)
                maxi = i;
                if(--nready <= 0)
                continue;
            }

            for(i = 0; i <= maxi && nready > 0; ++i)
            {
                if(client[i] == -1)
                continue;
                if(FD_ISSET(client[i], &rset))
                {
                    if((n = read(client[i], buf, MAXLINE))>0)
                    {
                        buf[n] = 0;
                        printf("socket %d get msg: %s\n", client[i], buf);
                        if(writen(client[i], buf, n)!=n)
                        print_err("write to sockfd error");
                    }
                    else if(n < 0)
                    print_err("read error");
                    else{
                        printf("sockfd: %d terminated\n", client[i]);
                        close(client[i]);
                        FD_CLR(client[i], &allset);
                        client[i] = -1;
                        break;
                    }
                    --nready;
                }
            }
        }
    case 3:
        for(;;)
        {
            connfd = tcp_accept(listenfd);
            if(pthread_create(&tid, NULL, &str_cli_base, &connfd) < 0)
            print_err("thread created error");
            printf("create thread %ld to handle socket number %d\n", tid, connfd);
            if(pthread_detach(tid)<0)
            print_err("thread detach error");
        }
    }
    close(listenfd);
}
