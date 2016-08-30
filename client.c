#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
# define MAXLINE  4096

int max(int a,int b){
	if(a > b) return a;
	else return b;

}

void str_cli(FILE *fp,int sockfd){
	int maxfdp1;
	fd_set rset;

	char sendline[MAXLINE],recvline[MAXLINE];

	FD_ZERO(&rset); //initial select

//select
	for( ; ; ){
		FD_SET(fileno(fp),&rset); //fp i/o file pointer
		FD_SET(sockfd,&rset);	//fileno convers fp into descriptor
		maxfdp1 = max(fileno(fp),sockfd)+1;
		select(maxfdp1,&rset,NULL,NULL,NULL);
		
		if(FD_ISSET(sockfd,&rset)){ // socket is readable
			if(read(sockfd,recvline,MAXLINE) == 0)
				printf("str_cli: server terminated prematurely\n");
			fputs(recvline,stdout);
		}

		if(FD_ISSET(fileno(fp),&rset)){// input is readable
			if(fgets(sendline,MAXLINE,fp) == NULL)
				return; //all done
			write(sockfd,sendline,strlen(sendline));

		}
	}
}

void str_cli_select(FILE *fp,int sockfd){
	int maxfdp1,stdineof=0;
	fd_set rset;
	char sendline[MAXLINE],recvline[MAXLINE];
	
	stdineof = 0; //use for test readable
	FD_ZERO(&rset);
	FD_SET(fileno(fp), &rset);
 	maxfdp1 = fileno(fp) + 1;
	for( ; ; ){
		select(maxfdp1, &rset, 0, 0, 0);
		if (stdineof == 0){
			FD_SET(fileno(fp),&rset);
			FD_SET(sockfd,&rset);
			maxfdp1 = max(fileno(fp),sockfd)+1;
			select(maxfdp1,&rset,NULL,NULL,NULL);
		}
		if(FD_ISSET(sockfd,&rset)){
			if(read(sockfd,recvline,MAXLINE) == 0){
				if(stdineof == 1)
					return; //normal termination
				else
					printf("str_cli:server terminated prematurely\n");
			}
			fputs(recvline,stdout);
		}
		if(FD_ISSET(fileno(fp),&rset)){//input is readable
			if(fgets(sendline,MAXLINE,fp) == NULL){
				stdineof = 1;
				shutdown(sockfd,SHUT_WR); //send FIN
				FD_CLR(fileno(fp),&rset);
				continue;
			}
			write(sockfd,sendline,strlen(sendline));
		}
	}
}
int main(int argc,char **argv){
	int sockfd;
	struct sockaddr_in servaddr;
	if(argc!=3)
		printf("usage:tcpcli<IPaddress> <Port>");
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET,argv[1],&servaddr.sin_addr);
	connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	str_cli(stdin,sockfd);
	exit(0);
}


