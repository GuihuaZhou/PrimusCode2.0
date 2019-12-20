#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fstream>
#include<iostream>
#include<sstream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<netdb.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<pthread.h>
#include"udp-bcast.h"

string GetNow(){
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );
  /*cout << tt << endl;*/

  /*printf("[%d-%02d-%02d %02d:%02d:%02d]:",
           t->tm_year + 1900,
           t->tm_mon + 1,
           t->tm_mday,
           t->tm_hour,
           t->tm_min,
           t->tm_sec);*/
  /*string time = to_string(t->tm_hour) +":"+ to_string(t->tm_min) +":"+ to_string(t->tm_sec);*/
  stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

Node *
UDPServerBcast::m_node=NULL;
UDPServerBcast::UDPServerBcast()
{
	
	

}
UDPServerBcast::~UDPServerBcast()
{
	
}
bool
UDPServerBcast::StartApplication()
{   
	//create thread and wait for packets 
	ofstream Logfout("Bcast.log",ios::app);
    pthread_t selectThread;
    /*pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);*/
    if(pthread_create(&selectThread, 0, HandleRead, NULL) != 0)
    {
    	Logfout<<GetNow()<<"UDPServerBcast::StartApplication (): Create Thread Failed."<<std::endl;
    	return false;
    }
    //将新线程与主线程分离，主线程不需要等待新线程执行完毕再接着运行
    pthread_detach(selectThread);
    Logfout.close();
    return true;
}
void
UDPServerBcast::SetNode(Node *node)
{
    m_node = node;
}


void*
UDPServerBcast::HandleRead(void* object)
{
    
	int ret = -1;
	int from_len;
	int sock = -1;
    struct sockaddr_in serverAddr;//服务端地址
	struct sockaddr_in fromAddr;//客户端地址
	ofstream Logfout("Bcast.log",ios::app);
	
	bzero(&serverAddr,sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;//Set as IP communication
	serverAddr.sin_addr.s_addr = htons(INADDR_ANY);//server IP address:allowed to connect any local address
	serverAddr.sin_port = htons(PORT);//server port
    
	//广播地址
	bzero(&fromAddr,sizeof(struct sockaddr_in));
	fromAddr.sin_family = AF_INET;//Set as IP communication
	fromAddr.sin_addr.s_addr = htons(INADDR_ANY);//server IP address:allowed to connect any local address
	fromAddr.sin_port = htons(PORT);//server port
	//set dgram for server

    if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	    Logfout << GetNow() << "UDPServerBcast::HandleRead (): Create Socket Failed." << std::endl;
	    exit(1);
	}

	int flag = 1;
	if((ret = setsockopt(sock, SOL_SOCKET,SO_BROADCAST,&flag,sizeof(flag))) < 0)
	{
		Logfout<<GetNow()<<"UDPServerBcast::HandleRead (): Set Socket Opt Failed."<<std::endl;
    	exit(1);
	}
	//bind 
	if((ret = bind(sock,(struct sockaddr*) &serverAddr,sizeof(serverAddr))) < 0)
	{
		Logfout<<GetNow()<<"UDPServerBcast::HandleRead (): Bind Socket Failed."<<std::endl;
    	exit(1);
	}

	while(1)
	{
		char buffer[BUF_SIZE];
		if((ret = recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr *)& fromAddr,(socklen_t*)&from_len)) < 0)
		{
			Logfout<<GetNow()<<"UDPServerBcast::HandleRead (): Socket recvfrom Failed."<<std::endl;
			break;
		}
		//排除本地广播
		if(strstr((char *)inet_ntoa(fromAddr.sin_addr), "127.0.0.1"))
		{
			continue;
		}
		printf("\nClient connection information:\n\t IP: %s, Port: %d\n",
							(char *)inet_ntoa(fromAddr.sin_addr),
							ntohs(fromAddr.sin_port));
        
		// m_node->GetAdjacency();

		memcpy(buffer, IP_FOUND_ACK, strlen(IP_FOUND_ACK) + 1);
		sendto(sock, buffer, strlen(buffer), 0,(struct sockaddr*) &fromAddr, from_len);
