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

#define MAXPACKETSIZE 1448

struct tempthreadparam
{
	int client_sockfd;
	int seq;
};

struct record
{
	int finishClient;// 已完成传输客户端个数
	struct timespec startTime;// 开始时间
};

struct record recordParam;
pthread_mutex_t mutex;
int client_num=0; 
FILE * fp;

void* ServerThread(void* tempThreadParam)
{
	int client_sockfd=((struct tempthreadparam *)tempThreadParam)->client_sockfd;
	int seq=((struct tempthreadparam *)tempThreadParam)->seq;
	struct timespec thisPacketTime;
	char buf[BUFSIZ]={};
	int len=0,i=0,recvLen=0;

	while (recvLen<2000)
	{
		memset(buf,'.',sizeof(BUFSIZ));
		if ((len=recv(client_sockfd,buf,BUFSIZ,0))<=0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"recv error:%s\n",strerror(errno));
			fflush(fp);
		}
		recvLen+=len;
	}
	 
	clock_gettime(CLOCK_MONOTONIC,&thisPacketTime);
	pthread_mutex_lock(&mutex);
	if (recordParam.finishClient<client_num-1);// 此次实验还有其他的客户端还没有完成传输
	else 
	{
		// 全部完成了
		fseek(fp,0L,SEEK_END);
		// fprintf(fp,"seq:%d\tdataLen:%d\tcost:%f ms\n",seq,recvLen,(thisPacketTime.tv_sec+thisPacketTime.tv_nsec*0.000000001-recordParam.startTime.tv_sec-recordParam.startTime.tv_nsec*0.000000001)*1000);//每收到一个packet都要记录
		fprintf(fp,"%f\n",((thisPacketTime.tv_sec-recordParam.startTime.tv_sec)*1000+(thisPacketTime.tv_nsec-recordParam.startTime.tv_nsec)*0.000001));//每收到一个packet都要记录
		fflush(fp); 
	}
	recordParam.finishClient++;
	pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		printf("usage: %s port checkThroughInterval output_file_name1 output_file_name1\n",argv[0]);
		exit(1);
	}

	if ((fp = fopen("/home/guolab/ATCOutput/server-record.txt", "a+")) == NULL)
	{
		printf("Cannot open packet record file.\n");
		exit(0);
	}

	int i=0;
	client_num=atoi(argv[1]);
	pthread_t server_thread;
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
		// fseek(fp,0L,SEEK_END);
		// fprintf(fp,"accept success client[sock:%d,addr:%s].\n",client_sockfd[i],inet_ntoa(remote_addr.sin_addr));
		// fflush(fp);
	}

	int counter=0;
	while (1)
	{
		recordParam.finishClient=0;
		clock_gettime(CLOCK_MONOTONIC,&(recordParam.startTime));

		// fseek(fp,0L,SEEK_END);
		// fprintf(fp,"第 %d 次请求.\t",counter+1);
		// fflush(fp);

		for (i=0;i<client_num;i++)
		{
			memset(buf,'.',MAXPACKETSIZE*sizeof(buf[0]));
			if ((len=send(client_sockfd[i],buf,MAXPACKETSIZE*sizeof(buf[0]),0))<0)
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"send request to [sock:%d] failed.\n",client_sockfd[i]);
				fflush(fp);
			}

			struct tempthreadparam *tempThreadParam=(struct tempthreadparam *)malloc(sizeof(struct tempthreadparam));
			tempThreadParam->seq=counter;
			tempThreadParam->client_sockfd=client_sockfd[i];
			if (pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"create thread for sock:%d failed.\n",client_sockfd[i]);
				fflush(fp);
			} 
		}
		while (1)
		{
			usleep(100);
			if (recordParam.finishClient>=client_num) break;
		}
		sleep(1);
		counter++;
	}

	for (i=0;i<client_num;i++) close(client_sockfd[i]);
	close(server_sockfd);
  return 0;
}

// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <netdb.h>
// #include <fcntl.h>
// #include <time.h>
// #include <linux/tcp.h>
// #include <errno.h>
// #include <pthread.h>

// #define MAXPACKETSIZE 1448

// int client_num=0; 
// FILE * fp;

// int main(int argc, char *argv[])
// {
// 	if(argc!=2)
// 	{
// 		printf("usage: %s port checkThroughInterval output_file_name1 output_file_name1\n",argv[0]);
// 		exit(1);
// 	}

