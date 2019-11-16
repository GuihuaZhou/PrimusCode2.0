#ifndef TCP_ROUTE
#define TCP_ROUTE
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

using namespace std;

class Ipv4GlobalRouting;
class TCPRoute
{
public:
	TCPRoute();
	TCPRoute(Ipv4GlobalRouting *m_globalRouting,int tempDefaultKeepaliveTimer);
	~TCPRoute();
	static string GetNow();
	void StartListen();// 创建接收数据的线程
	void SendHelloToMaster(vector<string> masterAddress,string middleAddress,string NICName,int port);// port是sonic测试用
	void SendMessageTo(int tempSock,struct MNinfo tempLinkInfo);

private:
	// 变量
	struct threadparam
  {
    TCPRoute *tempTCPRoute;
    int tempSock;
  };
	static Ipv4GlobalRouting *m_globalRouting;
	ident myIdent;
	pthread_t server_thread;
	int m_defaultKeepaliveTimer;
	struct sockaddr_in serverAddr;// 监听本地地址
	// 函数
	static void* ServerThread(void* tempThreadParam);
};

#endif