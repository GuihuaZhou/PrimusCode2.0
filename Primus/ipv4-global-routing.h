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
  struct linktableentry// master链路表条目
  {
    // 对链路两端的ident重新进行排序，level大的为high，若level相等，则position大的为high
    ident high;
    ident low;
    bool linkFlag;// 当前链路的状态
    bool lastLinkFlag;// 链路震荡时最新的链路状态
    double lastUpdateTime;// 记录该条链路上一次更新的时间戳
    double linkUpdateTimer;// 定时器，即抑制时间，在这个时间内，同一条链路的其他更新信息不会被Master处理
    bool isStartTimer;// 是否已经开启定时器
    struct linktableentry *next;
  };

  struct pathtableentryaddress
  {
    struct pathtableentry *address;
    struct pathtableentryaddress *next;
  };

  struct mappingtableentry// 映射表表项
  {
    ident high;
    ident low;
    struct pathtableentry *address;// 该链路影响的路径块中第一条路径的地址
    struct mappingtableentry *next;
  };

  struct threadparamA
  {
    Ipv4GlobalRouting *tempGlobalRouting;
    struct linktableentry *tempLinkTableEntry;
  };

  struct threadparamB
  {
    Ipv4GlobalRouting *tempGlobalRouting;
    UDPClient *tempUdpClient;
    TCPRoute *tempTCPRoute;
  };

  struct threadparamC
  {
    Ipv4GlobalRouting *tempGlobalRouting;
    TCPRoute *tempTCPRoute;
    ident masterIdent;
  };

  // sonic test
  bool isListenNIC(string ifName);
  vector<string> GetListenNIC();
  // end

  Ipv4GlobalRouting(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultLinkTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting);
  ~Ipv4GlobalRouting();

  // 通用
  static void* KeepAliveThread(void* tempThreadParam);
  ident GetMyIdent();
  int GetSockByAddress(string tempAddress);
  int GetSockByMasterIdent(ident masterIdent);
  static string GetNow();//获取当前时间，精确到秒
  double GetSystemTime();//获取系统时间
  string GetMasterAddrByIdent(ident masterIdent);
  bool IsLegalMaster(string masterAddress);// 检查和master的连接是否合法
  bool IsLegalNeighbor(struct NICinfo tempNICInfo);
  bool IsDetectNeighbor(struct sockaddr_in addr);
  bool SameNode(ident nodeA,ident nodeB);//判断两个Node是否相同
  bool IsLegalPathInfo(ident tempPathNodeIdent[],ident neighborIdent);
  void CloseSock(int sock);
  void GetLocalAddrByRemoteAddr(struct sockaddr_in *localAddr,struct sockaddr_in remoteAddr);
  void GetLocalAddrByNeighborIdent(struct sockaddr_in *localAddr,ident neighborIdent);
  struct sockaddr_in *GetAddrByNICName(string NICName);// 通过rtnetlink获取网口的地址
  string GetNICNameByRemoteAddr(struct sockaddr_in addr);
  ident GetNeighborIdentByRemoteAddr(struct sockaddr_in addr);
  ident GetChiefMasterIdent();
  unsigned long Convert(struct sockaddr_in destAddr,unsigned int prefixLen);//获得网络号
  unsigned int NetmaskToPrefixlen(struct sockaddr_in destMask);//掩码到前缀长度
  // node ND
  static void* ListenNICThread(void* tempThreadParam);
  void Start(vector<string> tempMasterAddress);
  bool IsNewNeighbor(string NICName);
  bool IsNewNIC(struct ifaddrs *ifa);
  void ListenNIC();
  void HandleMessage(ident nodeA,ident nodeB,bool linkFlag);
  void NDRecvND(struct NDinfo tempNDInfo);
  void NDRecvACK(struct NDinfo tempNDInfo);
  void FreshNeighboorList(struct NICinfo tempNICInfo);
  void FreshPathTable(struct pathinfo *tempPathInfo,struct sockaddr_in remote_addr);
  // 修改路由
  void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);
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
  void PrintMappingTableIndex();//打印映射表的索引表
  void PrintNodeInDirPathTable();//打印备用路径
  void PrintMasterMapToSock();//记录node和master的连接情况

  struct pathtableentry *InformUnreachableNode(ident destIdent,ident srcIdent);
  void GetHeadPathTableEntry(struct pathtableentry **tempPathTableEntry);//获得路径表的第一条路径
  void UpdateAddrSet(ident pathNodeIdentA,ident pathNodeIdentB,int nodeCounter,vector<struct addrset> addrSet);
  void UpdateMappingTableEntry(struct pathtableentry *newPathTableEntry);//更新映射表
  void GetMappingTableEntry(ident pathNodeIdentA,ident pathNodeIdentB,vector<struct mappingtableentry> *tempMappingTable);//根据两个节点(link)获取映射表项，也有可能是根据一个节点来获取所有相关的映射表项
  void InsertPathTable(struct pathtableentry *currentPathTableEntry,struct pathtableentry *nextPathTableEntry,struct pathtableentry *newPathTableEntry);//插入一条路径
  void InsertMappingTable(struct mappingtableentry *currentMappingTableEntry,struct mappingtableentry *nextMappingTableEntry,struct mappingtableentry *newMappingTableEntry);
  void PathAddNewAddr(struct pathtableentry *nextPathTableEntry,struct pathaddrset *tempPathAddrSet);//相同的路径增加服务器地址
  void AddNewPathTableEntry(struct pathinfo tempPathInfo);//Node调用此函数添加新的路径
  void ModifyNodeDirConFlag(ident high,ident low,bool dirConFlag);// 修改dirConFlag
  void ModifyPathEntryTable(ident high,ident low,bool linkFlag);// 修改路径表项，linkCounter
  void InsertNodeInDirPathTable(struct pathtableentry *tempPathTableEntry);//插入node和master间接连接的路径
  void CheckMasterMapToSock();// 更新完路径表中的链路或者直连标志后都要检查和master的连接的间接路径是否受到影响
  void UpdateNodeInDirPathTable();// 修改路径表或者直连标志位后，需要检查间接路径表是否需要更新
  void UpdateMasterMapToSock(struct mastermaptosock tempMasterMapToSock,int cmd);//node连接的Master的一些信息
  struct pathtableentry *GetNodePathToMaster(struct pathtableentry *lastPath);// 获得一条通往master的路径，可能是直连，可能是间接连接
  vector<struct mastermaptosock> GetMasterMapToSock();// 获得node和master的连接信息
  vector<struct nodemaptosock> GetNodeMapToSock();
  int GetMappingTableEntryIndex(ident high);// 获取映射表的索引，返回值就是索引的位置
  int UpdateKeepAlive(int masterSock,ident masterIdent,bool recvKeepAlive);// keepAliveFlag为真，则表示master回复了keep alive；为假，则表示node发出了keep alive
  // 返回值为1表示正常，为2表示keepalive未接收次数超过3次，为3表示套接字不存在

  double Test(bool isMaster);//Master查找和修改链路表的时间测试，测试Node查找映射表、查找和修改路径表的时间

  // master
  static void* MasterLinkTimer(void* threadParam);
  static void* ListenKeepAliveThread(void* threadParam);
  static void* OldChiefFindNewOneThread(void* threadParam);
  ident GetIdentBySock(int sock);// 通过套接字获得ident
  void PrintClusterMasterInfo();
  void PrintMasterLinkTable();
  void PrintNodeMapToSock();
  void PrintMasterInDirPathTable();
  int GetSockByIdent(ident nodeIdent);// 通过ident获得套接字
  int GetInDirNodeNum();// 获得间接连接的Node数量
  bool IsUnreachableInDirNode(ident tempNode,vector<ident> effectInDirNode,struct MNinfo tempMNInfo);// 判断Node是否为不可达的间接node
  bool UpdateMasterLinkTable(ident high,ident low,bool linkFlag);
  void UpdateInDirPath(ident nodeIdent,ident pathNodeIdent[MAX_PATH_LEN],int nodeSock);
  void UpdateNodeMapToSock(ident nodeIdent,int sock,bool direct);
  void UpdateKeepAliveFaildNum(ident nodeIdent);
  void UpdateClusterMasterInfo(struct clustermasterinfo tempClusterMasterMapToSock,int cmd);
  void SendInDirNodeNumToCommon(struct clustermasterinfo tempClusterMasterMapToSock);// master向其他master转发indirnodenum
  void GetEffectNode(vector<ident> effectInDirNode,vector<ident> tempNode,ident noNeedToNotice,string type,vector<ident> *tempEffectNode,struct MNinfo tempMNInfo);// 求出nodeIdent的上行或者下行链路的另一端结点的ident
  void SendMessageToNode(ident high,ident low,bool linkFlag);
  void NewChiefMasterElection(ident chiefMasterIdent);
  void ChooseNodeToInformInDirNode(ident destIdent,ident lastNode,struct MNinfo tempMNInfo);// 随机选择一个直连node来通知某个间接连接且需要重连的destIdent

