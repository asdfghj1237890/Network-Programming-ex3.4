#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

# define MAXLINE  4096

void str_echo(int sockfd){
	ssize_t n;
	char buf[MAXLINE];
	again:
		while((n = read(sockfd,buf,MAXLINE))>0)
			write(sockfd,buf,n);
		if(n<0 && errno == EINTR)
			goto again;
		else if (n<0)
			printf("str_echo:read error");
}
int main(int argc,char **argv){
	char * ip;

//select
	int listenfd,connfd,udpfd,nready,maxfdp1;
	char mesg[MAXLINE];
	pid_t childpid;
	fd_set rset;
	ssize_t n;
	socklen_t len;
	const int on = 1;
	struct sockaddr_in cliaddr,servaddr;
	void sig_chld(int);
	/*
	int i,maxi,maxfd,listenfd,connfd,sockfd;
	int nready,client[FD_SETSIZE];
	ssize_t n;
	fd_set rset,allset;
	char line[MAXLINE];
	socklen_t clilen;
	struct sockaddr_in cliaddr,servaddr;*/
	
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family  = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(9877);
	
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	listen(listenfd,1024);
	
	//for create UDP socket
	udpfd = socket(AF_INET,SOCK_DGRAM,0);
	
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind(udpfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	siganl(SIGCHLD,sig_chld); //must call waitpid()
	
	FD_ZERO(&rset);
	maxfdp1 = max(listenfd,udpfd)+1;
	for( ; ; ){
		FD_SET(listenfd,&rset);
		FD_SET(udpfd,&rset);
		if((nready = select(maxfdp1,&rset,NULL,NULL,NULL))<0){
			if(errno == EINTR)
				continue; // back to for()
			else
				printf("select error");
		}
		if(FD_ISSET(listenfd,&rset)){
			len = sizeof(cliaddr);
			connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&len);
		if((childpid = fork()) == 0){ //child process
			close(listenfd); //close listening socket
			str_echo(connfd);//process request
			exit(0);
		}
		close(connfd); // parent closes connected socket
		}
		if(FD_ISSET(udpfd,&rset)){
			len = sizeof(cliaddr);
			n = recvfrom(udpfd,mesg,MAXLINE,0,(struct sockaddr*)&cliaddr,&len);
			sendto(udpfd,mesg,n,0,(struct sockaddr*)&cliaddr,len);
		}
	}
	maxfd = listenfd; //initialize
	maxi = -1; //index into client[] array
	for(i = 0;i<FD_SETSIZE;i++)
		client[i] = -1; // -1 indicates available entry
	FD_ZERO(&allset);
	FD_SET(listenfd,&allset);
	puts("prepared to accept\n");
	for( ; ; ){
		
		rset = allset; //structure assignment
		nready = select(maxfd +1,&rset,NULL,NULL,NULL);
		
		if(FD_ISSET(listenfd,&rset)){// new client connection
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);

			for(i = 0;i < FD_SETSIZE;i++)
				if(client[i] <0 ){
					client[i] = connfd;
					break;
				}
			ip = inet_ntoa(cliaddr.sin_addr);
			printf("connection from %s port:%d \n",ip,cliaddr.sin_port);
		
			if(i == FD_SETSIZE)
				printf("too many clients\n"); //cannot accept clients
			FD_SET(connfd,&allset);
			if(connfd > maxfd) maxfd = connfd; // for select
			if( i > maxi ) maxi = i; // max index in client[] array
			if(--nready <= 0) continue;	//no more readable descriptors
			//check all clients for data
			for(i = 0;i <= maxi;i++){
				if((sockfd = client[i])<0) continue; //skip empty client
				if(FD_ISSET(sockfd,&rset)){
					if((n = read(sockfd,line,MAXLINE)) == 0){
						//connection closed by client
						close(sockfd);
						//close(sockfd,&allset);
						FD_CLR(sockfd,&allset);
						client[i] = -1;
					}else //writen(sockfd,line,n);
						write(sockfd,line,n);
					if(--nready <= 0)  break; //no more readable descriptors
				}
			}
		}
	}
}

