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
#define LISTENQ 1024
#define SERVPORT 1111


typedef void Sigfunc(int);

Sigfunc *signal(int signo, Sigfunc *sig_handler)
{
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if(signo == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    if(sigaction(signo, &act, &oact)<0)
    return SIG_ERR;
    return oact.sa_handler;
}


void sigchld_handler(int signo)
{
    int stat;
    pid_t pid;
    while((pid = waitpid(-1, &stat, WNOHANG))>0)
    {
        printf("child %d terminated\n", pid);
    }
}

/*void str_echo(int sockfd)
{
    ssize_t n;
    int nback;
    char buf[MAXLINE];
again:
    while((n = read(sockfd, buf, MAXLINE))>0)
    {
        printf("get mesg:%s\n", buf);
        buf[n] = 0;
        if((nback = write(sockfd, buf, strlen(buf)))<0)
        {
            perror("write to sockfd error");
            exit(-1);
        }
        else
        {
            printf("return mesg:%s back, char number: %d\n", buf, nback);
        }
    }
    if(n<0 && errno==EINTR)
    goto again;
    else if(n<0)
    {
        perror("read error");
        exit(-1);
    }
}*/

void str_echo(int sockfd){
	ssize_t n;
	int buf_byte;
	char buf[MAXLINE];
	char buf1;
	again:
		while((buf_byte = read(sockfd,buf,MAXLINE))>0){
			buf[buf_byte] = '\0';
			//buf1 = "sendto 127.0.0.1 :"+atoi(cliaddr.sin_port);
			//write(sockfd,buf1,strlen(buf1));
			write(sockfd,buf,strlen(buf));
			
		}
		if(buf_byte<0 && errno == EINTR)  /* interrupted by a signal before any data was read*/
			goto again; //ignore EITNER
		else if (buf_byte<0)
			printf("str_echo:read error\n");
}

int main()
{
    int listenfd, connfd, udpfd, nready, maxfdp1, nback;
    char mesg[MAXLINE], cliaddrbuf[INET_ADDRSTRLEN], servaddrbuf[INET_ADDRSTRLEN];
    fd_set rset;
    ssize_t n;
    socklen_t len;
    const int on = 1;
    struct sockaddr_in cliaddr, servaddr;
    printf("server starting...\n");


    //tcp
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0))>1)
    printf("listen socket done, socket number: %d\n", listenfd);
    else
    {
        perror("listen socket error");
        exit(-1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))==0)//使得tcp和udp共用
    printf("setsockopt success\n");
    else
    {
        //close(listenfd);
        perror("setsockopt error");
        exit(-1);
    }

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))==0)
    	printf("tcp bind success\n");
    else
    {
        perror("tcp bind error\n");
        exit(-1);
    }


    //udp
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr))==0)
    printf("udp bind success, udpfd socket number: %d\n", udpfd);
    else
    {
        printf("udp bind error\n");
        exit(-1);
    }

    //handle connect
    signal(SIGCHLD, sigchld_handler);
    printf("signal_handler ready\n");
    FD_ZERO(&rset);
    maxfdp1 = (listenfd > udpfd ? listenfd : udpfd) +1;
    if(listen(listenfd, LISTENQ)==0)
    printf("listening\n");
    else
    {
        perror("listen error");
        exit(-1);
    }
    for(;;)
    {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);
        if((nready = select(maxfdp1, &rset, NULL, NULL, NULL))<0)
        {
            if(errno == EINTR)
            continue;
            else
            {
                perror("select error");
                exit(-1);
            }
        }
         if(FD_ISSET(udpfd, &rset))
        {
            //printf("udpfd readable\n");
            len = sizeof(cliaddr);
            //printf("####################\n");
            if((n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr*)&cliaddr, &len))>0)
            {
                mesg[n] = 0;
                printf("get mesg: %s\n", mesg);
            }
            else
            {
                perror("get mesg error");
                exit(-1);
            }
            if((nback = sendto(udpfd, mesg, strlen(mesg), 0, (struct sockaddr*)&cliaddr, len))>0)
            {
                //printf("sendto %s with char number: %ld\n", inet_ntop(AF_INET, (struct sockaddr*)&cliaddr, servaddrbuf, INET_ADDRSTRLEN), strlen(mesg));
		//printf("connection from %s port:%d \n",ip,cliaddr.sin_port);
		printf("sendto 127.0.0.1 :%d with char number: %ld\n",cliaddr.sin_port, strlen(mesg));
            }
            else
            {
                perror("sendto error");
                exit(-1);
            }
        }
        if(FD_ISSET(listenfd, &rset))
        {
            printf("tcpfd readable\n");
            len = sizeof(cliaddr);
            if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len))>1)

            printf("accept connect from %s\n", inet_ntop(AF_INET, &cliaddr.sin_addr, cliaddrbuf, INET_ADDRSTRLEN));
            else
            {
                perror("accept error");
                exit(-1);
            }
            if(fork()==0)
            {
                printf("*****************************\n");
                printf("fork a child progress to handle, pid: %d from %d\n", getpid(), getppid());
                close(listenfd);
                str_echo(connfd);
                printf("*****************************\n");
                exit(0);
            }
            close(connfd);
        }

    }
    //  close(udpfd);
    // close(listenfd);
    exit(0);
}
