#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/rtnetlink.h>
// #include "raft.h"

#define MAX_OUTBOUNDING_EVENTS 1000
#define MAX_PATH_LEN 5
#define MAX_ADDR_NUM 48
#define PRIMUS_LISTEN_PORT 8848
#define ND_LISTEN_PORT 8849
#define CONNECT_INTERVAL 1000000
#define TCP_BUF_SIZE (2048*1024)
#define MAX_TCP_PENDING_ACCEPT_CONNS 1024
#define MESSAGE_BUF_SIZE sizeof(message)
#define ND_BUF_SIZE sizeof(NDmessage)
#define MAX_SOCKNUM_PER_THREAD 1000
#define MAX_NODESUN_PER_SENDTHREAD 1000
#define MAX_EPOLLFD_NUM 100
#define MAX_CTRL_NUM 100
#define MAX_NEXTHOP_NUM 100
#define MAX_EVENT_ID 1000
#define NL_PKT_BUF_SIZE 512
#define NL_DEFAULT_ROUTE_METRIC 20
#define WAIT_INTERVAL 30000
#define MAX_FOWNODE_NUM 3
#define NIC_CHECK_INTERVAL 6000
#define KEEPALIVE_INTERVAL 3//s
#define PRINT_MASTER_RECV_ALLRS_TIME false
#define PRINT_NODE_MODIFY_TIME false 
#define PRINT_NODE_RECV_RS_TIME false
#define MASTER_TEST false
#define NODE_TEST false
#define COMMON_PATH "/home/guolab/"

// // vm testbed
#define MGMT_INTERFACE "eth0"
#define MASTER_ADDRESS "172.16.80.1"
// // tlinux
// #define MGMT_INTERFACE "eth0"
// #define MASTER_ADDRESS "9.134.73.204"
// // 9.1-10.1
// #define MGMT_INTERFACE "eno1"
// #define MASTER_ADDRESS "10.0.9.1"

// 函数名：所有首字母大写
// 变量名：第一个首字母小写，其他大写（驼峰命名）
// 结构体名：全小写

using namespace std;

class Primus
{
public:
    Primus(
        int level,
        int position,
        int toRNodes,
        int leafNodes,
        int spineNodes,
        int nPods,
        int defaultLinkTimer,
        int defaultKeepaliveTimer);
    ~Primus();

    struct threadparama
    {
        Primus *tempPrimus;
        int tempEpollFd;
    };

    struct threadparamb
    {
        Primus *tempPrimus;
        int startIndex;// socktable的开始位置
    };

    struct threadparamc
    {
        Primus *tempPrimus;
        int linkIndex;// 链路在链路表中的位置
    };

    struct threadparame
    {
        Primus *tempPrimus;
        struct sockaddr_in tempLocalAddr;
        struct sockaddr_in tempNeighborAddr;
        string tempLocalNICName;
    };

    struct nexthopandweight
    {
        string NICName;
        struct sockaddr_in gateAddr;
        struct sockaddr_in srcAddr;
      int weight;
    };
    
    struct ident
    {
        int level;
        int position;
    };

    struct neighborinfo
    {
        ident neighborIdent;
        string localNICName;
        struct sockaddr_in neighborAddr;
        struct sockaddr_in srcAddr;
        int NDType;//0,没有邻居发现;1,正在进行;3,已经完成
        int NICFlag;// 0,1正常，-1故障
    };

    struct threadparamd
    {
        Primus *tempPrimus;
        struct neighborinfo tempNeighborInfo;
    };

    struct NDmessage
    {
        int messageType;// 1,hello;2,ack
        ident myIdent;
        ident neighborIdent;
        struct sockaddr_in srcAddr;
    };

    struct link
    {
        ident identA;
        ident identB;
        sockaddr_in addrA;
        sockaddr_in addrB;
        int eventId;
        bool linkStatus;
    };

    struct message
    {
        link linkInfo;
        ident srcIdent;
        ident fowIdent;// 中间转发节点
        ident dstIdent;
        bool ack;
        // message type
        int messageType;// 1:hello;2:linkStatus;3:keepalive;4,reelect
        // srcIdent type
        int srcIdentRole;// 1:node;2:master;3:slave
        int messageEventQueueIndex;
        int transportType;// 1:tcp;2:udp
    };

