#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <linux/tcp.h>

#define MAXPACKETSIZE 1448

int main(int argc, char *argv[])
{
	if(argc!=3)
	{
		printf("usage: %s destIP port packet_interval(us) output_file_name\n",argv[0]);
		exit(1);
	}
	int server_sockfd;
	int len;
	struct in_addr server_addr;
	if(!inet_aton(argv[1], &server_addr)) 
		perror("inet_aton");
	struct sockaddr_in remote_addr; 
	char buf[BUFSIZ];
	memset(&remote_addr,0,sizeof(remote_addr)); 
	remote_addr.sin_family=AF_INET; 
	remote_addr.sin_addr=server_addr;
//	memcpy (&remote_addr.sin_addr, &server_addr, sizeof(server_addr));
	int Flow_size=atoi(argv[2]);//发送数据的总大小，MB
	// double flow_size = Flow_size * 200000.0;
	long flow_size = Flow_size * 1000000.0;
	remote_addr.sin_port=htons(6688); 

	FILE * fp;
	if ((fp = fopen("/home/guolab/output/client-record.txt", "w")) == NULL)
	{
		printf("Cannot open packet record file\n");
		exit(0);
	}
	
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
//	int cflags = fcntl(server_sockfd,F_GETFL,0);
//	fcntl(server_sockfd,F_SETFL, cflags&~O_NONBLOCK);

	int nRecvBuf=1024*1024;
	setsockopt(server_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
	int nSendBuf=1024*1024;//ÉèÖÃÎª1M
	setsockopt(server_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
	
	if(connect(server_sockfd,(const struct sockaddr *)&remote_addr,sizeof(remote_addr))<0)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"connected error\n");
		fflush(fp);
		return 1;
	}

	// int testBuf=2048*2048;
	// if(setsockopt(server_sockfd, IPPROTO_TCP, TCP_CONGESTION,&testBuf,sizeof(int)) <0)
	// {
	// 	fseek(fp,0L,SEEK_END);
	// 	fprintf(fp,"setsockopt TCP_CONGESTION error\n");
	// 	fflush(fp);
	// 	return 0;
	// }
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"connected to server  %s\n",inet_ntoa(remote_addr.sin_addr));
	fflush(fp);

	long bytesSent=0;
	long packetsSent=0;
	long bytesToSend=MAXPACKETSIZE;
	char bufSent[MAXPACKETSIZE];

	struct timespec start,end,lastPacketStamp;
	clock_gettime(CLOCK_MONOTONIC,&start);

	while(bytesSent<flow_size)
	{
		// bytesToSend=flow_size-bytesSent;
		// if(bytesToSend>=MAXPACKETSIZE)
		// {
		// 	bytesToSend=MAXPACKETSIZE;
		// }
		memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
		// len=send(server_sockfd,"wwww",bytesToSend,0);
		// if ((len=send(server_sockfd,bufSent,bytesToSend,0))==-1)
		if ((len=send(server_sockfd,"wwwwwww",bytesToSend,0))==-1)
		{
			clock_gettime(CLOCK_MONOTONIC,&end);
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"interval is %f(us)\n",1000000*(end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)*0.001);
			fflush(fp);
		}
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"len is %d\n",len);
		fflush(fp);
		bytesSent+=bytesToSend;
		packetsSent++;
		// usleep(1000);
	}

	fprintf(fp,"Total %ld bytes, %ld packets sent to the server!\n",bytesSent,packetsSent);

	clock_gettime(CLOCK_MONOTONIC,&end);
	double timeuse=1000000*(end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)*0.001; 
	fprintf(fp,"Total time consumed is %lf(s)\n",timeuse/1000000);
	fprintf(fp,"Sending rate is %lf(Mbps)\n",flow_size*8/timeuse);
	fflush(fp);
	close(server_sockfd);//¹Ø±ÕÌ×½Ó×Ö
    return 0;
}
