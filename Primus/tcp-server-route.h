#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <netinet/ip.h>

#define CLIENTSIZE 80

class Node;
class TCPServerRoute
{
public:
    TCPServerRoute();
    TCPServerRoute(int Localport);
    ~TCPServerRoute();
    void SetNode(Node *node,int defaultKeepaliveTimer);

    bool StartApplication();
    void StopApplication ();
    
    void SetLocalPort(int LocalPort);
    void IssueLinkStatus(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag);
    void SetFillAndSend (std::string fill);
    void NoticeToNode(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag,std::vector<int> sock);

    static string GetNow();
    
private:
	
	void HandleRead();
	void Send ();
	static void* ServerThread(void* client_sockfd_ptr);
	static void* KeepAliveThread(void* object);
	static Node *m_node;
	int server_sockfd;//服务器端套接字
	int client_sockfds[CLIENTSIZE];//客户端套接字
	static int client_count;
	int len;
	struct sockaddr_in my_addr;   //服务器网络地址结构体
	struct sockaddr_in remote_addr[CLIENTSIZE]; //客户端网络地址结构体
	unsigned sin_size;
	char buf[CLIENTSIZE][BUFSIZ];  //数据传送的缓冲区

	char *m_data;
	int m_dataSize;
	int m_port;

	pthread_t server_thread;//A thread to deal with client_sockfd
	pthread_t keepalive_thread;

	int* client_sockfd_ptr=NULL;//Socket pointer for client 
	int m_defaultKeepaliveTimer; 
};