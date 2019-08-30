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

#define SEND_MESSAGE_SIZE (100*1024)

//Start TCP server
void start_server(int port);
//Function to deal with an incoming connection
void* server_thread_func(void* client_sockfd_ptr);
//Print usage information
void usage();

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		usage();
		return 0;
	}
	start_server(atoi(argv[1]));
	return 0;
}

void usage()
{
	printf("./server.o [port]\n");
}
void start_server(int port)
{
	int totalClientConnectionCount=0;

	//Socket for server
	int server_sockfd; 
	//Socket pointer for client 
	int* client_sockfd_ptr=NULL;
	//A thread to deal with client_sockfd
	pthread_t server_thread;
	//Server address
	struct sockaddr_in server_addr;
	//Client address
	struct sockaddr_in client_addr;

	memset(&server_addr,0,sizeof(server_addr)); 
	//IP protocol
    server_addr.sin_family=AF_INET;
	//Listen on "0.0.0.0" (Any IP address of this host)
    server_addr.sin_addr.s_addr=INADDR_ANY;
	//Specify port number
    server_addr.sin_port=htons(port); 
	
	//Init socket
	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)  
	{    
		perror("socket error");  
		return;  
	}

	setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,(int[]){1}, sizeof(int));
	int flag = 1;
	int result = setsockopt(server_sockfd,            /* socket affected */
	                        IPPROTO_TCP,     /* set option at TCP level */
	                        TCP_NODELAY,     /* name of option */
	                        (char *) &flag,  /* the cast is historical cruft */
	                        sizeof(int));    /* length of option value */
	if (result < 0)
	{
		perror("setsockopt TCP_NODELAY error\n");  
		return; 
	}
	setsockopt(server_sockfd, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));

	//Bind socket on IP:Port
	if(bind(server_sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))<0)  
	{  
		perror("bind error");  
		return;  
	}
	
	//setsockopt(SO_REUSEADDR);

	//Start listen
	//The maximum number of concurrent connections is 200
	listen(server_sockfd,200);  
	socklen_t sin_size=sizeof(struct sockaddr_in); 
	
	while(1)
	{
		client_sockfd_ptr=(int*)malloc(sizeof(int));
		int value=accept(server_sockfd,(struct sockaddr *)&client_addr,&sin_size);
		if(value<0)  
		{  
			perror("accept error");  
			free(client_sockfd_ptr);
			return;  
		}  
		printf("%d client conection in\n",totalClientConnectionCount);
		fflush(stdout);
		totalClientConnectionCount++;

		*client_sockfd_ptr=value;
		if(pthread_create(&server_thread, NULL , server_thread_func, (void*)client_sockfd_ptr) < 0)
		{
			perror("could not create thread");
			return;
		}	
		// server_thread_func((void*)client_sockfd_ptr);
	}
}

void* server_thread_func(void* client_sockfd_ptr)
{
	int i;
	int sock=*(int*)client_sockfd_ptr;
	char write_message[SEND_MESSAGE_SIZE+1];
	char read_message[1024]={0};
	int len;
	int data_size;
	int remaining_size;
	int loop;
	struct sockaddr_in client_addr;
	int client_addr_len=sizeof(client_addr);
	getpeername(sock, (struct sockaddr *)&client_addr, &client_addr_len);
	char IPaddress[20];
	inet_ntop(AF_INET, &client_addr.sin_addr, IPaddress, sizeof(IPaddress));

	int flag = 1;
	int result = setsockopt(sock,            /* socket affected */
	                        IPPROTO_TCP,     /* set option at TCP level */
	                        TCP_NODELAY,     /* name of option */
	                        (char *) &flag,  /* the cast is historical cruft */
	                        sizeof(int));    /* length of option value */
	if (result < 0)
	{
		fprintf(stderr, "%s setsockopt TCP_NODELAY error\n", IPaddress);  
		return((void *)0); 
	}
	
	free(client_sockfd_ptr);

	int requestID=0;

	while(1)
	{
		memset(read_message,0,1024);
		len=recv(sock,read_message,1024,0);
		if(len<=0)
		{
			fprintf(stderr, "[%d] %s can not recv request\n", requestID, IPaddress);  
			close(sock);
			return((void *)0);
		}
		setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));

		//Get data volume (Unit:KB)
		data_size=atoi(read_message);
		//Calculate loops. In each loop, we can send SEND_MESSAGE_SIZE (128K) bytes of data
		loop=data_size*1024/SEND_MESSAGE_SIZE;
		//Calculate remaining size to be sent 
		remaining_size=data_size*1024-loop*SEND_MESSAGE_SIZE;

		memset(write_message,1,SEND_MESSAGE_SIZE);
		write_message[SEND_MESSAGE_SIZE]='\0';
		//Send data with 8192 bytes each loop
		for(i=0;i<loop;i++)
		{
			if(send(sock,write_message,strlen(write_message),0)<=0)
			{
				fprintf(stderr, "[%d] %s can not send response data: %s\n", requestID, IPaddress, strerror(errno));
				close(sock);
				return((void *)0);
			}
			setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
		}
		//Send remaining data

		if(remaining_size>0)
		{
			memset(write_message,1,SEND_MESSAGE_SIZE);
			write_message[remaining_size]='\0';
			if(send(sock,write_message,strlen(write_message),0)<=0)
			{
				fprintf(stderr, "[%d] %s can not send response data: %s\n", requestID, IPaddress, strerror(errno));
				close(sock);
				return((void *)0);
			}
			setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
		}
		requestID++;
	}
	
	return((void *)0);
}