    struct threadparamf
    {
        Primus *tempPrimus;
        int linkIndex;// 链路在链路表中的位置
        struct message tempMessage;
    };

    struct messageevent
    {
        struct message messageInfo;
        unsigned int numOfSwitchesShouldNotify;
        unsigned int numOfSwitchesSent;//Only looked and modified by send thread
        unsigned int numOfSwitchesResponsed;//Only looked and modified by recv thread
        //We only record the count instead of which switch, since we always notify all ToR switches
        struct timeval startStamp;//Time when the message is generated (or received from reporting switch) 
        // struct timeval endStamp;//Time when all send threads send out this event
        // struct timeval tvFirstSent;//Time when the first send thread sends out this event
    };

    struct messageeventqueue
    {
        struct messageevent eventQueue[MAX_OUTBOUNDING_EVENTS]; //Message pending to process
        unsigned int head;//First element is head
        unsigned int tail;//Last element is tail-1
        unsigned int len;
        unsigned int counterIn;//How many events have ever been pushed in
        unsigned int counterOut;//How many events have ever been popped out
        //messageID start from 1
    };

    struct nexthopandvaildpathnum
    {
        ident nextHopIdent;
        int vaildPathNum;
    };

    struct pathtableentry 
    {
        ident pathNodeIdent[MAX_PATH_LEN];
        sockaddr_in addrSet[MAX_ADDR_NUM];
        int faultLinkCounter;
        bool dirConMasterFlag;
        nexthopandvaildpathnum *numOfVaildPathPerNextHop;
    };

    struct linktableentry
    {
        link linkInfo;
        bool isStartTimer;// 是否已经开启定时器
        int linkSleepTimes;// 定时器，即抑制时间，在这个时间内，同一条链路的其他更新信息不会被Master处理，每次休眠m_DefaultLinkTimer us
        struct message lastMessage;
        struct timeval startStamp;// node
    };

    struct nodesocktableentry
    {
        ident nodeIdent;
        int nodeSock;//
        int unRecvNum;//未收到的keep alive次数，超过3次即认为失联
    };

    struct controllersocktableentry
    {
        ident controllerIdent;
        int controllerSock;
        int controllerRole;//
        int unRecvNum;//未收到的keep alive次数，超过3次即认为失联
    };

    // 修改路由
    int rta_addattr_l(struct rtattr *rta,size_t maxlen,int type,void *data,size_t alen);
    int addattr_l(struct nlmsghdr *n,size_t maxlen,int type,void *data,size_t alen);
    int addattr32 (struct nlmsghdr *n,size_t maxlen,int type,int data);
    int OpenNetlink();
    void AddMultiRoute(struct sockaddr_in dstAddr,unsigned int prefixLen,vector<struct nexthopandweight> nextHopAndWeight);//添加多路径路由
    void AddSingleRoute(struct sockaddr_in dstAddr,unsigned int prefixLen,struct nexthopandweight tempNextHopAndWeight);
    void DelRoute(struct sockaddr_in dstAddr,unsigned int prefixLen);
    unsigned long Convert(struct sockaddr_in dstAddr,unsigned int prefixLen);//获得网络号
    static string GetNow();//获取当前时间，精确到秒

