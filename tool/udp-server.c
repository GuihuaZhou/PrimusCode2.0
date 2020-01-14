#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex;

FILE * fp;
int checkInterval=10000;
double tempRecvByte=0,allReceiveByte=0;
int receiveCount=0;
struct timespec start,end;

void* ThroughputThread(void* object)
{
	while (1)
	{
		usleep(checkInterval);
		pthread_mutex_lock(&mutex);
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"%.3f\n",(tempRecvByte*8)/checkInterval);
		fflush(fp);
		allReceiveByte+=tempRecvByte;
		tempRecvByte=0;
		pthread_mutex_unlock(&mutex);
	}
}

int main(int argc, char *argv[])
{
	// if(argc!=2)
	// {
	// 	printf("usage: %s flow_size(bytes) results_file_name throughput_file_name\n",argv[0]);
	// 	exit(1);
	// }

	int server_sockfd;
	struct sockaddr_in my_addr;  
	struct sockaddr_in remote_addr; 
	int sin_size;
	char buf[BUFSIZ]; 
	memset(&my_addr,0,sizeof(my_addr)); 
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=INADDR_ANY;
	my_addr.sin_port=htons(6689); 

	struct timeval timeout;
	timeout.tv_sec=5;
	timeout.tv_usec=0;
	
	checkInterval=atoi(argv[1]);

	if ((fp=fopen(argv[2], "a+")) == NULL)
	{
		printf("Cannot open packet record file\n");
		exit(0);
	}	

	if((server_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{  
		perror("create socket");
		return 1;
	}
	
	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))<0)
	{
		perror("set bind");
		return 1;
	}
	
	int len=0;
	unsigned int addrLen = sizeof(struct sockaddr_in);

	pthread_t throughput_thread;
	if (pthread_create(&throughput_thread,NULL,ThroughputThread,NULL)<0)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp, "create throughput thread error.\n");
		fflush(fp);
	}

	if (setsockopt(server_sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))<0)
	{
		perror("set SO_RCVTIMEO");
		return 1;
	}

	clock_gettime(CLOCK_MONOTONIC,&start);

	while (1)
	{
		if ((len=recvfrom(server_sockfd,buf,BUFSIZ,0,(struct sockaddr*)&remote_addr,&addrLen))<0)
		{
			fseek(fp,0L,SEEK_END);
		 	fprintf(fp,"recvfrom error.\n");
		 	fflush(fp);
			break;
		}

		tempRecvByte+=len;
		receiveCount++;
	}
	clock_gettime(CLOCK_MONOTONIC,&end);
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"Total %f bytes, %f packets recv!\n",allReceiveByte,receiveCount);
	fprintf(fp,"Recv rate is %lf(Mbps)\n",allReceiveByte*8/((end.tv_sec-start.tv_sec)*100000+(end.tv_nsec-start.tv_nsec)*0.001));
	fflush(fp);
	fclose(fp);
	
	close(server_sockfd);
  return 0;
}
