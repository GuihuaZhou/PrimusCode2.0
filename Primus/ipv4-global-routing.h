#include <stddef.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <time.h> 
#include <sys/time.h> 
#include <sys/types.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <ifaddrs.h>  
#include <unistd.h>
#include <net/if.h> 
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <iomanip>
#include "udp.h"
#include "tcp.h"
#include <strings.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include "/usr/include/net/if_arp.h"

using namespace std;

class Ipv4GlobalRouting
{
public:
  struct linktableentry//master链路表条目
  {
    ident high;
    ident low;
    bool linkFlag;
    bool lastLinkFlag;// 链路震荡时最新的链路状态
    double lastUpdateTime;//记录该条链路上一次更新的时间戳
    double linkUpdateTimer;//默认10ms内同一条链路的其他update信息不会被广播出去
    bool isStartTimer;
    struct linktableentry *next;
  };

  // 和master直连失效，需要中间node代为转发，即需要一条可用的路径
  // 每个下一跳的都选一条最短的备用路径
  // struct nodeconmasterbynode
  // {
  //   struct pathtableentry *address;
  // };

  struct pathtableentryaddress
  {
    struct pathtableentry *address;
    struct pathtableentryaddress *next;
  };

  struct mappingtableentry//映射表条目
  {
    ident high;
    ident low;
    struct pathtableentry *address;
  };

  struct threadparamA
  {
    Ipv4GlobalRouting* tempGlobalRouting;
    struct linktableentry *tempLinkTableEntry;
  };

  struct threadparamB
  {
    Ipv4GlobalRouting* tempGlobalRouting;
    UDPClient *tempUdpClient;
    TCPRoute *tempTcp;
  };

  // sonic test
  bool isListenNIC(string ifName);
  vector<string> GetListenNIC();
  // end

  Ipv4GlobalRouting(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultLinkTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting);
  ~Ipv4GlobalRouting();
  // 通用
  ident GetMyIdent();
  static string GetNow();//获取当前时间，精确到秒
  double GetSystemTime();//获取系统时间
  bool IsLegalNeighbor(struct NICinfo tempNICInfo);
  bool IsDetectNeighbor(struct sockaddr_in addr);
  bool SameNode(ident nodeA,ident nodeB);//判断两个Node是否相同
  bool IsLegalPathInfo(ident tempPathNodeIdent[],ident neighborIdent);
  void GetLocalAddrByRemoteAddr(struct sockaddr_in *localAddr,struct sockaddr_in remoteAddr);
  string GetNICNameByRemoteAddr(struct sockaddr_in addr);
  ident GetNeighborIdentByRemoteAddr(struct sockaddr_in addr);
  unsigned long Convert(struct sockaddr_in destAddr,unsigned int prefixLen);//获得网络号
  unsigned int NetmaskToPrefixlen(struct sockaddr_in destMask);//掩码到前缀长度
  // node ND
  static void* ListenInterfaceThread(void* object);
  void Start(vector<string> tempMasterAddress);
  bool IsNewNIC(struct ifaddrs *ifa);
  void ListenInterfacesAndSubmit();
  void HandleMessage(ident nodeA,ident nodeB,bool linkFlag);
  void NDRecvHello(struct NDinfo tempNDInfo);
  void NDRecvACK(struct NDinfo tempNDInfo);
  void FreshNeighboorList(struct NICinfo tempNICInfo);
  void FreshPathTable(struct pathinfo *tempPathInfo,struct sockaddr_in remote_addr);
  // 修改路由
  int rta_addattr_l(struct rtattr *rta,size_t maxlen,int type,void *data,size_t alen);
  int addattr_l(struct nlmsghdr *n,size_t maxlen,int type,void *data,size_t alen);
  int addattr32 (struct nlmsghdr *n,size_t maxlen,int type,int data);
  int OpenNetlink();
  void AddMultiRoute(struct sockaddr_in destAddr,unsigned int prefixLen,vector<struct nexthopandweight> nextHopAndWeight);//添加多路径路由
  void AddSingleRoute(struct sockaddr_in destAddr,unsigned int prefixLen,struct nexthopandweight tempNextHopAndWeight);
  void DelRoute(struct sockaddr_in destAddr,unsigned int prefixLen);
  // node 对路径表的操作
  void PrintPathEntryTable();//打印路径表
  void PrintPathEntry(struct pathtableentry *tempPathTableEntry);
  void PrintMappingTable();//打印映射表
  void PrintNodeConMasterByNode();//打印备用路径
  void PrintMasterMapToSock();//记录node和master的连接情况

