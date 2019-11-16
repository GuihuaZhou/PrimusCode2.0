#include <string>
#include <vector>
#define NL_PKT_BUF_SIZE 8192
#define NL_DEFAULT_ROUTE_METRIC 20
#define MAX_PATH_LEN 5
#define MAX_ADDR_NUM 24
#define MN_PORT 6666
#define TCP_BUF_SIZE 2048//KB

using namespace std;

struct ident
{
	int level;
	int position;
};

struct NICinfo
{
  string NICName;//网卡名称  
  bool isMaster;
  bool isServer;//邻居角色
  bool isSwitch;
  bool flag;
  ident neighborIdent;
  struct sockaddr_in localAddr;
  struct sockaddr_in localMask;
  struct sockaddr_in neighborAddr;
};

struct MNinfo//Master-Node通信的信息格式
{
	ident destIdent;// 目的ident
	ident srcIdent;// 源ident
	ident pathNodeIdentA;
	ident pathNodeIdentB;
	bool keepAlive;
	bool linkFlag;
	bool hello;
	bool ACK;
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
	struct sockaddr_in localAddr;// 将自己的地址和掩码告诉给邻居
  bool ACK;// 收到后，邻居会回复
};

struct pathinfo//节点间传递路径信息格式
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

struct pathtableentry//Node路径表条目，存储用
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

struct nexthopandweight
{
	string NICName;
	struct sockaddr_in srcAddr;
	struct sockaddr_in gateAddr;
	int weight;
};

struct masterinfo//master用来存储其他master信息
{
	bool chiefMaster;// 是否是管事的master
	int masterSock;
	ident masterIdent;
	struct sockaddr_in masterAddr;
};

struct nodemaptosock//master和node通用，node也可能作转发
{
  ident nodeIdent;
  int nodeSock;
  bool direct;
  int keepAliveFaildNum;//未接收到的keep alive数量
  bool keepAliveFlag;
};

struct mastermaptosock//node用
{
	string masterAddr;
  ident masterIdent;
  int masterSock;
  string NICName;// node通过该网口与master连接
  bool direct;// 
  string middleAddr;//中间节点的地址，作转发
  int keepAliveFaildNum;//未接收到的keep alive数量
  bool keepAliveFlag;
};