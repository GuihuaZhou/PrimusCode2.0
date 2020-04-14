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

#define MAXPACKETSIZE 1448//

int main(int argc, char *argv[])
{
	// if(argc!=4)
	// {
	// 	printf("usage: %s destIP packet_interval(us) flow_size(bytes) output_file_name\n",argv[0]);
	// 	exit(1);
	// }
	int client_sockfd;
	int len;
	struct in_addr server_addr;
	if(!inet_aton(argv[1], &server_addr)) 
		perror("inet_aton");
	struct sockaddr_in remote_addr; 
	memset(&remote_addr,0,sizeof(remote_addr));
	remote_addr.sin_family=AF_INET;
	remote_addr.sin_addr=server_addr;
	remote_addr.sin_port=htons(6689);
	useconds_t packet_interval;
	packet_interval=atoi(argv[2]);//us
	int packet_number=atoi(argv[3]);

	FILE * fp;
	if ((fp=fopen(argv[4], "a+")) == NULL)
	{
		printf("Cannot open record_file\n");
		exit(0);
	}
	
	if((client_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{
		perror("socket");
		return 1;
	}
	long bytesSent=0;
	long packetsSent=0;
	int byteToSend=MAXPACKETSIZE;
	char bufSent[MAXPACKETSIZE];
	
	struct timespec start,end,lastPacketStamp;
	clock_gettime(CLOCK_MONOTONIC,&start);

	while (1)
	{
		for (int i=0;i<packet_number;i++)
		{
			bytesSent+=byteToSend;
			memset(bufSent,'.',byteToSend*sizeof(bufSent[0]));
			if (sendto(client_sockfd,bufSent,byteToSend,0,(struct sockaddr*)&remote_addr,sizeof(remote_addr))<0)
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"Send to error\n");
				fflush(fp);
				break;
			}
		}
		usleep(packet_interval);
	}

	clock_gettime(CLOCK_MONOTONIC,&end);
	double timeuse=1000000*(end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)*0.001; //us

	fseek(fp,0L,SEEK_END);
	// fprintf(fp,"Time:%f\n",timeuse);
	fprintf(fp,"Total %ld bytes, %ld packets sent to the server!\n",bytesSent,packetsSent);
	fprintf(fp,"Total time consumed is %lf(s)\n",timeuse/1000000);
	fprintf(fp,"Sending rate is %lf(Mbps)\n",bytesSent*8/timeuse);
	fflush(fp);

	close(client_sockfd);
  return 0;
}
