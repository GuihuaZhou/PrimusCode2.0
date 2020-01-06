#include <string>
#include <vector>
#define NL_PKT_BUF_SIZE 8192
#define NL_DEFAULT_ROUTE_METRIC 20

#define MAX_PATH_LEN 5// 路径最大长度
#define MAX_ADDR_NUM 48// 一条路径能达到的服务器最大数
#define MAX_INDIR_NUM 2// Master间接连接node的最大数量，管理员设定阈值，超过即询问其他master是否状态更好

#define MN_PORT 6666
#define ND_PORT 2225
#define PT_PORT 8848//传递路径信息

#define NIC_CHECK_INTERVAL 10000// us
#define SEND_PATH_INTERVAL 100000// us

#define TCP_BUF_SIZE 2048// KB
#define MNINFO_BUF_SIZE sizeof(struct MNinfo)
#define ND_BUF_SIZE sizeof(struct NDinfo)
#define PT_BUF_SIZE sizeof(struct pathinfo)

#define MGMT_INTERFACE "eth0"

using namespace std;

struct ident
{
	int level;
	int position;
};

struct NICinfo
{
  string NICName;// 网口名称  
  // 该网口连接的邻居的角色
  bool isMaster;
  bool isServer;
  bool isSwitch;
  bool isNeedJudge;// 判断是否需要检测该网口的状态，直连链路故障后，邻居信息不会删除，但是以后的故障检测就可以跳过该条目
  bool flag;// 网卡状态
  ident neighborIdent;// 邻居的ident
  struct sockaddr_in localAddr;// 网口的IP地址
  struct sockaddr_in localMask;// 网口的掩码
  struct sockaddr_in neighborAddr;// 邻居的IP地址
};

struct MNinfo// Master-Node通信的信息格式
{
	struct sockaddr_in addr;// 
	ident destIdent;// 接收该Message的ident
	ident forwardIdent;
	ident srcIdent;// 发送该Message的ident
	// 如果是链路信息，则pathNodeIdent[0]和pathNodeIdent[1]表示一条链路
	ident pathNodeIdent[MAX_PATH_LEN];
	bool clusterMaster;// master内部信息格式
	bool chiefMaster;// 
	bool reachable;// 若转发node发现目的地址不可达，则会原路返回该message，并将此标志位标为false
	bool keepAlive;// 表示是keep alive
	bool linkFlag;// 表示是链路变化信息
	bool hello;// 建立Tcp连接后向Master发送的hello信息
	bool ACK;// 确认目的结点已经收到本地发出的信息
	bool bye;
};

// 发送hello时，pathNodeIdentA和pathNodeIdentB都填上发送节点的ident;master回复ack时，pathNodeIdentA和pathNodeIdentB都填上master的ident;
// 表示某个node能和master直连时，pathNodeIdentA填master的ident，pathNodeIdentB填node的ident

struct addrset
{
	struct sockaddr_in addr;// 16B
	unsigned int prefixLen;
};

struct NDinfo
{
	ident myIdent;
	struct sockaddr_in localAddr;// 将自己的地址告诉给邻居
};

struct pathinfo// 节点间传递路径信息格式
{
	ident pathNodeIdent[MAX_PATH_LEN];
	struct addrset addrSet[MAX_ADDR_NUM];
	struct addrset nodeAddr;
	struct addrset nextHopAddr;
	int nodeCounter;
	bool dirConFlag;
};

struct pathaddrset
{
	struct addrset addrSet[MAX_ADDR_NUM];
};

struct pathtableentry// Node路径表条目，存储用
{
	ident pathNodeIdent[MAX_PATH_LEN];
	struct pathaddrset *pathAddrSet;
	struct addrset nodeAddr;
	struct addrset nextHopAddr;
	bool dirConFlag;
	int linkCounter;
	int nodeCounter;
	struct pathtableentry *next;
};

struct accumulationMNinfo// 间接连接的node可能会因为拓扑变化使得Master下发的信息无法到达，因此这些信息就累积下来了
{
  struct MNinfo *tempMNInfo;
  struct accumulationMNinfo *next;
};

struct indirpathtableentry
{
	ident pathNodeIdent[MAX_PATH_LEN];
	int nodeSock;
	struct accumulationMNinfo *headAccumulationMNInfo;
};

struct nexthopandweight
{
	string NICName;
	struct sockaddr_in srcAddr;
	struct sockaddr_in gateAddr;
	int weight;
};

struct masterinfo// master用来存储其他master信息
{
	bool chiefMaster;// 是否是管事的master
	int masterSock;
	ident masterIdent;
	struct sockaddr_in masterAddr;
};

struct nodemaptosock// master和node通用，node也可能作转发，用来存储与其建立了tcp连接的Node的信息
{
  ident nodeIdent;// Node的ident
  int nodeSock;// 该连接的套接字
  bool direct;// 是否为直连
  int keepAliveFaildNum;// 未接收到的keep alive数量
  bool recvKeepAlive;// 标志位，表示收到了Master的keep alive
};

struct mastermaptosock// node用，用来存储已经建立了tcp连接的master的信息
{
	bool chiefMaster;// 此连接是否为chiefMaster
	string masterAddr;// Master的地址
  ident masterIdent;// 
  int masterSock;// 和Master建立的连接的套接字
  string NICName;// node通过该网口与master连接
  bool direct;// 是否为直接连接
  string middleAddr;// 中间节点的地址，作转发
  int keepAliveFaildNum;// 未接收到的keep alive数量
  bool recvKeepAlive;// 标志位，表示收到了Master的keep alive
  bool isStartKeepAlive;
  struct pathtableentry *inDirPath;// 如果是间接连接，则保存其选择的间接路径
};

struct clustermasterinfo// chiefmaster和common master用，用来存储master内部的连接关系
{
	bool chiefMaster;// 此连接是否为chiefMaster
	struct sockaddr_in masterAddr;// Master的地址
  ident masterIdent;// 
  int inDirNodeNum;// 此master的间接连接数量，如果是chiefmaster宕机，common master就会通过自己存储的这些信息来选举chief，其他情况更换chief必须优chief主导
};