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

void dg_cli(FILE *fp,int sockfd,const struct sockaddr * pservaddr,socklen_t servlen){
	int n;
	char sendline[MAXLINE],recvline[MAXLINE+1];
	while(fgets(sendline,MAXLINE,fp)!=NULL){
		sendto(sockfd,sendline,strlen(sendline),0,pservaddr,servlen);
		n = recvfrom(sockfd,recvline,MAXLINE,0,NULL,NULL);
		recvline[n] = 0; //null terminate
		fputs(recvline,stdout);

	}
}
int main(int argc,char **argv){
	int sockfd;
	struct sockaddr_in servaddr;

	if(argc != 2)
		err_quit("usage:udpcli<IPaddress>");
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,argv[1],&servaddr.sin_addr);
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	
	dg_cli(stdin,sockfd,(struct socksddr*)&servaddr,sizeof(servaddr));
	exit(0);

}