/*		if((len = sendto(sock, buffer, strlen(buffer), 0,(struct sockaddr*) &fromAddr, from_len)) < 0)
		{
			Logfout<<GetNow()<<"UDPServerBcast::HandleRead (): Socket SendTo Failed."<<std::endl;
			break;
		}*/

	}
	close(sock);
	Logfout.close();
	
}






















Node *
UDPClientBcast::m_node=NULL;

UDPClientBcast::UDPClientBcast()
{

}
UDPClientBcast::~UDPClientBcast()
{
	
}
bool
UDPClientBcast::StartApplication()
{   
	SayHelloToAdjacency();
	
    return true;
}
void
UDPClientBcast::SetNode(Node *node)
{
    m_node = node;
}

void
UDPClientBcast::SayHelloToAdjacency()
{
	//create thread and wait for packets 
	// int* sockPtr = (int*)malloc(sizeof(int*));
	// *sockPtr = sock; 
	ofstream Logfout("Bcast.log",ios::app);
	pthread_t selectThread;
	if(pthread_create(&selectThread, NULL, SayHelloThread, NULL) < 0)
	{
	    Logfout<<GetNow()<<"UDPClientBcast::SayHelloToAdjacency (): Create Thread Failed."<<std::endl;
	    return;
	}
	pthread_detach(selectThread);
	Logfout.close();
}
void*
UDPClientBcast::SayHelloThread(void* object)
{
	int sock =  -1;
	int ret = -1;
	int so_broadcast = 1;
	struct sockaddr_in bcastAddr;//广播地址
	struct sockaddr_in fromAddr;//服务端端地址
	int from_len;
	char buffer[BUF_SIZE];
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	ofstream Logfout("Bcast.log",ios::app);

    //set dgram for client
    if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	    Logfout << GetNow() << "UDPClientBcast::SayHelloToAdjacency (): Create Socket Failed." << std::endl;
	    exit(0);
	}
	//the default socket doesn't support broadcast,so we should set socket discriptor to do it.
	if((ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast))) < 0)
	{
		Logfout<<GetNow()<<"UDPClientBcast::SayHelloToAdjacency (): SetSocketOpt Failed."<<std::endl;
    	exit(0);
	}
	int time_live = TTL;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));
	bzero(&bcastAddr,sizeof(struct sockaddr_in));
	bcastAddr.sin_family = AF_INET;
	bcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	bcastAddr.sin_port = htons(MCAST_PORT);
	int i;
	for(i = 0; i < 3; i++)
	{
		//send
	    if(sendto(sock,IP_FOUND,strlen(IP_FOUND),0,(struct sockaddr *)&bcastAddr,sizeof(bcastAddr))<0)
	    {
	    	Logfout<<GetNow()<<"UDPClientBcast::SayHelloToAdjacency (): SentTo Failed."<<std::endl;
	    	exit(0);
	    }
	    //set receive timeout
		setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
		bzero(&fromAddr,sizeof(struct sockaddr_in));
	    if(recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr*) &fromAddr,(socklen_t*)&from_len)<0)
		{
			Logfout<<GetNow()<<"UDPClientBcast::SayHelloToAdjacency (): Recvfrom Timeout."<<std::endl;
			continue;
		}
		printf("\tfound server IP is %s, Port is %d\n",
							inet_ntoa(fromAddr.sin_addr),
							htons(fromAddr.sin_port));
	    
	}
	close(sock);
    Logfout.close();
}
void*
UDPClientBcast::HandleRead(void* object)
{
	int sock = *(int *) object;
	struct sockaddr_in fromAddr;//服务端端地址
	int from_len;
	int count = -1;
	char buffer[BUF_SIZE];
	while(1)
	{
		if((count = recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr*) &fromAddr,(socklen_t*)&from_len))<0)
		{
			continue;
		}
		printf("\tfound server IP is %s, Port is %d\n",
							inet_ntoa(fromAddr.sin_addr),
							htons(fromAddr.sin_port));
/*		if(strstr(buffer, IP_FOUND_ACK))
		{
			
		}*/
		sleep(1);

	}
}

