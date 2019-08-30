#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>

#define MAXPACKETSIZE 1448

int main(int argc, char *argv[])
{
	//printf("hello udp-client\n");
	if(argc!=5)
	{
		printf("usage: %s destIP packet_interval(us) flow_size(bytes) output_file_name\n",argv[0]);
		exit(1);
	}
	int client_sockfd;
	int len;
	struct in_addr server_addr;
	if(!inet_aton(argv[1], &server_addr)) 
		perror("inet_aton");
	struct sockaddr_in remote_addr; //服务器端网络地址结构体
//	char buf[BUFSIZ];  //数据传送的缓冲区
	char buf[BUFSIZ];
	memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
	remote_addr.sin_family=AF_INET; //设置为IP通信
	remote_addr.sin_addr=server_addr;
	remote_addr.sin_port=htons(atoi(argv[2])); //服务器端口号
	useconds_t packet_interval;
	packet_interval=atoi(argv[3]);

	FILE * fp;
	if ((fp = fopen(argv[4], "a+")) == NULL)
	{
		printf("Cannot open output_file %s\n",argv[4]);
		exit(0);
	}
	
	/*创建客户端套接字--UDP协议*/
	if((client_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{
		perror("socket");
		return 1;
	}
	
	/*循环的发送接收信息并打印接收信息--recv返回接收到的字节数，send返回发送的字节数*/
	struct timeval start,time;
	gettimeofday( &start, NULL );

	double stopTime=start.tv_sec+50.0;

	int bytesSent=0;
	int packetsSent=0;
	char bufSent[MAXPACKETSIZE];
	while(1)
	{
		/*int bytesToSend=flow_size-bytesSent;
		if(bytesToSend>=MAXPACKETSIZE)
		{
			bytesToSend=MAXPACKETSIZE;
		}*/
		int bytesToSend=MAXPACKETSIZE;
		bytesSent=bytesSent+bytesToSend;
		memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
		sendto(client_sockfd,bufSent,bytesToSend,0,(struct sockaddr*)&remote_addr,sizeof(remote_addr));
		packetsSent++;
		gettimeofday( &time, NULL );
		/*fprintf(fp,"Now Time is:%ld and stopTime:%lf\n",time.tv_sec,stopTime);
		fflush(fp);*/
		if (time.tv_sec>stopTime) break;
		usleep(packet_interval);
	}
	
	fprintf(fp,"Total %d bytes, %d packets sent to the server!\n",bytesSent,packetsSent);
	struct timeval end;
	gettimeofday( &end, NULL );
	//double timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
	double timeuse = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)*0.000001; 
	fprintf(fp,"Total time consumed is %lf(s)\n",timeuse);
	fprintf(fp,"Sending rate is %lf(Mbps)\n",bytesSent*8/(timeuse*1024*1024));
	fflush(fp);

	close(client_sockfd);//关闭套接字
         return 0;
}
