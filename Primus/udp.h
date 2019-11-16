#ifndef UDP
#define UDP
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include "defs.h"

#define IP_FOUND "IP_FOUND"
#define IP_FOUND_ACK "IP_FOUND_ACK"
#define ND_PORT 2225
#define PT_PORT 8848//传递路径信息
#define MCAST_PORT 2225

#define TTL 1
#define BUF_SIZE 1448
#define MAX_IF 100

using namespace std;

class Ipv4GlobalRouting;
class UDPServer{
public:
	//Set base information
	UDPServer();
	~UDPServer();
	bool StartApplication();
	void SetGlobalRouting(Ipv4GlobalRouting *tempGlobalRouting);

private:
	static void* HandleReadND(void* object);
	static void* HandleReadPathTable(void* object);
	static Ipv4GlobalRouting* m_globalRouting;
	ident myIdent;
};

class UDPClient{
public:
	//Set base information
	UDPClient();
	~UDPClient();
  void SetGlobalRouting(Ipv4GlobalRouting *tempGlobalRouting);
  void SendNDTo(struct sockaddr_in localAddr);
  void SendPathInfoTo(struct sockaddr_in localAddr,struct sockaddr_in remoteAddr,struct pathinfo *tempPathInfo);

private:
	static void* SendNDThread(void* tempLoaclAddr);
	static Ipv4GlobalRouting* m_globalRouting;
	ident myIdent;
};

#endif