// 	if ((fp = fopen("/home/guolab/ATCOutput/server-record.txt", "a+")) == NULL)
// 	{
// 		printf("Cannot open packet record file.\n");
// 		exit(0);
// 	}

// 	int i=0;
// 	client_num=atoi(argv[1]);
// 	pthread_t server_thread;
// 	int server_sockfd;
// 	int client_sockfd[100]={0};
// 	struct sockaddr_in my_addr;   
// 	struct sockaddr_in remote_addr;
// 	char buf[BUFSIZ]={}; 
// 	memset(&my_addr,0,sizeof(my_addr)); 
// 	my_addr.sin_family=AF_INET; 
// 	my_addr.sin_addr.s_addr=INADDR_ANY;
// 	my_addr.sin_port=htons(6688); 

// 	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
// 	{  
// 		fseek(fp,0L,SEEK_END);
// 		fprintf(fp,"socket error.\n");
// 		fflush(fp);
// 		return 1;
// 	}

// 	int on=1;  
//   if((setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)  
//   {  
//   	fseek(fp,0L,SEEK_END);
// 		fprintf(fp,"setsockopt error.\n");
// 		fflush(fp);
//     return 1;  
//   }  

// 	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
// 	{
// 		fseek(fp,0L,SEEK_END);
// 		fprintf(fp,"bind error.\n");
// 		fflush(fp);
// 		return 1;
// 	}

// 	if (listen(server_sockfd,100)==-1)
// 	{
// 		fseek(fp,0L,SEEK_END);
// 		fprintf(fp,"listen error.\n");
// 		fflush(fp);
// 		return 1;
// 	}

// 	socklen_t sin_size=sizeof(struct sockaddr_in);
 
// 	int len=0;
// 	for (i=0;i<client_num;i++)
// 	{
// 		int nRecvBuf=1024*1024;
// 		setsockopt(client_sockfd[i],SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
// 		int nSendBuf=1024*1024;
// 		setsockopt(client_sockfd[i],SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
// 		if ((client_sockfd[i]=accept(server_sockfd,(struct sockaddr *)&remote_addr,&sin_size))<0)
// 		{
// 			fseek(fp,0L,SEEK_END);
// 			fprintf(fp,"accept error.\n");
// 			fflush(fp);
// 			return 0;
// 		}
// 		// fseek(fp,0L,SEEK_END);
// 		// fprintf(fp,"accept success client[sock:%d,addr:%s].\n",client_sockfd[i],inet_ntoa(remote_addr.sin_addr));
// 		// fflush(fp);
// 	}

// 	struct timespec startTime,stopTime;
// 	int recvLen=0;
// 	while (1)
// 	{
// 		clock_gettime(CLOCK_MONOTONIC,&(startTime));

// 		for (i=0;i<client_num;i++)
// 		{
// 			memset(buf,'.',MAXPACKETSIZE*sizeof(buf[0]));
// 			if ((len=send(client_sockfd[i],buf,MAXPACKETSIZE*sizeof(buf[0]),0))<0)
// 			{
// 				fseek(fp,0L,SEEK_END);
// 				fprintf(fp,"send request to [sock:%d] failed.\n",client_sockfd[i]);
// 				fflush(fp);
// 			}
// 			recvLen=0;
// 			while (recvLen<2000)
// 			{
// 				memset(buf,'.',sizeof(BUFSIZ));
// 				if ((len=recv(client_sockfd[i],buf,BUFSIZ,0))<=0)
// 				{
// 					fseek(fp,0L,SEEK_END);
// 					fprintf(fp,"recv error:%s\n",strerror(errno));
// 					fflush(fp);
// 				}
// 				recvLen+=len;
// 			}
// 		}
// 		clock_gettime(CLOCK_MONOTONIC,&stopTime);
		
// 		fseek(fp,0L,SEEK_END);
// 		fprintf(fp,"%f\n",((stopTime.tv_sec-startTime.tv_sec)*1000+(stopTime.tv_nsec-startTime.tv_nsec)*0.000001));//每收到一个packet都要记录
// 		fflush(fp); 

// 		sleep(1);
// 	}

// 	for (i=0;i<client_num;i++) close(client_sockfd[i]);
// 	close(server_sockfd);
//   return 0;
// }
