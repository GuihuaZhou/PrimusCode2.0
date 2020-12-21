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
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#define MAXPACKETSIZE 1448
#define MAXPACKETNUM 10000

struct tempthreadparam
{
	int client_sockfd;
};

struct data// 数据包格式
{
	int serialNumber;
	int temp[1444];
};

struct datarecord// 发送端记录已经发送的信息
{
	struct timeval startStamp;
	int unRecvRS;
};

struct datarecord dataRecord[MAXPACKETNUM];
pthread_mutex_t mutex;
int client_num=0; 
FILE * fp;

void* RecvThread(void* tempThreadParam)
{
	int client_sockfd=((struct tempthreadparam *)tempThreadParam)->client_sockfd;
	struct timeval stopStamp;
	char buf[MAXPACKETSIZE]={};
	int len=0;
	struct data tempData;

	while (1)
	{
		memset(buf,'.',sizeof(MAXPACKETSIZE));
		if ((len=recv(client_sockfd,buf,MAXPACKETSIZE,0))<=0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"[Sock:%d] recv error:%s\n",client_sockfd,strerror(errno));
			fflush(fp);
		}

		memcpy(&tempData,buf,MAXPACKETSIZE);

		pthread_mutex_lock(&mutex);
		gettimeofday(&stopStamp,NULL);
		if (dataRecord[tempData.serialNumber].unRecvRS==1)// 完成传输
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"%f\n",((stopStamp.tv_sec-dataRecord[tempData.serialNumber].startStamp.tv_sec)*1000+(stopStamp.tv_usec-dataRecord[tempData.serialNumber].startStamp.tv_usec)*0.001));//记录时间
			fflush(fp); 
		}
		else dataRecord[tempData.serialNumber].unRecvRS--;
		pthread_mutex_unlock(&mutex);
	}
}

int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		printf("usage: %s port checkThroughInterval output_file_name1 output_file_name1\n",argv[0]);
		exit(1);
	}

	if ((fp = fopen("/home/guolab/server-record.txt", "a+")) == NULL)
	{
		printf("Cannot open packet record file.\n");
		exit(0);
	}

	int i=0;
	client_num=atoi(argv[1]);
	pthread_t recv_thread;
	int server_sockfd;
	int client_sockfd[100]={0};
	struct sockaddr_in my_addr;   
	struct sockaddr_in remote_addr;
	char buf[BUFSIZ]={}; 
	memset(&my_addr,0,sizeof(my_addr)); 
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=INADDR_ANY;
	my_addr.sin_port=htons(6688); 

	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{  
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"socket error.\n");
		fflush(fp);
		return 1;
	}

	int on=1;  
  if((setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)  
  {  
  	fseek(fp,0L,SEEK_END);
		fprintf(fp,"setsockopt error.\n");
		fflush(fp);
    return 1;  
  }  

	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"bind error.\n");
		fflush(fp);
		return 1;
	}

	if (listen(server_sockfd,100)==-1)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"listen error.\n");
		fflush(fp);
		return 1;
	}

	socklen_t sin_size=sizeof(struct sockaddr_in);

	struct timeval tempStamp={0,0};
	for (int i=0;i<MAXPACKETNUM;i++) dataRecord[i]={tempStamp,client_num};
 
	int len=0;
	for (i=0;i<client_num;i++)
	{
		int nRecvBuf=1024*1024;
		setsockopt(client_sockfd[i],SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
		int nSendBuf=1024*1024;
		setsockopt(client_sockfd[i],SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
		if ((client_sockfd[i]=accept(server_sockfd,(struct sockaddr *)&remote_addr,&sin_size))<0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"accept error.\n");
			fflush(fp);
			return 0;
		}

		struct tempthreadparam *tempThreadParam=(struct tempthreadparam *)malloc(sizeof(struct tempthreadparam));
		tempThreadParam->client_sockfd=client_sockfd[i];
		if (pthread_create(&recv_thread,NULL,RecvThread,(void*)tempThreadParam)<0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"create thread for sock:%d failed.\n",client_sockfd[i]);
			fflush(fp);
		} 
		// fseek(fp,0L,SEEK_END);
		// fprintf(fp,"accept success client[sock:%d,addr:%s].\n",client_sockfd[i],inet_ntoa(remote_addr.sin_addr));
		// fflush(fp);
	}

	int counter=0;
	struct data sendPacket;
	sendPacket.serialNumber=0;
	for (int i=0;i<1444;i++) sendPacket.temp[i]=0;

	while (1)
	{
		gettimeofday(&(dataRecord[counter].startStamp),NULL);

		// fseek(fp,0L,SEEK_END);
		// fprintf(fp,"第 %d 次请求.\t",counter);
		// fflush(fp);
		sendPacket.serialNumber=counter;
		memcpy(buf,&sendPacket,MAXPACKETSIZE);

		for (i=0;i<client_num;i++)
		{
			if ((len=send(client_sockfd[i],buf,MAXPACKETSIZE,0))<0)
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"send request to [sock:%d] failed.\n",client_sockfd[i]);
				fflush(fp);
			}
		}
		sleep(1);
		counter++;
	}

	for (i=0;i<client_num;i++) close(client_sockfd[i]);
	close(server_sockfd);
  return 0;
}