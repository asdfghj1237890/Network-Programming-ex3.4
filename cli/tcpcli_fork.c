#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/select.h>

#define MAXLINE 2048
#define SERVPORT 1111

int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, const int nsec)
{
    int flags, n, error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    tval.tv_sec = nsec;
    tval.tv_usec = 0;
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if((n = connect(sockfd, saptr, salen))<0)
    {
        if(errno!=EINPROGRESS)
        return -1;
    }
    if(n == 0)
    goto done;

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(sockfd, &rset);
    FD_SET(sockfd, &wset);

    if((n = select(sockfd+1, &rset, &wset, NULL, nsec?&tval:NULL)) ==0)
    {
        close(sockfd);
        errno = ETIMEDOUT;
        perror("select sockfd error");
        exit(-1);
    }
    printf("select return %d\n", n);

    if(FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
    {
        len = sizeof(error);
        if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)<0)
        {
            printf("getsockopt error\n");
            return -1;
        }
    }
    else
    {
        perror("select error: sockfd not set");
        exit(-1);
    }

done:
    fcntl(sockfd, F_SETFL, flags);
    if(error)
    {
        errno = error;
        return -1;
    }
    return 0;

}

void str_cli(FILE *fp, int sockfd)
{
    char sendline[MAXLINE], recvline[MAXLINE];
    pid_t pid;
    int n;

    if((pid = fork())==0)
    {
        while((n = read(sockfd, recvline, MAXLINE))>0)
        {
            write(STDOUT_FILENO, recvline, n);
        }
        kill(getpid(), SIGTERM);
        exit(0);
    }
    while((n = read(STDIN_FILENO, sendline, MAXLINE))>0)
    {
        write(sockfd, sendline, n);
    }
    shutdown(sockfd, SHUT_WR);
    pause();
    return;
}


int main()
{
    const char SERVIPADDR[] = "127.0.0.1";
    const char *servipaddr = SERVIPADDR;
    struct sockaddr_in servaddr;
    int sockfd, n;

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    inet_pton(AF_INET, servipaddr, &servaddr.sin_addr);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    for(;;)
    if(connect_nonb(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr), 5)<0)
    {
        continue;
    }
    else
    break;

    str_cli(stdin, sockfd);
}