    bool SameNode(ident nodeIdentA,ident nodeIdentB);
    void PrintMessage(struct message tempMessage);
    void PrintNodeSockTable();
    void PrintControllerSockTable();
    void PrintLinkTable();
    void PrintNeighborTable();
    void InitiateLinkTable();
    void InitiateControllerTable();
    bool AddSocketToEpoll(int sock);
    bool DelSocket(int sock);
    void InitiateUDPServer();
    void InitiateNDServer();
    void ListenTCP();
    void Start();
    bool AddNeighborEntry(string localNICName,struct sockaddr_in localAddr,struct sockaddr_in neighborAddr);
    bool UpdateNeighbor(ident neighborIdent,struct sockaddr_in neighborAddr);
    bool AddNodeSock(ident nodeIdent,int nodeSock);
    bool UpdateNodeKeepAlive(ident nodeIdent);
    static void* RecvMessageThread(void* tempThreadParam);
    static void* SendMessageThread(void* tempThreadParam);
    static void* LinkTimerThread(void* tempThreadParam);
    static void* ListenNICThread(void* tempThreadParam);
    static void* RecvNDMessageThread(void* tempThreadParam);
    static void* SendNDHelloThread(void* tempThreadParam);
    static void* SrcWaitRSThread(void* tempThreadParam);
    static void* MasterGenerateLinkStatusChange(void* tempThreadParam);
    static void* KeepaliveThread(void* tempThreadParam);
    static void* CheckKeepaliveThread(void* tempThreadParam);
    static void* SlaveWaitRSThread(void* tempThreadParam);
    void CreateKeepAliveThread();
    void PrintPathTable();
    void InitiatePathTable();
    int SendMessageByTCP(int sock,struct message tempMessage);
    int SendMessageByUDP(struct sockaddr_in localAddr,struct sockaddr_in remoteAddr,struct message tempMessage);
    void InitiateMessageEventQueues();
    int EnqueueMessageIntoEventQueue(struct message tempMessage);
    bool DequeueMessageFromEventQueue();
    void InitiateNodeSockTable();
    void UpdateControllerSockTable(int controllerSock,ident controllerIdent,int controllerRole);
    void CreateEpollFdAndRecvMessageThread();
    int SendToAllAffectedNodes(struct message tempMessage,int tempStartIndex,int tempEndIndex);// 在此处确定下发规则
    bool ConnectWithMaster(string masterIP,string NICName);
    bool UpdateLinkTable(struct message tempMessage);
    bool UpdateMultiPathBlock(int startIndex,int affectedNextHopIndex,int numOfModifyPathEntryPerDstNode,int numOfDstNode,int numOfJumpPath,bool linkStatus);
    bool UpdateSinglePathBlock(int startIndex,int numOfModifyPathEntryPerDstNode,int numOfDstNode,int numOfJumpPath,bool linkStatus);
    bool UpdatePathTable(struct link tempLink);
    void GenerateLinkStatusChange();
    int GetNodeSock(ident nodeIdent);
    bool RecvNDReply(struct NDmessage tempNDMessage,struct sockaddr_in neighborAddr);
    bool RecvNDHello(ident neighborIdent,struct sockaddr_in neighborAddr);
    void SendLSToController(struct link tempLink,int linkIndex,ident faultNextHop);
    int GetLinkIndex(ident identA,ident identB);
    bool RecvRS(struct link tempLink);
    sockaddr_in GetLocalAddrByNeighborIdent(ident neighborIdent);
    sockaddr_in GetGateAddrByNeighborIdent(ident neighborIdent);
    string GetLocalNICNameByNeighborIdent(ident neighborIdent);

private:

    //Global 
    struct messageeventqueue messageEventQueue;
    //Message pending to process (link-states to deliver and response)
    //Main (or recv) thread produce messages, recv thread consume (when the link-states have been responded by all switches)
    pthread_mutex_t MsgQueueMutex;//=PTHREAD_MUTEX_INITIALIZER;//mutex for this queue
    pthread_mutex_t LinkTableMutex;
    pthread_mutex_t MsgQueueEventMutex[MAX_OUTBOUNDING_EVENTS];
    pthread_mutex_t UdpMutex;
    pthread_mutex_t PrintMessageMutex;
    //mutex for each event position in this queue
    //Note mutex have to be global variables, so we don't attach it to each event struct.

    linktableentry *linkTable;
    pathtableentry *pathTable;
    nodesocktableentry *nodeSockTable;
    controllersocktableentry controllerSockTable[MAX_CTRL_NUM];
    vector<struct neighborinfo> neighborTable;

    int linkNum=0;
    int pathNum=0;
    int nodeSockNum=0;
    int udpSock=-1;
    int affectedNodeNumA=0;
    int affectedNodeNumB=0;

    ident tempIdent;
    sockaddr_in tempAddr;

    ident m_Ident;
    int m_Role;// 1:node;2:master;3:slave
    int m_Pod;
    int m_ToRNodes;
    int m_LeafNodes;
    int m_SpineNodes;
    int m_nPods;
    int m_DefaultLinkTimer;
    int m_DefaultKeepaliveTimer;

    // int epollFdTable[MAX_EPOLLFD_NUM];// 主要是控制器使用epollFdTable，node为其他node转发时也会用到 
    int m_EpollFd;//该fd专门用于node处理

    bool m_RecvReElectFromNode=false;
    bool m_RecvReElectReplyFromMaster=false;

    int recvTcpNum=0;
    int recvUdpNum=0;

    ofstream m_MessageLogFout;
    ofstream m_PathLogFout;
};
