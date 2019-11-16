#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <errno.h>
#include <time.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/socket.h>
#include <ifaddrs.h>  
#include <unistd.h>
#include <net/if.h> 
#include <sys/ioctl.h>
#include <linux/rtnetlink.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <iomanip>

#include "ipv4-global-routing.h"
#include "tcp-server-route.h"
#include "tcp-client-route.h"

using namespace std;
#define SIZE 20

class Node{

struct nodemaptosock
{
  ident node;
  int nodeSock;
  int keepAliveFaildNum;//未接收到的keep alive数量
  bool keepAliveFlag;
};

struct rtm{
  char m_routeData[SIZE];//路由目的地址
  char m_routeOif[SIZE];//输入口
  char m_routeProtocal;//路由协议
};

public:
 
  Node();
  Node(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultMasterTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting);
  void Setmyident(int pre,int suf);
  ident Getmyident();
  void SetRole(string masterAddress,int masterPort);
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

  //add by pannian
  int rtnl_receive(int fd, struct msghdr *msg, int flags);
  static int rtnl_recvmsg(int fd, struct msghdr *msg, char **answer);
  void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);
  static inline int rtm_get_table(struct rtmsg *r, struct rtattr **tb);
  void print_route(struct nlmsghdr* nl_header_answer);
  int open_netlink();
  int do_route_dump_requst(int sock);
  int get_route_dump_response(int sock);
  void SendRouteToMaster();
  //end 

  static string GetNow();
  
private:
  pthread_t NIC_thread;//A thread to deal with client_sockfd
  int Pod;//属于哪个Pod
  ident myident;//标识自己是哪个交换机
  bool m_isMaster;
  int m_defaultKeepaliveTimer=3;
  //char m_routeProtocal = 0;

  Ipv4GlobalRouting *m_globalRouting;

  TCPClientRoute m_tcpClient;//tcp to submit the link status to master if m_isMaster = false;
  TCPServerRoute m_tcpServer;//tcp to deal with the link status and sent to switchs if m_isMaster = true;

  //建立Node和套接字之间的映射关系
  std::vector<struct nodemaptosock> nodeMapToSock;
  std::vector<struct rtm> rootMessage;
};