private:
  // 预期拓扑
  int m_Pod;//是第几个Pod
  int m_Links;//Link总数
  int m_nPods;//Pod的数量
  int m_nMaster;//master数量
  int m_ToRNodes;//第一层每个Pod结点的数量
  int m_LeafNodes;//第二层每个Pod结点的数量
  int m_SpineNodes;//第三层结点的数量
  int m_defaultLinkTimer;//链路定时器，避免震荡
  bool m_IsCenterRouting=false;
  bool m_randomEcmpRouting=false; 
  ident myIdent;

  bool chiefMaster;
  int inDirNodeNum=0;// 非直连的node个数
  int m_defaultKeepaliveTimer=3;
  bool chiefMasterStatusChange=false;// 标志位，chief master判断出自己不是最优后，会等待3s再询问其他master，在这期间如果chief master的状态发生变化，则还要等待3s
  bool isStartMasterElection=false;
  vector<struct NICinfo> NICInfo;// 状态为up的网卡信息存储数组
  vector<int> MasterSock;// 存储所有和Master连接的套接字
  TCPRoute *m_tcpRoute;
  UDPClient m_udpClient;// udp to broadcast (only node not master)
  UDPServer m_udpServer;// udp to deal broadcast (only node not master)
  // master和转发nodo用来存储Node和套接字之间的映射关系
  vector<struct nodemaptosock> nodeMapToSock;
  // 在sonic上虚拟出多个交换机时使用，
  vector<string> listenNICName;//只能监听物理机上的部分网卡
  vector<struct mastermaptosock> masterMapToSock;//node记录自己和master连接的套接字
  vector<struct pathtableentry *> nodeInDirPathTable;//node自己选的和master间接连接的路径
  vector<struct clustermasterinfo> clusterMasterInfo;// master内部记录的连接信息
  vector<struct indirpathtableentry> masterInDirPathTable;// master会记录所有间接连接node所用的路径
  vector<string> masterAddress;
  
  struct mappingtableentry *headMappingTableEntry=(struct mappingtableentry *)malloc(sizeof(struct mappingtableentry));// 映射表的头节点
  // 映射表数量很多，所以还得给映射表加个索引表
  vector<struct mappingtableentry *> mappingTableEntryIndex;

  struct linktableentry *headLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));//master链路表的头节点
  struct pathtableentry *headPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));//头节点中的counter用作链表长度计数器
  
  pthread_t masterLink_thread;
  pthread_t keepalive_thread;
  pthread_t oldChiefFindNewOne_thread;// 判断是否为最优的master或者重新选举master
  pthread_t listenNIC_thread;
  pthread_t listenKeepAlive_thread;
};