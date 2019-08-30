#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <sys/time.h>
#include <pthread.h>
#include <linux/tcp.h>
#include <errno.h>

char IPaddress[32];

//A struct for a connection
struct connection
{
	int id;
	int port;
	int size;
};

struct sockfd_size{
	int sockfd;
	int size;
	int id;
};

//Function to init a connection to transmit data
void* client_thread_func(void* connection_ptr);

//Set send window
void set_send_window(int sockfd, int window);

//Set receive window
void set_recv_window(int sockfd, int window);

//Print usage information
void usage();

int main(int argc, char **argv)
{
	int i=0;
	int port=5001;
	int data_size=0;
	int connections=0;
	//Array of struct connection
	struct connection* incast_connections=NULL;
	//Array of pthread_t
	pthread_t* client_threads=NULL;
	//Total start time
	struct timeval tv_start_total;
	//Total end time
	struct timeval tv_end_total;

	struct sockfd_size* sockfds=NULL;
	
	if(argc!=7)
	{
		usage();
		return 0;
	}
	
	//Get connections: char* to int
	connections=atoi(argv[1]);
	//Get data_size: char* to int
	data_size=atoi(argv[2]);

	port=atoi(argv[3]);

	char resultFileName[256];
	memset(resultFileName,0,256*sizeof(resultFileName[0]));
	strncpy(resultFileName,argv[4],strlen(argv[4]));

	int ifWriteNewResultFile=0;
	ifWriteNewResultFile=atoi(argv[5]);

	memset(IPaddress,0,32*sizeof(IPaddress[0]));
	sprintf(IPaddress,"%s",argv[6]);

	//Initialize 
	incast_connections=(struct connection*)malloc(connections*sizeof(struct connection));
	client_threads=(pthread_t*)malloc(connections*sizeof(pthread_t));
	sockfds = (struct sockfd_size *)malloc(connections*sizeof(struct sockfd_size));

	for(i=0;i<connections;i++){
		//struct connection incast_connection=*(struct connection*)connection_ptr;
	//Get ID
		printf("%d connection \n",i);
		fflush(stdout);
		int id=i+1;

		//Get port
		int subport=port;
		//Get traffic size
		int size=data_size;
		int sockfd;
		struct sockaddr_in servaddr;
		int len;
		char data_sizes[6]={0};
		char buf[BUFSIZ];
		//struct timeval tv_start;
		//struct timeval tv_end;
		
		//Init sockaddr_in
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		//IP address
		servaddr.sin_addr.s_addr=inet_addr(IPaddress);
		//Port number
		servaddr.sin_port=htons(subport);
		
		//Convert int to char*
		sprintf(data_sizes,"%d",size);
		
		//Init socket
		if((sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
		{
			perror("socket error\n");  
			return;  
		}
		else{
			sockfds[i].size=data_size;
			sockfds[i].sockfd=sockfd;
			sockfds[i].id = id;
		}
	
	set_recv_window(sockfd, 512000);
	set_send_window(sockfd, 32000);
		
	//Establish connection
		if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))<0)
		{
			fprintf(stderr, "Can not connect to [%d] %s: %s\n", id, IPaddress, strerror(errno));
			continue;
		}
	}
	gettimeofday(&tv_start_total,NULL);

	for(i=0;i<connections;i++)
	{	
		if(pthread_create(&client_threads[i], NULL , client_thread_func , (void*)&sockfds[i]) < 0)
		{
			perror("could not create client thread");
		}	
	}
	
	for(i=0;i<connections;i++)
	{
		pthread_join(client_threads[i], NULL);  
	}
	gettimeofday(&tv_end_total,NULL);
	//Time interval (unit: microsecond)
	unsigned long interval=(tv_end_total.tv_sec-tv_start_total.tv_sec)*1000000+(tv_end_total.tv_usec-tv_start_total.tv_usec);
	//KB->bit 1024*8
	float throughput=data_size*connections*1024*8.0/interval;

	FILE *fpResult;
	if(ifWriteNewResultFile)
		fpResult=fopen(resultFileName,"w");
	else
		fpResult=fopen(resultFileName,"a+");
	if(fpResult==NULL)
	{
		printf("open file %s failed!\n",resultFileName);
        exit(1);
	}

	///Thoughput(Mbps), FCT(us)
	fprintf(fpResult,"%.1f\t%lu\n"
		,throughput
		,interval);
	fflush(fpResult);
	fclose(fpResult);

	free(sockfds);
	free(incast_connections);
	free(client_threads);
	return 0;
}

void* client_thread_func(void* sockfd_ptr)
{
	struct sockfd_size subsock =*(struct sockfd_size *)sockfd_ptr;
	//Get ID
	
	int len;
	char data_size[6]={0};
	char buf[BUFSIZ];
	struct timeval tv_start;
	struct timeval tv_end;
	int size = subsock.size;
	int sockfd = subsock.sockfd;
	int id = subsock.id;
	
	//Get start time
	sprintf(data_size,"%d",size);
	gettimeofday(&tv_start,NULL);
	
	int totalLen=0;
	//Send Request
	len=send(sockfd,data_size,strlen(data_size),0);
	if(len<=0)
	{
		fprintf(stderr, "[%d] %s can not send request: %s\n", id, IPaddress, strerror(errno));
		close(sockfd);
		exit(1);
	}
	//Receive data

	int lastPrintCount=0;
	int printStep=(float)atoi(data_size)*1024/100;
	while(1)
	{
		len=recv(sockfd,buf,BUFSIZ,0);
		totalLen=totalLen+len;

		if(totalLen>lastPrintCount*printStep)
		{
			printf("[%d] %s: received %d bytes now\n", id, IPaddress, totalLen);
			fflush(stdout);
			lastPrintCount=(int)totalLen/printStep+1;
		}
		if(len<=0 || totalLen>=atoi(data_size)*1024)
			break;
	}

	if(totalLen<atoi(data_size)*1024)
	{
		fprintf(stderr, "[%d] %s has not received all data: %s\n", id, IPaddress, strerror(errno));
		close(sockfd);
		exit(1);
	}
	
	//Get end time
	gettimeofday(&tv_end,NULL);
	
	//Time interval (unit: microsecond)
	unsigned long interval=(tv_end.tv_sec-tv_start.tv_sec)*1000000+(tv_end.tv_usec-tv_start.tv_usec);
	//KB->bit 1024*8
	float throughput=size*1024*8.0/interval;
	
	//Print throughput information 
	printf("[%d] %s 0-%lu ms, %d KB, %.1f Mbps\n",id,IPaddress,interval/1000,size,throughput); 
	fflush(stdout);
	return((void *)0);
}

void set_recv_window(int sockfd, int rcvbuf)
{
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(rcvbuf));
}

void set_send_window(int sockfd, int sndbuf)
{
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(sndbuf));
}

void usage()
{
	printf("./client.o [connections] [data_size (KB)] [port] [log result file] [0|1: if write to a new file? write (1) or append (0) to the result file.)] destIP\n");
}