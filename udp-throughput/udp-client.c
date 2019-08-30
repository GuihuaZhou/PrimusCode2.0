#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>

#define MAXPACKETSIZE 1448//5792

int main(int argc, char *argv[])
{
	if(argc!=4)
	{
		printf("usage: %s destIP packet_interval(us) flow_size(bytes) output_file_name\n",argv[0]);
		exit(1);
	}
	int client_sockfd;
	int len;
	struct in_addr server_addr;
	if(!inet_aton(argv[1], &server_addr)) 
		perror("inet_aton");
	struct sockaddr_in remote_addr; 
	char buf[BUFSIZ];
	memset(&remote_addr,0,sizeof(remote_addr));
	remote_addr.sin_family=AF_INET;
	remote_addr.sin_addr=server_addr;
	remote_addr.sin_port=htons(atoi(argv[2]));
	// int Flow_size=atoi(argv[3]);//发送数据的总大小，MB
	// double flow_size = Flow_size * 200000.0;
	// long flow_size = Flow_size * 1000000.0;
	useconds_t packet_interval;
	packet_interval=atoi(argv[3]);//us

	FILE * fp;
	if ((fp = fopen("/home/guolab/output/clientRecord.txt", "w")) == NULL)
	{
		printf("Cannot open record_file\n");
		exit(0);
	}
	
	if((client_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{
		perror("socket");
		return 1;
	}
	
	struct timespec start,end,lastPacketStamp;
	clock_gettime(CLOCK_MONOTONIC,&start);

	long bytesSent=0;
	long packetsSent=0;
	char bufSent[MAXPACKETSIZE];
	long bytesToSend=MAXPACKETSIZE;;
	while(1)
	{
		// bytesToSend=flow_size-bytesSent;
		// if(bytesToSend>=MAXPACKETSIZE)
		// {
		// 	bytesToSend=MAXPACKETSIZE;
		// }
		bytesSent=bytesSent+bytesToSend;
		memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
		sendto(client_sockfd,bufSent,bytesToSend,0,(struct sockaddr*)&remote_addr,sizeof(remote_addr));
		// sendto(client_sockfd,bufSent,bytesToSend,0,(struct sockaddr*)&remote_addr,sizeof(remote_addr));
		// clock_gettime(CLOCK_MONOTONIC,&lastPacketStamp);
		// fseek(fp,0L,SEEK_END);
		// fprintf(fp,"%d\t%.9f\n",bytesToSend,lastPacketStamp.tv_sec+lastPacketStamp.tv_nsec*0.000000001);//每收到一个packet都要记录
		// fflush(fp);
		packetsSent++;
		usleep(packet_interval);
	}
	
	fprintf(fp,"Total %ld bytes, %ld packets sent to the server!\n",bytesSent,packetsSent);

	clock_gettime(CLOCK_MONOTONIC,&end);
	double timeuse=1000000*(end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)*0.001; 
	fprintf(fp,"Total time consumed is %lf(s)\n",timeuse/1000000);
	fprintf(fp,"Sending rate is %lf(Mbps)\n",bytesToSend*8/timeuse);
	fflush(fp);

	close(client_sockfd);
    return 0;
}
