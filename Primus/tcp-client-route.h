/*#ifndef TCP_CLIENT_ROUTE_H
#define TCP_CLIENT_ROUTE_H*/

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
#include <signal.h>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <netinet/ip.h>

class Node;
class TCPClientRoute
{
public:
    struct threadparam
    {
        TCPClientRoute* m_TCPClientRoute;
        int index;
    };

    TCPClientRoute();
    TCPClientRoute(std::string remoteAddr,int remotePort);
    ~TCPClientRoute();
    void SetNode(Node *node,int defaultKeepaliveTimer);

    void SetRemoteAddress(std::string remoteAddr);
    void SetRemotePort(int RemotePort);
    bool StartApplication();
    void StopApplication();
    void SayHelloToMaster(int level,int position);
    void SubmitLinkStatus(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag);
    void SayGoodByeToMaster();
    void KeepAliveTimer(int level,int position);
    
    void SetFillAndSend (std::string fill);
    static string GetNow();

private:
	void Send ();          
	static void* HandleRead(void* sock);//创建线程用
    static void* SayKeepAliveToMaster(void* threadParam);
	static Node *m_node;
	int server_sockfd;//服务器端套接字
    int client_sockfd;//客户端套接字
	int len;
    int m_level;
    int m_position;
    int m_defaultKeepaliveTimer;
    int keepAliveFaildNum=0;
	struct sockaddr_in remote_addr; //客户端网络地址结构体
	int sin_size;
	int* sock=NULL;
	char *m_data;
	int m_dataSize;
	bool * m_isConnected;// whether or not the client successfully connect to server
    bool keepAliveFlag=false;
    pthread_t client_thread;
    pthread_t keepAlive_thread;
};