  void GetHeadPathTableEntry(struct pathtableentry **tempPathTableEntry);//获得路径表的第一条路径
  void UpdateAddrSet(ident pathNodeIdentA,ident pathNodeIdentB,int nodeCounter,vector<struct addrset> addrSet);
  void UpdateMappingTableEntry(struct pathtableentry *newPathTableEntry);//更新映射表
  void GetMappingTableEntry(ident pathNodeIdentA,ident pathNodeIdentB,vector<struct mappingtableentry> *tempMappingTable);//根据两个节点(link)获取映射表项，也有可能是根据一个节点来获取所有相关的映射表项
  void InsertPathTable(struct pathtableentry *currentPathTableEntry,struct pathtableentry *nextPathTableEntry,struct pathtableentry *newPathTableEntry);//插入一条路径
  void PathAddNewAddr(struct pathtableentry *nextPathTableEntry,struct pathaddrset *tempPathAddrSet);//相同的路径增加服务器地址
  void AddNewPathTableEntry(struct pathinfo tempPathInfo);//Node调用此函数添加新的路径
  void ModifyNodeDirConFlag(ident high,ident low,bool dirConFlag);// 修改dirConFlag
  void ModifyPathEntryTable(ident high,ident low,bool linkFlag);// 修改路径表项，linkCounter
  void UpdateNodeConMasterByNode(struct pathtableentry *tempPathTableEntry);//更新node和master间接连接的路径
  void UpdateMasterMapToSock(struct mastermaptosock tempMasterMapToSock,int cmd);//node连接的Master的一些信息
  vector<struct mastermaptosock> GetMasterMapToSock();//获得node和master的连接信息
  vector<struct nodemaptosock> GetNodeMapToSock();

  double Test(bool isMaster);//Master查找和修改链路表的时间测试，测试Node查找映射表、查找和修改路径表的时间

  // master
  static void* MasterLinkTimer(void* threadParam);
  void PrintMasterLinkTable();
  void PrintNodeMapToSock();
  bool UpdateMasterLinkTable(ident high,ident low,bool linkFlag);
  void UpdateNodeMapToSock(ident nodeIdent,int sock,bool direct);
  int GetSockByIdent(ident nodeIdent);// 通过ident获得套接字
  ident GetIdentBySock(int sock);// 通过套接字获得ident
  void GetEffectNode(vector<ident> tempNode,string type,vector<ident> *tempEffectNode,struct MNinfo *tempLinkInfo);// 求出nodeIdent的上行或者下行链路的另一端结点的ident
  void SendMessageToNode(ident high,ident low,bool linkFlag);

private:
  // 预期拓扑
  int m_ToRNodes;//第一层每个Pod结点的数量
  int m_LeafNodes;//第二层每个Pod结点的数量
  int m_SpineNodes;//第三层结点的数量
  int m_nPods;//Pod的数量
  int m_Pod;//是第几个Pod
  int m_nMaster;//master数量
  int m_Links=10;//Link总数
  ident myIdent;
  int m_defaultLinkTimer;//链路定时器，避免震荡
  bool m_IsCenterRouting=false;
  bool m_randomEcmpRouting=false; 

  bool ChiefMaster;
  int m_defaultKeepaliveTimer=3;
  vector<struct NICinfo> NICInfo;//状态为up的网卡信息存储数组
  vector<int> MasterSock;//存储所有和Master连接的套接字
  TCPRoute *m_tcp;
  UDPClient m_udpClient;//udp to broadcast (only node not master)
  UDPServer m_udpServer;//udp to deal broadcast (only node not master)
  //master和转发nodo用来存储Node和套接字之间的映射关系
  vector<struct nodemaptosock> nodeMapToSock;
  // 在sonic上虚拟出多个交换机时使用，
  vector<string> listenNICName;//只能监听物理机上的部分网卡
  vector<struct mastermaptosock> masterMapToSock;//node记录自己和master连接的套接字
  vector<struct pathtableentry *> nodeConMasterByNode;//node自己选的和master间接连接的路径
  vector<string> masterAddress;
  
  vector<struct mappingtableentry> mappingEntryTable;
  struct linktableentry *headLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));//master链路表的头节点
  struct pathtableentry *headPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));//头节点中的counter用作链表长度计数器
  // 路径表索引
  vector<struct pathtableentry *> pathEntryAddressIndexOne;
  vector<struct pathtableentry *> pathEntryAddressIndexTwo;
  vector<struct pathtableentry *> pathEntryAddressIndexThree;
  
  pthread_t masterLink_thread;
};