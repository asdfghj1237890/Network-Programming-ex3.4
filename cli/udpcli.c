#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>

#define MAXLINE 2048
#define SERVPORT 1111

void dg_cli(FILE *fp, int sockfd, struct sockaddr* pservaddr, socklen_t servlen)
{
    int maxfdp1, fp_fd, stdineof, n;
    fd_set rset;
    char buf[MAXLINE], servaddrbuf[INET_ADDRSTRLEN];
    struct sockaddr servaddr;
    stdineof = 0;
    FD_ZERO(&rset);
    fp_fd = fileno(fp);
    for(;;)
    {
        if(stdineof == 0)
        FD_SET(fp_fd, &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = fp_fd > sockfd ? fp_fd : sockfd +1;
        select(maxfdp1, &rset, NULL, NULL, NULL);
        if(FD_ISSET(sockfd, &rset))
        {
            //printf("---------------------------\n");
            //printf("*sockfd readable*\n");
            if((n = recvfrom(sockfd, buf, MAXLINE, 0,  NULL, NULL)) == 0)
            {
                if(stdineof)
                {
                    printf("*No data read*\n*close client*\n");
                    printf("---------------------------\n");
                    return;
                }
                printf("sockfd read error");
                exit(-1);
            }
            else if(n < 0)
            {
                perror("read from sockfd error");
                exit(-1);
            }
            if(write(STDOUT_FILENO, buf, strlen(buf))<=0)
            {
                perror("write to stdout error");
                exit(-1);
            }
            else
            {
                //printf("get socket return from %s\n---------------------------\n", inet_ntop(AF_INET, &servaddr, servaddrbuf, INET_ADDRSTRLEN));
		printf("get socket return from 127.0.0.1:1111---------------------------\n");
            }

        }
        if(FD_ISSET(fp_fd, &rset))
        {
            //printf("+++++++++++++++++++++++++++\n");
            //printf("*stdin readable*\n");
            if((n = read(fp_fd, buf, MAXLINE) == 0))
            {
                printf("*No data read*\n");
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);
                printf("*close sock write*\n");
                printf("+++++++++++++++++++++++++++\n");
                FD_CLR(fp_fd, &rset);
                continue;
            }
            else if(n < 0)
            {
                perror("read from stdin error");
                exit(-1);
            }
            //printf("*write %s to socket*\n+++++++++++++++++++++++++++\n", buf);
            if(sendto(sockfd, buf, strlen(buf), 0, pservaddr, servlen)<=0)
            {
                perror("write to socket error");
                exit(-1);
            }
            //else
            //printf("send success\n");
        }
    }
}
int main()
{
    const char SERVADDR[] = "127.0.0.1";
    char servaddrbuf[INET_ADDRSTRLEN];
    const char *localaddr = SERVADDR;
    int sockfd;
    struct sockaddr_in servaddr;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))>1)
    printf("sockfd socket done, socket number: %d\n", sockfd);
    else
    {
        perror("sockfd socket error");
        exit(-1);
    }
    printf("SERVADDR: %s\n", SERVADDR);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    inet_pton(AF_INET, localaddr, &servaddr.sin_addr);

    dg_cli(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
}
