#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <linux/tcp.h>

int main(int argc, char *argv[])
{
	if(argc!=1)
	{
		printf("usage: %s port checkThroughInterval output_file_name1 output_file_name1\n",argv[0]);
		exit(1);
	}

	FILE * fp;
	if ((fp = fopen("/home/guolab/output/server-record.txt", "w")) == NULL)
	{
		printf("Cannot open packet record file\n");
		exit(0);
	}

	int server_sockfd;
	int client_sockfd;
	struct sockaddr_in my_addr;   
	struct sockaddr_in remote_addr;
	char buf[BUFSIZ]={}; 
	memset(&my_addr,0,sizeof(my_addr)); 
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=INADDR_ANY;
	my_addr.sin_port=htons(6688); 
	int* client_sockfd_ptr=NULL;
	
	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{  
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"socket error\n");
		fflush(fp);
		return 1;
	}
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"socket success\n");
	fflush(fp);

	int on=100;  
    if((setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)  
    {  
    	fseek(fp,0L,SEEK_END);
		fprintf(fp,"setsockopt error\n");
		fflush(fp);
        return 1;  
    }  
    fseek(fp,0L,SEEK_END);
	fprintf(fp,"setsockopt success\n");
	fflush(fp);
    // int nSendBuf=1024*1024;
	/*if(setsockopt(client_sockfd, IPPROTO_TCP, TCP_CONGESTION,&nSendBuf,sizeof(int)) <0)
	{
		fseek(fp,0L,SEEK_END);
	    fprintf(fp,"setsockopt error\n");
		fflush(fp);
		return 0; 
	}*/

	// int flag=1;
	// int result = setsockopt(server_sockfd,            /* socket affected */
	//                         IPPROTO_TCP,     /* set option at TCP level */
	//                         TCP_NODELAY,     /* name of option */
	//                         (char *) &flag,   the cast is historical cruft 
	//                         sizeof(int));    /* length of option value */
	// // setsockopt(server_sockfd, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
	// setsockopt(server_sockfd, IPPROTO_TCP, TCP_QUICKACK, (void *)&flag, sizeof(flag));

	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"bind error\n");
		fflush(fp);
		return 1;
	}
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"bind success\n");
	fflush(fp);
	
	if (listen(server_sockfd,100)==-1)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"listen error\n");
		fflush(fp);
		return 1;
	}
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"listen success\n");
	fflush(fp);
	
	socklen_t sin_size=sizeof(struct sockaddr_in);
 
	// long allReceiveBytes=0;
	struct timespec lastStamp,thisPacketTime;
	int len=0;

	int nRecvBuf=1024*1024;
	setsockopt(client_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
	int nSendBuf=1024*1024;
	setsockopt(client_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
	
	if ((client_sockfd=accept(server_sockfd,(struct sockaddr *)&remote_addr,&sin_size)<0))
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"accept error\n");
		fflush(fp);
		return 0;
	}
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"accept success\n");
	fflush(fp);
	
	while (1)
	{
		if ((len=recv(client_sockfd,buf,BUFSIZ,0))<=0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"recv error\n");
			fflush(fp);
			break;
		} 
		// allReceiveBytes+=len;
		clock_gettime(CLOCK_MONOTONIC,&thisPacketTime);
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"%d\t%f\n",len,thisPacketTime.tv_sec+thisPacketTime.tv_nsec*0.000000001);//每收到一个packet都要记录
		fflush(fp); 
	}

	close(client_sockfd);
	close(server_sockfd);
    return 0;
}
