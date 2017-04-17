#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define SERVPORT 1111
#define MAXLINE 2048

void print_exit(const char *msg)
{
    fputs(msg, stderr);
    exit(-1);
}

void str_cli(FILE *fp, int sockfd)
{
    int maxfdp1, val, stdineof;
    int n, nwritten;
    fd_set rset, wset;
    char to[MAXLINE], fr[MAXLINE];
    char *toiptr, *tooptr, *friptr, *froptr;

    val = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, val | O_NONBLOCK);
    val = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);
    val = fcntl(STDOUT_FILENO, F_GETFL, 0);
    fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

    toiptr = tooptr = to;
    friptr = froptr = fr;
    stdineof = 0;
    maxfdp1 = sockfd +1;
    for(;;)
    {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        if(stdineof == 0 && toiptr < &to[MAXLINE])
        FD_SET(STDIN_FILENO, &rset);
        if(friptr < &fr[MAXLINE])
        FD_SET(sockfd, &rset);
        if(tooptr!=toiptr)
        FD_SET(sockfd, &wset);
        if(froptr!=friptr)
        FD_SET(STDOUT_FILENO, &wset);
        select(maxfdp1, &rset, &wset, NULL, NULL);

        if(FD_ISSET(STDIN_FILENO, &rset))
        {
            if((n=read(STDIN_FILENO, toiptr, &to[MAXLINE] - toiptr))<0)
            {
                if(errno != EWOULDBLOCK)
                print_exit("read error on stdin");
            }
            else if(n == 0)
            {
                fprintf(stderr, "EOF on stdin\n");
                stdineof = 1;
                if(tooptr == toiptr)
                shutdown(sockfd, SHUT_WR);
            }
            else
            {
            toiptr += n;
            FD_SET(sockfd, &wset);
            }
        }

        if(FD_ISSET(sockfd, &rset))
        {
            if((n = read(sockfd, friptr, &fr[MAXLINE]- friptr))<0)
            {
                if(errno != EWOULDBLOCK)
                print_exit("read error on sockfd");
            }
            else if(n == 0)
            {
                fprintf(stderr, "EOF on socket\n");
                if(stdineof)
                return;
                else
                print_exit("str_cli: server terminated prematurely");
            }
            else
            {
                fprintf(stderr, "read %d bytes from socket\n", n);
                friptr+=n;
                FD_SET(STDOUT_FILENO, &wset);
            }
        }

        if(FD_ISSET(STDOUT_FILENO, &wset) && ((n=friptr - froptr))>0)
        {
            if((nwritten = write(STDOUT_FILENO, froptr, n))<0)
            {
                if(errno != EWOULDBLOCK)
                print_exit("write error to stdout");
            }
            else
            {
                fprintf(stderr, "wrote %d bytes to stdout\n", n);
                froptr += n;
                if(froptr == friptr)
                froptr = friptr = fr;
            }
        }

        if(FD_ISSET(sockfd, &wset) && (n=toiptr - tooptr)>0)
        {
            if((nwritten = write(sockfd, tooptr, n))<0)
            {
                if(errno!=EWOULDBLOCK)
                print_exit("write error to sockfd");
            }
            else
            {
                fprintf(stderr, "wrote %d bytes to sockfd\n",n);
                tooptr += n;
                if(tooptr == toiptr)
                toiptr = tooptr = to;
                if(stdineof)
                shutdown(sockfd, SHUT_WR);
            }
        }
    }
}

int main()
{
    const char SERVADDR[] = "127.0.0.1";
    const char *servipddr = SERVADDR;
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    inet_pton(AF_INET, servipddr, &servaddr.sin_addr);

    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    str_cli(stdin, sockfd);
    exit(0);
}