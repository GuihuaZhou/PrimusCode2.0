#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <time.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <ifaddrs.h>  
#include <unistd.h>
#include <net/if.h> 
#include <sys/ioctl.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <iomanip>

#include "ipv4-global-routing.h"
#include "tcp-server-route.h"
#include "tcp-client-route.h"
#include "udp-bcast.h"

using namespace std;

class Node{

struct nodemaptosock
{
  ident node;
  int nodeSock;
  int keepAliveFaildNum;//未接收到的keep alive数量
  bool keepAliveFlag;
};

public:
 
  Node();
  Node(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultMasterTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting);
  void Setmyident(int pre,int suf);
  ident Getmyident();
  void Start(string masterAddress,int masterPort);
  static int GetInterfaceStatus(char *name);

  void SetUp (int i);
  void SetDown (int i);
  void SendMessageToMaster (int i,bool interfaceFlag);
  void SendMessageToNode(ident high,ident low,bool flag);
  void IssueInformation(ident source,ident destination,bool flag);
  void RecordNodeMapToSock(int level,int postion,int sock);
     
  void HandleMessage(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag);

  void PrintMasterLink();
  void PrintPathTable();
  void PrintMappingTable();
  static void* ListeningNICProcess(void* i);
  void ListenInterfacesAndSubmit();
  void Test();
  void RecordKeepAliveFlag(int sock);
  void CheckKeepAliveFlag();

  static string GetNow();
  
private:
  pthread_t NIC_thread;//A thread to deal with client_sockfd
  int Pod;//属于哪个Pod
  ident myident;//标识自己是哪个交换机
  bool m_isMaster;
  int m_defaultKeepaliveTimer=3;

  Ipv4GlobalRouting *m_globalRouting;

  TCPClientRoute m_tcpClient;//tcp to submit the link status to master if m_isMaster = false;
  TCPServerRoute m_tcpServer;//tcp to deal with the link status and sent to switchs if m_isMaster = true;
  
  UDPClientBcast m_udpClient;//udp to broadcast (only node not master)
  UDPServerBcast m_udpServer;//udp to deal broadcast (only node not master)
  //建立Node和套接字之间的映射关系
  std::vector<struct nodemaptosock> nodeMapToSock;
};