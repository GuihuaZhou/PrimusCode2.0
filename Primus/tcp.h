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
	TCPRoute(Ipv4GlobalRouting *m_globalRouting,int tempDefaultKeepaliveTimer,bool tempChiefMaster);
	~TCPRoute();
	static string GetNow();
	void UpdateChiefMaster(bool tempChiefMaster);
	void StartListen();// 创建接收数据的线程
	void SendHelloToChief(string masterAddress,string NICName);
	void SendHelloToMaster(vector<string> masterAddress,string middleAddress,string NICName);// port是sonic测试用
	int SendMessageTo(int tempSock,struct MNinfo tempLinkInfo);

private:
	// 变量
	struct threadparamA
  {
    TCPRoute *tempTCPRoute;
    sockaddr_in tempAddr;
    int tempSock;
  };
  struct threadparamB
  {
    TCPRoute *tempTCPRoute;
    string masterAddress;
    string NICName;
  };
	static Ipv4GlobalRouting *m_globalRouting;
	ident myIdent;
	bool chiefMaster;
	pthread_t server_thread;
	pthread_t hellotochief_thread;
	int m_defaultKeepaliveTimer;
	struct sockaddr_in serverAddr;// 监听本地地址
	// 函数
	static void* ServerThread(void* tempThreadParam);
	static void* SendHelloToChiefThread(void* tempThreadParam);
};

#endif