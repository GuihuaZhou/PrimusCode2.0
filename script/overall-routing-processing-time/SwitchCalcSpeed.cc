#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <asm/types.h>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

#define PRINT_PGOGRESS_ROUND_INTERVAL 100

#define IF_PRINT_DEBUG_INFO false

#define TOR_PER_POD 100
#define AGG_PER_POD 4
#define CORE_PER_AGG 4
#define POD_NUM 100

#define MAX_PATH_LENGTH 4

#define TOTAL_LINK_NUM (TOR_PER_POD*AGG_PER_POD*POD_NUM+AGG_PER_POD*CORE_PER_AGG*POD_NUM)
#define TOTAL_PATH_NUM (AGG_PER_POD*CORE_PER_AGG*TOR_PER_POD*POD_NUM)
#define TOTAL_ROUTE_NUM (TOR_PER_POD*POD_NUM)

#define MAX_EVENT_ID 1000000

#define MASTER_LISTEN_PORT 8801
// #define MASTER_IP "127.0.0.1"

#define TCP_BUF_SIZE (2048*1024)
//B

#define CONNECT_INTERVAL 5000000
//us

static unsigned int kernelRouteChangeTimes=0;

static double totalTimeForRouteUpdateElapsed=0;
static double linkTableTimeElapsed=0;
static double pathTableTimeElapsed=0;
static double kernelRouteChangeTimeElapsed=0;

static double totalTimeElapsed=0;
static double timeElapsedTillLSReportSentOut=0;
static double timeElapsedTillLSEpollNotified=0;
static double timeElapsedTillLSUpdateReceived=0;
static double timeElapsedTillLSResponseSentOut=0;
static double timeElapsedTillLSACKReceived=0;

static double lastSwitchTimeFromEpollNotifyToTCPRecvDone=0;
static double lastSwitchTimeForRouteUpdateElapsed=0;
static double lastSwitchTimeFromRouteUpdateDoneToTCPSendDone=0;
static double totalSwitchTimeFromRouteUpdateDoneToTCPSendDone=0;

static struct timeval tvAllStart;

static unsigned int totalLSUpdateReceived=0;
static unsigned int totalLSUpdateReported=0;
static unsigned int totalLSUpdateReportACKed=0;

static struct timeval lastPrintGlobalTime={0,0};

static int MainChannelSock=0;
//TCP sock to communicate with master 
//Normally, its the TCP socket connected to master
//In case of failure, may be the indirect TCP socket through other switches

static int RTNetlinkSockForNICMonitor=0;
//Socket for fetch interface states through rt_netlink

static int RTNetlinkSockForModifyRoute=0;
//Socket for modify route through rt_netlink

#define MAX_INDIRECT_CHANNEL_NUM TOR_PER_POD
static int OthersIndirectChannelSockTable[MAX_INDIRECT_CHANNEL_NUM];

#define MAX_UDP_CHANNEL_NUM 3
static int UDPSockTable[MAX_UDP_CHANNEL_NUM];

#define MAX_EPOLL_EVENTS_PER_RECV 10

struct SwitchNode
{
	int level;
	int ID;
};

struct Link 
{
	SwitchNode srcNode;
	SwitchNode dstNode;
};

struct LinkAffectAreaType
{
	char type;//1: ToR(other)<->Agg;  
			//2: Agg(myself pod)<->Core;
			//3: ToR(myself)<->Agg;
			//4: Agg(other pod)<->Core; 
	int firstEntry;
};

struct LinkTableEntry
{
	Link link;
	bool state;
	unsigned int eventID;
	LinkAffectAreaType affectArea;
};

double time_diff(struct timeval x , struct timeval y)
{
	double x_ms , y_ms , diff;
	
	x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
	y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;
	
	diff = (double)y_ms - (double)x_ms;

	if(diff<0)
	{
		fprintf(stderr, "ERROR! time_diff<0\n");
		printf("ERROR! time_diff<0\n");
		fflush(stdout);
		exit(1);
	}

	// printf("time_diff: %f\n",diff);
	
	return diff;
}

bool sameNode(SwitchNode A, SwitchNode B)
{
	if(A.level==B.level && A.ID==B.ID)
		return true;
	else
		return false;
}

bool sameLink(Link A, Link B)
{
	if((sameNode(A.srcNode,B.srcNode) && sameNode(A.dstNode,B.dstNode))
		|| (sameNode(A.dstNode,B.srcNode) && sameNode(A.srcNode,B.dstNode)))
		return true;
	else
		return false;
}

bool sameLink(Link A, SwitchNode Bsrc, SwitchNode Bdst)
{
	if((sameNode(A.srcNode,Bsrc) && sameNode(A.dstNode,Bdst))
		|| (sameNode(A.dstNode,Bsrc) && sameNode(A.srcNode,Bdst)))
		return true;
	else
		return false;
}

void generateRandomLink(Link *testLink)
{
	testLink->srcNode.level=rand()%2+1;
	testLink->dstNode.level=testLink->srcNode.level+1;
	switch (testLink->srcNode.level)
	{
		case 1:
			testLink->srcNode.ID=rand()%(TOR_PER_POD*POD_NUM);
			testLink->dstNode.ID=rand()%AGG_PER_POD+int(testLink->srcNode.ID/TOR_PER_POD)*AGG_PER_POD;
			break;
		case 2:
			testLink->srcNode.ID=rand()%(AGG_PER_POD*POD_NUM);
			testLink->dstNode.ID=rand()%CORE_PER_AGG+(testLink->srcNode.ID%AGG_PER_POD)*CORE_PER_AGG;
			break;
		default:
			fprintf(stderr, "generateRandomLink testLink->srcNode.level=%d, ERROR!\n", testLink->srcNode.level);
			exit(1);
	}
}

void printLink(Link link)
{
	printf("link(%d.%d<-->%d.%d)\t"
			,link.srcNode.level,link.srcNode.ID
			,link.dstNode.level,link.dstNode.ID
			);
}

void printLinkTableEntry(LinkTableEntry linkTableEntry)
{
	printLink(linkTableEntry.link);
	printf("state:%d\teventID:%d\taffectArea(%d:%d)\t"
			,linkTableEntry.state
			,linkTableEntry.eventID
			,linkTableEntry.affectArea.type
			,linkTableEntry.affectArea.firstEntry
			);
}

LinkTableEntry *linkTable;

void generateLinkTable(SwitchNode mySwitchNode)
{
	printf("------- generateLinkTable -------\n");
	printf("total # of links:%d, total memory consumption of link table:%.2f MB\n",TOTAL_LINK_NUM, sizeof(LinkTableEntry)*TOTAL_LINK_NUM/1E6);
	
	linkTable=(LinkTableEntry *)malloc(sizeof(LinkTableEntry)*TOTAL_LINK_NUM);

	//Generate links between ToR and Agg
	int totalLinkNumBetweenToRandAgg=TOR_PER_POD*AGG_PER_POD*POD_NUM;
	SwitchNode srcToR;
	SwitchNode dstAgg;
	srcToR.level=1;
	dstAgg.level=2;
	for(int i=0;i<totalLinkNumBetweenToRandAgg;i++)
	{
		srcToR.ID=int(i/AGG_PER_POD);
		dstAgg.ID=i%AGG_PER_POD+int(i/(TOR_PER_POD*AGG_PER_POD))*AGG_PER_POD;
		linkTable[i].link.srcNode=srcToR;
		linkTable[i].link.dstNode=dstAgg;
		linkTable[i].state=true;
		linkTable[i].eventID=0;
		if(!sameNode(srcToR,mySwitchNode))//To other ToR
		{
			linkTable[i].affectArea.type=1;
			linkTable[i].affectArea.firstEntry=srcToR.ID*AGG_PER_POD*CORE_PER_AGG+(dstAgg.ID%AGG_PER_POD)*CORE_PER_AGG;
		}
		else
		{
			linkTable[i].affectArea.type=3;//To this ToR
			linkTable[i].affectArea.firstEntry=(dstAgg.ID%AGG_PER_POD)*CORE_PER_AGG;
		}
	}

	//Generate links between Agg and Core
	int totalLinkNumBetweenAggandCore=AGG_PER_POD*CORE_PER_AGG*POD_NUM;
	SwitchNode srcAgg;
	SwitchNode dstCore;
	srcAgg.level=2;
	dstCore.level=3;
	for(int i=0;i<totalLinkNumBetweenAggandCore;i++)
	{
		srcAgg.ID=int(i/CORE_PER_AGG);
		dstCore.ID=i%(AGG_PER_POD*CORE_PER_AGG);
		int j=i+totalLinkNumBetweenToRandAgg;
		linkTable[j].link.srcNode=srcAgg;
		linkTable[j].link.dstNode=dstCore;
		linkTable[j].state=true;
		linkTable[j].eventID=0;
		if(int(srcAgg.ID/AGG_PER_POD)==int(mySwitchNode.ID/TOR_PER_POD))
		//From this pod
		{
			linkTable[j].affectArea.type=2;
			linkTable[j].affectArea.firstEntry=dstCore.ID;
		}
		else// To other pod
		{
			linkTable[j].affectArea.type=4;
			linkTable[j].affectArea.firstEntry=dstCore.ID+int(srcAgg.ID/AGG_PER_POD)*TOR_PER_POD*AGG_PER_POD*CORE_PER_AGG;
		}
	}
}

void printLinkTable( )
{
	for(int i=0;i<TOTAL_LINK_NUM;i++)
	{
		printf("LinkTableEntry[%5d]:\t",i);
		printLinkTableEntry(linkTable[i]);
		printf("\n");
	}
}

int getLinkTableEntryIDBruteForce(Link testLink)
{
	for(int j=0;j<TOTAL_LINK_NUM;j++)
	{
		if(sameLink(testLink,linkTable[j].link))
		{
			return j;
		}
	}
	return -1;
}

int getLinkTableEntryID(Link testLink)
{
	switch (testLink.srcNode.level)
	{
		case 1:
			return testLink.srcNode.ID*AGG_PER_POD+testLink.dstNode.ID%AGG_PER_POD;
			break;
		case 2:
			return TOR_PER_POD*AGG_PER_POD*POD_NUM+testLink.srcNode.ID*CORE_PER_AGG+testLink.dstNode.ID%CORE_PER_AGG;
			break;
		default:
			fprintf(stderr, "getLinkEntryID testLink.srcNode.level=%d, ERROR!\n", testLink.srcNode.level);
			return -1;
	} 
}

struct Path 
{
	SwitchNode srcNode;
	SwitchNode nextHop[MAX_PATH_LENGTH];
	int dstHop;	//which next hop is the destination hop; 
				//-1 means this path is useless, holding place for dest to myself
};

struct PathTableEntry
{
	Path path;
	int FL;
};

int GetDstNodeIDFromPathTableEntry(PathTableEntry pathTableEntry)
{
	return pathTableEntry.path.nextHop[pathTableEntry.path.dstHop].ID;
}

void printPath(Path path)
{
	printf("path(%d.%d",path.srcNode.level,path.srcNode.ID);
	for(int i=0;i<MAX_PATH_LENGTH;i++)
		printf("-->%d.%d",path.nextHop[i].level,path.nextHop[i].ID);
	printf(")\tdstHop:%d\t",path.dstHop);
}

void printPathTableEntry(PathTableEntry pathTableEntry)
{
	printPath(pathTableEntry.path);
	printf("FL:%d\t"
			,pathTableEntry.FL
			);
}

PathTableEntry *pathTable;

struct RouteNextHop
{
	SwitchNode switchNode;
	int weight;//0 means this nexthop is not working
};

struct RouteTableEntry
{
	SwitchNode dstNode;//Destination ToR
	RouteNextHop nextHop[AGG_PER_POD];
	int numOfWorkingNextHops;//0 means the route to this destination is not working
};

RouteTableEntry *routeTable;
//This is the internal data structure to facilitate kernel route change

RouteTableEntry defaultRoute;
//The merged default route to all destinations, expressed in one kernel route
//The dstNode is -1.-1
RouteTableEntry *defaultRouteToPod;
//The merged default route to each pod, so each pod can be expressed in one kernel route
//The dstNode in each entry is -1.ID, where ID is the first ToR's ID in this pod

void printRouteTableEntry(RouteTableEntry routeTableEntry)
{
	printf("dst(%d.%d)\t",routeTableEntry.dstNode.level,routeTableEntry.dstNode.ID);
	printf("nexthop(");
	for(int i=0;i<AGG_PER_POD;i++)
	{
		printf(" %d.%d:%d"
			,routeTableEntry.nextHop[i].switchNode.level
			,routeTableEntry.nextHop[i].switchNode.ID
			,routeTableEntry.nextHop[i].weight);
	}
	printf(")\t");
	printf("numOfWorkingNextHops(%d)\t",routeTableEntry.numOfWorkingNextHops);
}

void printRouteTable()
{
	for(int i=0;i<TOTAL_ROUTE_NUM;i++)
	{
		printf("RouteTableEntry[%5d]:\t",i);
		printRouteTableEntry(routeTable[i]);
		printf("\n");
	}
}

struct NexthopAndWeight    //多路径路由使用
{
	string NICName;        //本地接口,如"eth0"
	int weight;            //路由权重
};

int
rta_addattr_l (struct rtattr *rta, size_t maxlen, int type, void *data, 
               size_t alen)
{
	size_t len;
	struct rtattr *subrta;

	len = RTA_LENGTH (alen);

	if (RTA_ALIGN (rta->rta_len) + len > maxlen)
	return -1;

	subrta = (struct rtattr *) (((char *) rta) + RTA_ALIGN (rta->rta_len));
	subrta->rta_type = type;
	subrta->rta_len = len;
	memcpy (RTA_DATA (subrta), data, alen);
	rta->rta_len = NLMSG_ALIGN (rta->rta_len) + len;

	return 0;
}

int
addattr_l (struct nlmsghdr *n, size_t maxlen, int type, void *data, size_t alen)
{
	size_t len;
	struct rtattr *rta;

	len = RTA_LENGTH (alen);

	if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
	return -1;

	rta = (struct rtattr *) (((char *) n) + NLMSG_ALIGN (n->nlmsg_len));
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy (RTA_DATA (rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

	return 0;
}

int waitForRouteChangeNetLinkACK ( )
{
  int status;
  int ret = -1;
  char buf[4096];
  struct iovec iov = { buf, sizeof buf };
  struct sockaddr_nl snl;
  struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
  struct nlmsghdr *h;
  struct nlmsgerr *errpayload;

  status = recvmsg (RTNetlinkSockForModifyRoute, &msg, 0);

  if (status <= 0)
  {
      perror ("read_netlink: Error: ");
      exit(1);
  }

  // We need to handle more than one message per 'recvmsg'
  for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status);
       h = NLMSG_NEXT (h, status))
  {
    switch(h->nlmsg_type)
    {
      case NLMSG_ERROR:
      	errpayload =  (struct nlmsgerr *)NLMSG_DATA (h);
      	if(errpayload->error!=0)
      	{
	        perror("waitForRouteChangeNetLinkACK: message error\n");
	        exit(1);
	    }
	    ret=0;
      break;
    }
  }

  return ret;
}

void
route_add_single_path(struct sockaddr_in sock_addr_in, string NICName , unsigned int netmask){    //单路径路由使用

	/***
	功能：向内核路由表添加单路径路由
	参数：
   sock_addr_in: 目的网络, struct sockaddr_in结构
	    NICName：接口名称, 如"eth0"
		netmask：掩码长度

	***/
	
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
        char nl_buf[4096];
    } nl_req;
	
	int if_index=if_nametoindex(NICName.c_str());    //接口名称转索引
	memset(&nl_req, 0, sizeof(nl_req));
    nl_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));    //NLMSG_LENGTH宏：返回nlmsghdr长度加上sizeof(struct rtmsg)长度
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE | NLM_F_ACK;           //没有则创建,有则替换,这相当于包含了修改功能
	nl_req.n.nlmsg_type=RTM_NEWROUTE;
	nl_req.r.rtm_family = AF_INET;
	nl_req.r.rtm_table = RT_TABLE_MAIN;
	nl_req.r.rtm_protocol = RTPROT_BOOT;
	nl_req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	nl_req.r.rtm_type = RTN_UNICAST;
	nl_req.r.rtm_dst_len=netmask;
	nl_req.r.rtm_src_len=0;
	nl_req.r.rtm_tos=0;
	//nl_req.r.rtm_flags=RT_TABLE_MAIN;

    addattr_l (&nl_req.n, sizeof(nl_req), RTA_DST, &(sock_addr_in.sin_addr.s_addr), 4);
	addattr_l(&nl_req.n, sizeof(nl_req), RTA_OIF, &if_index, 4);
    
    
    if(send(RTNetlinkSockForModifyRoute,&nl_req,nl_req.n.nlmsg_len,0)<=0){
    	perror("route_add_single_path error!");
    	exit(1);
    }
	waitForRouteChangeNetLinkACK();

	if(IF_PRINT_DEBUG_INFO)
		printf("******** route_add_single_path succeed! ********\n");
}


void
route_add_multi_path(struct sockaddr_in sock_addr_in, unsigned int netmask, vector<struct NexthopAndWeight> nexthopAndWeight_vector)
{    //多路径路由使用
	
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
        char nl_buf[2048];
    } nl_req;
	memset(&nl_req, 0, sizeof(nl_req));
	
	char rta_buf[2048];
	struct rtattr *rta =(struct rtattr *)((void *) rta_buf);
	struct rtnexthop *rtnh;
	
    nl_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE | NLM_F_ACK;
	nl_req.n.nlmsg_type=RTM_NEWROUTE;
	nl_req.r.rtm_family = AF_INET;
	nl_req.r.rtm_table = RT_TABLE_MAIN;
	nl_req.r.rtm_protocol = RTPROT_BOOT;
	nl_req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	nl_req.r.rtm_type = RTN_UNICAST;
	nl_req.r.rtm_dst_len=netmask;     //netmask位数
	nl_req.r.rtm_src_len=0;
	nl_req.r.rtm_tos=0;
	//nl_req.r.rtm_flags=RT_TABLE_MAIN;
	
	//内核通信数据包attribute构建
	addattr_l (&nl_req.n, 2048, RTA_DST, &(sock_addr_in.sin_addr.s_addr), 4);
	
	rta->rta_type = RTA_MULTIPATH;
    rta->rta_len = RTA_LENGTH (0);
	rtnh = (struct rtnexthop *)RTA_DATA (rta);
	
	for (int i = 0; i<nexthopAndWeight_vector.size(); i++){
		
		rtnh->rtnh_len=sizeof(struct rtnexthop);
	    rtnh->rtnh_flags=0;//RTNH_F_ONLINK;
		rtnh->rtnh_hops=nexthopAndWeight_vector[i].weight-1;
		rtnh->rtnh_ifindex=if_nametoindex((nexthopAndWeight_vector[i].NICName).c_str());
		rta->rta_len = rta->rta_len+RTNH_LENGTH(0);
		rtnh=RTNH_NEXT(rtnh);
	}
	
	addattr_l (&nl_req.n, 4096, RTA_MULTIPATH, RTA_DATA (rta), RTA_PAYLOAD (rta));

    if(send(RTNetlinkSockForModifyRoute,&nl_req,nl_req.n.nlmsg_len,0)<=0){
    	perror("route_add_multi_path error!");
    	exit(1);
    }
	waitForRouteChangeNetLinkACK();

	if(IF_PRINT_DEBUG_INFO)
		printf("******** route_add_multi_path succeed! ********\n");
}

void route_del(struct sockaddr_in sock_addr_in, unsigned int netmask){
	
	/***
	功能：向内核路由表删除路由(e.g. route_del(sock_addr_in, 24))
	参数：
	     des_ip: 目的网络,32位网络字节序地址
		netmask: 掩码长度

	***/
	
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
        char buf[4096];
    } nl_req;
	
	//__u32 des=des_ip;

	memset(&nl_req, 0, sizeof(nl_req));
    nl_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	nl_req.n.nlmsg_type=RTM_DELROUTE;
	nl_req.r.rtm_family = AF_INET;
	nl_req.r.rtm_table = RT_TABLE_MAIN;
	nl_req.r.rtm_protocol = RTPROT_BOOT;
	nl_req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	nl_req.r.rtm_type = RTN_UNICAST;
	nl_req.r.rtm_dst_len=netmask;
	nl_req.r.rtm_src_len=0;
	nl_req.r.rtm_tos=0;
	//nl_req.r.rtm_flags=RT_TABLE_MAIN;

	addattr_l(&nl_req.n, sizeof(nl_req), RTA_DST, &(sock_addr_in.sin_addr.s_addr), 4);

    if(send(RTNetlinkSockForModifyRoute,&nl_req,nl_req.n.nlmsg_len,0)<=0){
    	perror("route_del error!");
    	exit(1);
    }
	waitForRouteChangeNetLinkACK();

	if(IF_PRINT_DEBUG_INFO)
		printf("******** route_del succeed! ********\n");
}

void changeKernelRoute(RouteTableEntry route)
//Pretend to modify kernel routes
{
	struct timeval tvStart,tvEnd;
	gettimeofday(&tvStart,NULL);
	kernelRouteChangeTimes++;

	//Test code for modify kernel routes
	struct sockaddr_in tempAddr;
	tempAddr.sin_family = AF_INET; 
    tempAddr.sin_port = htons(0); 
    tempAddr.sin_addr.s_addr = inet_addr("169.254.222.0"); 

	vector<struct NexthopAndWeight> NexthopAndWeight_vector;
	struct NexthopAndWeight nexthopAndWeight;
	nexthopAndWeight.weight=20;
	nexthopAndWeight.NICName="lo";
	NexthopAndWeight_vector.push_back(nexthopAndWeight);
	nexthopAndWeight.weight=10;
	nexthopAndWeight.NICName="lo";
	NexthopAndWeight_vector.push_back(nexthopAndWeight);

	if(kernelRouteChangeTimes%2==1)
		route_add_multi_path(tempAddr,24,NexthopAndWeight_vector);
	else
		route_del(tempAddr, 24);

	gettimeofday(&tvEnd,NULL);
	kernelRouteChangeTimeElapsed=kernelRouteChangeTimeElapsed+time_diff(tvStart,tvEnd);
	return;
	//Test code for modify kernel routes;

	if(route.dstNode.ID==-1)//Default route
	{
		printf("=== Change default route ===\n");
		printf("dst(ToR: *.*)\t");
		printf("nexthop(");
		for(int i=0;i<AGG_PER_POD;i++)
		{
			if(route.nextHop[i].weight!=0)
				printf("%d.%d:%d, "
					,route.nextHop[i].switchNode.level
					,route.nextHop[i].switchNode.ID
					,route.nextHop[i].weight);
		}
		printf(")\n");
	}
	else if(route.dstNode.level==-1)//Default pod route
	{
		printf("=== Change default pod route ===\n");
		if(route.numOfWorkingNextHops!=defaultRoute.numOfWorkingNextHops)
			//Different to default route, add this one
			printf("route add:\t");
		else
			//Same to default route, delete this one
			printf("route del:\t");
		printf("dst(pod: %d.*)\t",int(route.dstNode.ID/TOR_PER_POD));
		printf("nexthop(");
		for(int i=0;i<AGG_PER_POD;i++)
		{
			if(route.nextHop[i].weight!=0)
				printf("%d.%d:%d, "
					,route.nextHop[i].switchNode.level
					,route.nextHop[i].switchNode.ID
					,route.nextHop[i].weight);
		}
		printf(")\n");
	}
	else//Route to specific ToR 
	{
		if(route.numOfWorkingNextHops!=defaultRoute.numOfWorkingNextHops
			&& route.numOfWorkingNextHops!=defaultRouteToPod[int(route.dstNode.ID/TOR_PER_POD)].numOfWorkingNextHops)
			//Different to default route and route to pod, add this one
			printf("route add:\t");
		else
			//Same to default route or route to pod, delete this one
			printf("route del:\t");
		printf("dst(%d.%d)\t",route.dstNode.level,route.dstNode.ID);
		printf("nexthop(");
		for(int i=0;i<AGG_PER_POD;i++)
		{
			if(route.nextHop[i].weight!=0)
				printf("%d.%d:%d, "
					,route.nextHop[i].switchNode.level
					,route.nextHop[i].switchNode.ID
					,route.nextHop[i].weight);
		}
		printf(")\t");
		printf("\n");
	}

	gettimeofday(&tvEnd,NULL);
	kernelRouteChangeTimeElapsed=kernelRouteChangeTimeElapsed+time_diff(tvStart,tvEnd);
}

void generateDefaultRoute(SwitchNode mySwitchNode)
{
	defaultRoute.dstNode.ID=-1;
	defaultRoute.dstNode.level=-1;
	for(int j=0;j<AGG_PER_POD;j++)
	{
		defaultRoute.nextHop[j].switchNode.level=2;
		defaultRoute.nextHop[j].switchNode.ID=j+int(mySwitchNode.ID/TOR_PER_POD)*AGG_PER_POD;
		defaultRoute.nextHop[j].weight=CORE_PER_AGG;
	}
	defaultRoute.numOfWorkingNextHops=AGG_PER_POD;

	defaultRouteToPod=(RouteTableEntry *)malloc(sizeof(RouteTableEntry)*POD_NUM);
	for(int i=0;i<POD_NUM;i++)
	{
		defaultRouteToPod[i].dstNode.level=-1;
		defaultRouteToPod[i].dstNode.ID=i*TOR_PER_POD; //The first ToR in this pod
		for(int j=0;j<AGG_PER_POD;j++)
		{
			defaultRouteToPod[i].nextHop[j].switchNode.level=2;
			defaultRouteToPod[i].nextHop[j].switchNode.ID=j+int(mySwitchNode.ID/TOR_PER_POD)*AGG_PER_POD;
			defaultRouteToPod[i].nextHop[j].weight=CORE_PER_AGG;
		}
		defaultRouteToPod[i].numOfWorkingNextHops=AGG_PER_POD;
	}
}

void generateRouteTable(SwitchNode mySwitchNode)
{
	printf("------- generateRouteTable for switch(%d.%d) -------\n",mySwitchNode.level,mySwitchNode.ID);
	printf("total # of routes:%d, total memory consumption of route table:%.2f MB\n",TOTAL_ROUTE_NUM, sizeof(RouteTableEntry)*TOTAL_ROUTE_NUM/1E6);
	
	routeTable=(RouteTableEntry *)malloc(sizeof(RouteTableEntry)*TOTAL_ROUTE_NUM);

	for(int i=0;i<TOTAL_ROUTE_NUM;i++)
	{
		routeTable[i].dstNode.level=1;
		routeTable[i].dstNode.ID=i;
		for(int j=0;j<AGG_PER_POD;j++)
		{
			routeTable[i].nextHop[j].switchNode.level=2;
			routeTable[i].nextHop[j].switchNode.ID=j+int(mySwitchNode.ID/TOR_PER_POD)*AGG_PER_POD;
			routeTable[i].nextHop[j].weight=CORE_PER_AGG;
		}
		routeTable[i].numOfWorkingNextHops=AGG_PER_POD;
	}
}

void generatePathTable(SwitchNode mySwitchNode)
{
	printf("------- generatePathTable for switch(%d.%d) -------\n",mySwitchNode.level,mySwitchNode.ID);
	printf("total # of paths:%d, total memory consumption of path table:%.2f MB\n",TOTAL_PATH_NUM, sizeof(PathTableEntry)*TOTAL_PATH_NUM/1E6);
	

	pathTable=(PathTableEntry *)malloc(sizeof(PathTableEntry)*TOTAL_PATH_NUM);

	int pathTableEntryID=0;
	//Generate paths for each TOR dstNode
	for(int i=0;i<TOR_PER_POD*POD_NUM;i++)
	{
		for(int j=0;j<AGG_PER_POD;j++)
		{
			for(int k=0;k<CORE_PER_AGG;k++)
			{
				if(i!=mySwitchNode.ID)
				{
					pathTable[pathTableEntryID].path.srcNode.level=mySwitchNode.level;
					pathTable[pathTableEntryID].path.srcNode.ID=mySwitchNode.ID;

					pathTable[pathTableEntryID].path.nextHop[0].level=2;
					pathTable[pathTableEntryID].path.nextHop[0].ID=j+int(mySwitchNode.ID/TOR_PER_POD)*AGG_PER_POD;
					
					if(int(i/TOR_PER_POD)!=int(mySwitchNode.ID/TOR_PER_POD))
					{
						pathTable[pathTableEntryID].path.nextHop[1].level=3;
						pathTable[pathTableEntryID].path.nextHop[1].ID=k+j%AGG_PER_POD*CORE_PER_AGG;
					
						pathTable[pathTableEntryID].path.nextHop[2].level=2;
						pathTable[pathTableEntryID].path.nextHop[2].ID=j+int(i/TOR_PER_POD)*AGG_PER_POD;
						
						pathTable[pathTableEntryID].path.nextHop[3].level=1;
						pathTable[pathTableEntryID].path.nextHop[3].ID=i;

						pathTable[pathTableEntryID].path.dstHop=3;
					}
					else//Paths to the same pod only has one nexthop
					{
						pathTable[pathTableEntryID].path.nextHop[1].level=1;
						pathTable[pathTableEntryID].path.nextHop[1].ID=i;
					
						pathTable[pathTableEntryID].path.nextHop[2].level=-1;
						pathTable[pathTableEntryID].path.nextHop[2].ID=-1;

						pathTable[pathTableEntryID].path.nextHop[3].level=-1;
						pathTable[pathTableEntryID].path.nextHop[3].ID=-1;

						pathTable[pathTableEntryID].path.dstHop=1;
					}
				}
				else//Don't store the paths to myself
				{
					pathTable[pathTableEntryID].path.srcNode.level=-1;
					pathTable[pathTableEntryID].path.srcNode.ID=-1; 

					for(int n=0;n<MAX_PATH_LENGTH;n++)
					{
						pathTable[pathTableEntryID].path.nextHop[n].level=-1;
						pathTable[pathTableEntryID].path.nextHop[n].ID=-1;
					}

					pathTable[pathTableEntryID].path.dstHop=-1;
				}

				pathTable[pathTableEntryID].FL=0;

				pathTableEntryID++;
			}
		}
	}
}

void printPathTable()
{
	for(int i=0;i<TOTAL_PATH_NUM;i++)
	{
		printf("PathTableEntry[%5d]:\t",i);
		printPathTableEntry(pathTable[i]);
		printf("\n");
	}
}

void generateRandomSwitch(int level, SwitchNode *testSwitchNode)
{
	testSwitchNode->level=level;
	switch (level)
	{
		case 1:
			testSwitchNode->ID=rand()%(TOR_PER_POD*POD_NUM);
			break;
		case 2:
			testSwitchNode->ID=rand()%(AGG_PER_POD*POD_NUM);
			break;
		case 3:
			testSwitchNode->ID=rand()%(AGG_PER_POD*CORE_PER_AGG);
			break;
		default:
			fprintf(stderr, "generateRandomSwitch level=%d, ERROR!\n", level);
			exit(1);
	}
}

bool checkLinkInPath(Link link, Path path)
{
	if(sameLink(link,path.srcNode,path.nextHop[0]))
		return true;
	else
	{
		for(int n=0;n<MAX_PATH_LENGTH-1;n++)
		{
			if(sameLink(link,path.nextHop[n],path.nextHop[n+1]))
				return true;
		}
	} 
	return false;
}

void checkRouteTableAndChangeKernelRoute(PathTableEntry pathEntry, int fromLiveToDead)
//fromLiveToDead: -1 means from live to dead, 1 means dead to live, others means no liveness change to this path
//This is only for change route to specific ToR
{
	int routeEntryID=GetDstNodeIDFromPathTableEntry(pathEntry);
	int routeNextHopID=pathEntry.path.nextHop[0].ID%AGG_PER_POD;
	routeTable[routeEntryID].nextHop[routeNextHopID].weight=routeTable[routeEntryID].nextHop[routeNextHopID].weight+fromLiveToDead;

	// printf("checkRouteTableAndChangeKernelRoute\tfromLiveToDead(%d)\t",fromLiveToDead);
	// printPathTableEntry(pathEntry);
	// printf("\n");

	// printRouteTable();

	if(routeTable[routeEntryID].nextHop[routeNextHopID].weight==0 && fromLiveToDead==-1)
	{
		//A nexthop down.
		//Delete a nexthop for this destination. 
		//May add specific route to this dest with longer prefix, without this nexthop.
		routeTable[routeEntryID].numOfWorkingNextHops--;
		changeKernelRoute(routeTable[routeEntryID]);
	}				
	else if(routeTable[routeEntryID].nextHop[routeNextHopID].weight==1 && fromLiveToDead==1)
	{
		//A nexthop back.
		//Add back a nexthop for this destination. 
		//May delete specific route to this dest with longer prefix.
		routeTable[routeEntryID].numOfWorkingNextHops++;
		if(routeTable[routeEntryID].numOfWorkingNextHops==AGG_PER_POD)
			changeKernelRoute(routeTable[routeEntryID]);
	}
}

void findAndUpdatePathTableBruteForce(int testLinkID, bool linkState)
{
	Link testLink=linkTable[testLinkID].link;
	bool found=false;
	for(int i=0;i<TOTAL_PATH_NUM;i++)
	{
		if(checkLinkInPath(testLink,pathTable[i].path))
		{
			if(linkState==true)
				pathTable[i].FL--;
			else
				pathTable[i].FL++;

			found=true;
		}
	}
	if(!found)
	{
		fprintf(stderr, "No PathTableEntry found, ERROR!\n");
		printf("---- Test link\t");
  		printLink(testLink);
  		printf("linkState:%d\t",linkState);
  		printf(" ----\n");
		exit(1);
	}
}

int checkPathTableEntryDestToThisToRorThisPod(PathTableEntry entry)
{
	///See if this path table entry's destination is to this ToR or this pod
	// 1: this ToR; 2: this pod; else: not ToR or pod
	if(entry.path.dstHop==-1)
		return 1;
	else if(entry.path.nextHop[entry.path.dstHop].ID==entry.path.srcNode.ID)
		return 1;
	else if(int(entry.path.nextHop[entry.path.dstHop].ID/TOR_PER_POD)==int(entry.path.srcNode.ID/TOR_PER_POD))
		return 2;
	else
		return 0;
}

void findAndUpdatePathTable(int testLinkID, bool linkState)
{
	Link testLink=linkTable[testLinkID].link;
	// printf("findAndUpdatePathTable testLinkID: %d, type: %d\n"
	// 	,testLinkID
	// 	,linkTable[testLinkID].affectArea.type);
	switch(linkTable[testLinkID].affectArea.type)
	{
		case 1://ToR(other)<->Agg
			for(int i=linkTable[testLinkID].affectArea.firstEntry;
				i<linkTable[testLinkID].affectArea.firstEntry+CORE_PER_AGG;
				i++)
			{
				//Jump the path block to this ToR
				if(checkPathTableEntryDestToThisToRorThisPod(pathTable[i])!=1)
				{
					if(linkState==true){
						pathTable[i].FL--;
						if(pathTable[i].FL==0)//Dead to live
							checkRouteTableAndChangeKernelRoute(pathTable[i],1);
					}
					else{
						pathTable[i].FL++;
						if(pathTable[i].FL==1)//Live to dead
							checkRouteTableAndChangeKernelRoute(pathTable[i],-1);
					}
				}
			}
			break;
		case 2://Agg(myself pod)<->Core
			{
				int AggID;
				//Link is bidirectional. Check direction first
				if(testLink.srcNode.level==2)
					AggID=testLink.srcNode.ID;
				else if(testLink.dstNode.level==2)
					AggID=testLink.dstNode.ID;
				if(linkState==true){
					defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight++;
					// Check whether the pod route should change
					if(defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight==1)
					{
						defaultRouteToPod[int(AggID/AGG_PER_POD)].numOfWorkingNextHops++;
						changeKernelRoute(defaultRouteToPod[int(AggID/AGG_PER_POD)]);
					}
				}
				else {
					defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight--;
					// Check whether the pod route should change
					if(defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight==0)
					{
						defaultRouteToPod[int(AggID/AGG_PER_POD)].numOfWorkingNextHops--;
						changeKernelRoute(defaultRouteToPod[int(AggID/AGG_PER_POD)]);
					}	
				}

				for(int i=linkTable[testLinkID].affectArea.firstEntry;
					i<TOTAL_PATH_NUM;
					i=i+CORE_PER_AGG*AGG_PER_POD)
				{
					//Jump the path block to this pod and this ToR
					if(checkPathTableEntryDestToThisToRorThisPod(pathTable[i])!=2 
						&& checkPathTableEntryDestToThisToRorThisPod(pathTable[i])!=1)
					{
						if(linkState==true){
							pathTable[i].FL--;
							// if(pathTable[i].FL==0)//Dead to live
							// 	checkRouteTableAndChangeKernelRoute(pathTable[i],1);
						}
						else{
							pathTable[i].FL++;
							// if(pathTable[i].FL==1)//Live to dead
							// 	checkRouteTableAndChangeKernelRoute(pathTable[i],-1);
						}
					}
				}
			}
			break;
		case 3://ToR(this)<->Agg
			{
				if(linkState==true)
				{
					//Link is bidirectional. Check direction first
					if(testLink.srcNode.level==2)
						defaultRoute.nextHop[testLink.srcNode.ID%AGG_PER_POD].weight=CORE_PER_AGG;
					else if(testLink.dstNode.level==2)
						defaultRoute.nextHop[testLink.dstNode.ID%AGG_PER_POD].weight=CORE_PER_AGG;
					defaultRoute.numOfWorkingNextHops++;
				}
				else
				{
					if(testLink.srcNode.level==2)
						defaultRoute.nextHop[testLink.srcNode.ID%AGG_PER_POD].weight=0;
					else if(testLink.dstNode.level==2)
						defaultRoute.nextHop[testLink.dstNode.ID%AGG_PER_POD].weight=0;
					defaultRoute.numOfWorkingNextHops--;
				}
				// This link type always change default route, rather than specific route
				changeKernelRoute(defaultRoute);

				int k=0;
				for(int i=0;i<TOR_PER_POD*POD_NUM;i++)
				{
					for(int j=0;j<CORE_PER_AGG;j++)
					{
						k=linkTable[testLinkID].affectArea.firstEntry+i*AGG_PER_POD*CORE_PER_AGG+j;
						//Jump the path block to this ToR
						if(checkPathTableEntryDestToThisToRorThisPod(pathTable[k])!=1)
						{
							if(linkState==true){
								pathTable[k].FL--;
							}
							else{
								pathTable[k].FL++;
							}
						}
					}
				}
			}
			break;
		case 4://Agg(other pod)<->Core
			{
				int AggID;
				//Link is bidirectional. Check direction first
				if(testLink.srcNode.level==2)
					AggID=testLink.srcNode.ID;
				else if(testLink.dstNode.level==2)
					AggID=testLink.dstNode.ID;
				if(linkState==true){
					defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight++;
					// Check whether the pod route should change
					if(defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight==1)
					{
						defaultRouteToPod[int(AggID/AGG_PER_POD)].numOfWorkingNextHops++;
						changeKernelRoute(defaultRouteToPod[int(AggID/AGG_PER_POD)]);
					}
				}
				else {
					defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight--;
					// Check whether the pod route should change
					if(defaultRouteToPod[int(AggID/AGG_PER_POD)].nextHop[AggID%AGG_PER_POD].weight==0)
					{
						defaultRouteToPod[int(AggID/AGG_PER_POD)].numOfWorkingNextHops--;
						changeKernelRoute(defaultRouteToPod[int(AggID/AGG_PER_POD)]);
					}	
				}
				
				for(int k=0;k<TOR_PER_POD;k++)
				{
					int i=linkTable[testLinkID].affectArea.firstEntry+k*CORE_PER_AGG*AGG_PER_POD;
					if(linkState==true){
						pathTable[i].FL--;
						// if(pathTable[i].FL==0)//Dead to live
						// 	checkRouteTableAndChangeKernelRoute(pathTable[i],1);
					}
					else{
						pathTable[i].FL++;
						// if(pathTable[i].FL==1)//Live to dead
						// 	checkRouteTableAndChangeKernelRoute(pathTable[i],-1);
					}
				}
			}
			break;
		default:
			fprintf(stderr, "Link[%d] Type[%d] ERROR!\n", testLinkID, linkTable[testLinkID].affectArea.type);
			exit(1);
			break;
	}
}

void connectToMaster(SwitchNode mySwitchNode, char *MASTER_IP)
{
	struct sockaddr_in masterAddr;
    memset(&masterAddr,0,sizeof(masterAddr)); //数据初始化--清零
    masterAddr.sin_family=AF_INET; //设置为IP通信
    masterAddr.sin_addr.s_addr=inet_addr(MASTER_IP);
    masterAddr.sin_port=htons(MASTER_LISTEN_PORT);

	int clientSock=0;
	int value=1;

	if ((clientSock=socket(PF_INET,SOCK_STREAM,0))<0)
    {
      fprintf(stderr,"TCPRoute Create Socket Failed.\n");
      exit(1);
    }

    if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)//设置端口复用
    {
      fprintf(stderr,"Set SO_REUSEPORT error.\n");
      exit(1);
    }

    if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
    {
      fprintf(stderr,"Set SO_REUSEADDR error.\n");
      exit(1);
    }

    if (setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
    {
		fprintf(stderr,"Set TCP_NODELAY error.\n");
      	exit(1);
	}

    int nRecvBuf=TCP_BUF_SIZE;
    setsockopt(clientSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
    // 发送缓冲区
    int nSendBuf=TCP_BUF_SIZE;
    setsockopt(clientSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
	if (getsockname(clientSock, (struct sockaddr *)&sin, &len) == -1)
	    perror("getsockname");
	
    while(1)
    {
    	if((connect(clientSock,(const struct sockaddr *)&masterAddr,sizeof(masterAddr)))==0)
    	{
    		printf("[%d.%d] Sock connected to master (%s:%d).\n"
    			,mySwitchNode.level,mySwitchNode.ID
    			,inet_ntoa(masterAddr.sin_addr),ntohs(masterAddr.sin_port));
    		fflush(stdout);
    		break;
    	}
    	else {
    		perror("connect");
    		fprintf(stderr,"[%d.%d] Sock connect error (%s:%d).\n"
    			,mySwitchNode.level,mySwitchNode.ID
    			,inet_ntoa(masterAddr.sin_addr),ntohs(masterAddr.sin_port));
    		usleep(CONNECT_INTERVAL);
    	}
    }

    MainChannelSock=clientSock;

    printf("[%d.%d] connected to master!\n",mySwitchNode.level,mySwitchNode.ID);
}

struct MNinfo// Master-Node通信的信息格式
{
	Link link;//Which link's state is being reported/delivered/responded
	SwitchNode reportNode;//Which switch reports this link. 
	//When reporting LS, this is the switch ID who reports. -1 means the master fakely generates this LS
	SwitchNode responseNode;//Which switch respond this link-state update
	bool linkState;
	unsigned int eventID;//event ID for this link, tagged by the reporter. Unique per reporter
	bool bye;//Used for master to notify finishing test
	unsigned int eventIDforSendResponse;
	//event ID for this link-state send/response, tagged by master. Unique for all events seen at master
	unsigned char ReptorDelvorResporAck;
	//0: a LS report; 1: a LS delivery; 2: a LS response from switch; 3 a LS ack to reporting switch

	//tiem perf info at switch side. Record the time for processing last LS at this switch
	double lastSwitchTimeFromEpollNotifyToTCPRecvDone;
	double lastSwitchTimeForRouteUpdateElapsed;
	double lastSwitchTimeFromRouteUpdateDoneToTCPSendDone;
};

void printCurrentTime()
{
	struct timeval now;
	gettimeofday(&now,NULL);

	printf("<%5ld.%6ld",now.tv_sec,now.tv_usec);
	if(lastPrintGlobalTime.tv_sec==0 && lastPrintGlobalTime.tv_usec==0)
		printf(" +%6dus>\t",0);
	else
		printf(" +%6dus>\t",(int)time_diff(lastPrintGlobalTime,now));
	gettimeofday(&lastPrintGlobalTime,NULL);
}

void printTime(struct timeval now)
{
	printf("<%5ld.%6ld",(now.tv_sec&0xFF),now.tv_usec);
	if(lastPrintGlobalTime.tv_sec==0 && lastPrintGlobalTime.tv_usec==0)
		printf(" +%6dus>\t",0);
	else
		printf(" +%6dus>\t",(int)time_diff(lastPrintGlobalTime,now));
	gettimeofday(&lastPrintGlobalTime,NULL);
}

void printMNinfo(MNinfo mn)
{
	printf("MNinfo: src(%d.%d), link[%d.%d-%d.%d] , linkstate(%d), eventID(%d), eventIDforSendResponse(%d), bye(%d)\t"
	,mn.reportNode.level
	,mn.reportNode.ID
	,mn.link.srcNode.level
	,mn.link.srcNode.ID
	,mn.link.dstNode.level
	,mn.link.dstNode.ID
	,mn.linkState
	,mn.eventID
	,mn.eventIDforSendResponse
	,mn.bye);
	switch(mn.ReptorDelvorResporAck)
	{
		case 0:
		printf("[LS report]");
		break;
		case 1:
		printf("[LS delivery]");
		break;
		case 2:
		printf("[LS response from switch]");
		break;
		case 3:
		printf("[LS ack to reporting switch]");
		break;
		default:
		printf("[Unknown LS type!!]");
		break;
	}
	printf("\t");
	
	printf("lastSwitchTimeFromEpollNotifyToTCPRecvDone:%.0lf us,\tlastSwitchTimeForRouteUpdateElapsed:%.0lf us,\tlastSwitchTimeFromRouteUpdateDoneToTCPSendDone:%.0lf us"
		,mn.lastSwitchTimeFromEpollNotifyToTCPRecvDone
		,mn.lastSwitchTimeForRouteUpdateElapsed
		,mn.lastSwitchTimeFromRouteUpdateDoneToTCPSendDone);
	printf("\t");
}

void recvMessageFromSock(MNinfo *tempMNInfo, int sock)
{
	int totalRecvSize=0;
	char recvBuf[sizeof(struct MNinfo)];
	memset(recvBuf,0,sizeof(recvBuf));
	while(totalRecvSize<sizeof(struct MNinfo))
	{
		int recvSize=0;
	    if((recvSize=recv(sock,recvBuf+totalRecvSize,sizeof(struct MNinfo)-totalRecvSize,0))<=0)
	    {
	      fprintf(stderr,"Sock(%d) recv error. totalRecvSize: %d\n"
	      	,sock
	      	,totalRecvSize);
	      exit(1);
	    }
	    totalRecvSize=totalRecvSize+recvSize;
	}
	memcpy(tempMNInfo,recvBuf,sizeof(struct MNinfo));

	if(IF_PRINT_DEBUG_INFO)
	{
		printf("recvMessageFromSock:\t");
		printMNinfo(*tempMNInfo);
		printf("\n");
		fflush(stdout);
	}
}

int sendMessageToSock(MNinfo tempMNInfo, int sock)
{
	char sendBuf[sizeof(struct MNinfo)];
  	memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));
	while(send(sock,sendBuf,sizeof(struct MNinfo),0)<=0)
	{
		fprintf(stderr,"ERROR: Sock(%d) send message error.\n"
			,sock);
		perror("sendMessageToSock");
		printf("ERROR: Sock(%d) send message error.\n"
			,sock);
		printf("Should have sent:\t");
		printMNinfo(tempMNInfo);
		fflush(stdout);
    	exit(1);
	}

	if(IF_PRINT_DEBUG_INFO)
	{
		printf("sendMessageToSock:\t");
		printMNinfo(tempMNInfo);
		printf("\n");
		fflush(stdout);
	}
	return 1;
}

struct RecvFuncPara 
{
	SwitchNode mySwitchNode; 
	int cotestWithMaster;
	int testPrimus;
	int ifDoSwitchCalc;
	int testTimes;
	int ifMonitorLocalNICChange;
};

struct HelloMsg
{
	SwitchNode mySwitchNode;
	int cotestWithMaster;
};

void initHelloToMasterSayImReadyToWork(RecvFuncPara para)
{
	SwitchNode mySwitchNode;
	mySwitchNode.level=para.mySwitchNode.level;
	mySwitchNode.ID=para.mySwitchNode.ID;
  	int cotestWithMaster=para.cotestWithMaster;
	int testPrimus=para.testPrimus;
	int ifDoSwitchCalc=para.ifDoSwitchCalc;
	int testTimes=para.testTimes;
	int ifMonitorLocalNICChange=para.ifMonitorLocalNICChange;

	HelloMsg msg;
	msg.mySwitchNode.level=mySwitchNode.level;
	msg.mySwitchNode.ID=mySwitchNode.ID;
	msg.cotestWithMaster=cotestWithMaster;

	 //Send mySwitchNode info
    char sendBuf[sizeof(struct HelloMsg)];
  	memcpy(sendBuf,&msg,sizeof(struct HelloMsg));
	while(send(MainChannelSock,sendBuf,sizeof(struct HelloMsg),0)<=0)
	{
		fprintf(stderr,"[%d.%d] Sock send hello (mySwitchNode info) error.\n"
			,mySwitchNode.level,mySwitchNode.ID);
    	exit(1);
	}
	printf("[%d.%d] hello done to master! \n",mySwitchNode.level,mySwitchNode.ID);
}

void updateLinkTableAndPathTableAndRoute(Link testLink, bool linkState, unsigned int eventID, int testPrimus, int ifDoSwitchCalc)
{
	if(!ifDoSwitchCalc)
		return;

	struct timeval tvStart,tvEnd,tvMiddle;

	gettimeofday(&tvStart,NULL);

	int entryID;
	if(testPrimus)
		entryID=getLinkTableEntryID(testLink);
	else
		entryID=getLinkTableEntryIDBruteForce(testLink);
	if(entryID>=TOTAL_LINK_NUM|| entryID<0)//Our smart index algorithm
	{
		fprintf(stderr, "No LinkTableEntry found, ERROR!\n");
		exit(1);
	}

	gettimeofday(&tvMiddle,NULL);

	if(eventID>linkTable[entryID].eventID && linkState!=linkTable[entryID].state)
	{
		linkTable[entryID].eventID=eventID;
		linkTable[entryID].state=linkState;

		if(testPrimus)
			findAndUpdatePathTable(entryID,linkState);
		else
			findAndUpdatePathTableBruteForce(entryID,linkState);
	}

	gettimeofday(&tvEnd,NULL);
	totalTimeForRouteUpdateElapsed=totalTimeForRouteUpdateElapsed+time_diff(tvStart,tvEnd);
	linkTableTimeElapsed=linkTableTimeElapsed+time_diff(tvStart,tvMiddle);
	pathTableTimeElapsed=pathTableTimeElapsed+time_diff(tvMiddle,tvEnd);

}

int createInterfaceNetlinkSocket()
{
	int nl_socket = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (nl_socket < 0)
	{
	  printf ("NETLINK_ROUTE Socket Open Error!");
	  exit (1);
	}

	// int option = 1;
	// setsockopt(nl_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	struct sockaddr_nl addr;
	memset ((void *) &addr, 0, sizeof (addr));

	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid ();//Kernel set message to this thread
	addr.nl_groups = RTMGRP_LINK;

	if (bind (nl_socket, (struct sockaddr *) &addr, sizeof (addr)) < 0)
	{
	  perror ("createInterfaceNetlinkSocket Socket bind failed!");
	  exit (1);
	}

	return nl_socket;
}

int createRouteNetlinkSocket()
{
	int nl_socket = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (nl_socket < 0)
	{
	  printf ("NETLINK_ROUTE Socket Open Error!");
	  exit (1);
	}

	// int option = 1;
	// setsockopt(nl_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	struct sockaddr_nl addr;
	memset ((void *) &addr, 0, sizeof (addr));

	addr.nl_family = AF_NETLINK;
	addr.nl_pid = 0;//This thread send message to kernel
	addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

	if (bind (nl_socket, (struct sockaddr *) &addr, sizeof (addr)) < 0)
	{
	  perror ("createRouteNetlinkSocket Socket bind failed!");
	  exit (1);
	}

	return nl_socket;
}

int createAndAddRecvEpollEvents(RecvFuncPara para)
{
	int epoll_fd;
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
    {
      perror ("epoll_create");
      exit(1);
    }
   
    //Add master socket recv monitor events
    if(para.cotestWithMaster!=0)
    {
	    struct epoll_event event1;
	    event1.events = EPOLLIN;
		event1.data.fd=MainChannelSock;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, MainChannelSock, &event1) == -1)
		{
		  perror ("EPOLL_CTL_ADD MainChannelSock");
		  exit(1);
		}
	}

	//Add link status monitor events
	if(para.ifMonitorLocalNICChange!=0)
    {
		struct epoll_event event2;
	    event2.events = EPOLLIN | EPOLLET;
		event2.data.fd=RTNetlinkSockForNICMonitor;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, RTNetlinkSockForNICMonitor, &event2) == -1)
		{
		  perror ("EPOLL_CTL_ADD RTNetlinkSockForNICMonitor");
		  exit(1);
		}
	}

	//Add epoll events
	//	OthersIndirectChannelSockTable[MAX_INDIRECT_CHANNEL_NUM];

	//Add epoll events
	//	UDPSockTable[MAX_UDP_CHANNEL_NUM];
	
	return epoll_fd;
}

void generateAFakeLSReport(RecvFuncPara para, MNinfo *tempMNInfo)
{
	SwitchNode mySwitchNode;
	mySwitchNode.level=para.mySwitchNode.level;
	mySwitchNode.ID=para.mySwitchNode.ID;
  	int cotestWithMaster=para.cotestWithMaster;
	int testPrimus=para.testPrimus;
	int ifDoSwitchCalc=para.ifDoSwitchCalc;
	int testTimes=para.testTimes;
	int ifMonitorLocalNICChange=para.ifMonitorLocalNICChange;

	
	// usleep(100000);//Push a LS report every 100ms
	// tempMNInfo->link.srcNode.level=1;
	// tempMNInfo->link.srcNode.ID=0;
	// tempMNInfo->link.dstNode.level=2;
	// tempMNInfo->link.dstNode.ID=0;
	generateRandomLink(&(tempMNInfo->link));

	tempMNInfo->linkState=!(linkTable[getLinkTableEntryID(tempMNInfo->link)].state);
	tempMNInfo->eventID=linkTable[getLinkTableEntryID(tempMNInfo->link)].eventID+1;
	tempMNInfo->reportNode.level=mySwitchNode.level;
	tempMNInfo->reportNode.ID=mySwitchNode.ID;
	tempMNInfo->bye=false;
	tempMNInfo->eventIDforSendResponse=0;//This is going to set by master, with globally unique number per master
	tempMNInfo->ReptorDelvorResporAck=0;//0 means LS report
}

//return value:
//0 means no new LS generate, 1 means a new LS generate packet in the tempMNinfo and return
int readLinkStateFromNetlinkSock(RecvFuncPara para, MNinfo *tempMNInfo)
{
	int ifMonitorLocalNICChange=para.ifMonitorLocalNICChange;
	SwitchNode mySwitchNode;
	mySwitchNode.level=para.mySwitchNode.level;
	mySwitchNode.ID=para.mySwitchNode.ID;

	int status;
	char buf[4096];
	struct iovec iov = { buf, sizeof buf };
	struct sockaddr_nl snl;
	struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
	struct nlmsghdr *h;
	struct ifinfomsg *ifi;

	status = recvmsg (RTNetlinkSockForNICMonitor, &msg, 0);

	if (status <= 0)
	{
		perror ("read_netlink: Error: ");
		exit(1);
	}

	char ifname[IF_NAMESIZE]={0};
	// We need to handle more than one message per 'recvmsg'
	for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status);
	   h = NLMSG_NEXT (h, status))
	{
		if(IF_PRINT_DEBUG_INFO)
			printf("(message type %4x)\t",h->nlmsg_type);

		memset(ifname,0,sizeof(char)*IF_NAMESIZE);

		switch(h->nlmsg_type)// check man page for netlink(7) and rtnetlink(7)
		{
			case NLMSG_DONE:
			// printf("read_netlink: done reading\n");
			break;
			case NLMSG_NOOP:
			// printf("read_netlink: noop message\n");
			break;
			case NLMSG_ERROR:
				perror("read_netlink: message error\n");
				exit(1);
			break;
			case RTM_NEWLINK:
				ifi = (struct ifinfomsg *)NLMSG_DATA (h);
				if(IF_PRINT_DEBUG_INFO)
				{
					if_indextoname(ifi->ifi_index,ifname);
					printf ("RTM_NEWLINK:: if %d(%s)\t%s\n", ifi->ifi_index, ifname, (ifi->ifi_flags & IFF_RUNNING) ? "Up" : "Down");
					fflush(stdout);
				}
				//link state should be (ifi->ifi_flags & IFF_RUNNING)
				generateAFakeLSReport(para,tempMNInfo);
				return 1;
			break;
		}
	}

	return 0;//No new LS generate
}

void waitForReadyToRoll()
{
	int totalRecvSize=0;
	char recvBuf[1000]={0};
	while(totalRecvSize<strlen("ready to roll"))
	{
		int recvSize=0;
	    if((recvSize=recv(MainChannelSock,recvBuf+totalRecvSize,strlen("ready to roll")-totalRecvSize,0))<=0)
	    {
	      fprintf(stderr,"Master Sock recv error. totalRecvSize: %d\n"
	      	,totalRecvSize);
	      exit(1);
	    }
	    totalRecvSize=totalRecvSize+recvSize;
	}
	printf("\n\n\n %s TEST begin! Start to report LS ...\n\n",recvBuf);
	fflush(stdout);
}

void recvLSMsgsAndLocalLinkChangesAndProcessAll(RecvFuncPara para, int epoll_fd)
{
	//This function does all the switch needs to do
	//Use an epoll to monitor messages received from 
	//1) master, 2) other switches, 3) local NIC (through RT_NETLINK)
	SwitchNode mySwitchNode;
	mySwitchNode.level=para.mySwitchNode.level;
	mySwitchNode.ID=para.mySwitchNode.ID;
  	int cotestWithMaster=para.cotestWithMaster;
	int testPrimus=para.testPrimus;
	int ifDoSwitchCalc=para.ifDoSwitchCalc;
	int testTimes=para.testTimes;
	int ifMonitorLocalNICChange=para.ifMonitorLocalNICChange;

	printf("Start recvLSMsgsAndLocalLinkChangesAndProcessAll on core(%d)\n"
			,sched_getcpu()
			);
	fflush(stdout);

	struct epoll_event events[MAX_EPOLL_EVENTS_PER_RECV];
	int received_socks_num=0;

	int recvStat;
	int recvSock;
	MNinfo tempMNInfo;

	struct timeval tvStartReport,tvAfterReport;
	struct timeval tvAfterEpollNotified,tvAfterReceiveUpdate;
	struct timeval tvBeforeSendResponse,tvAfterSendResponse;
	struct timeval tvAfterReceiveAck;

	while(1)
	{
		///********Code after this are for test mode
		if(cotestWithMaster==1)//I'm a LS reporter to master
			if(totalLSUpdateReported==0)//First LS report
				waitForReadyToRoll();

		if(cotestWithMaster==0 && totalLSUpdateReported>=testTimes)//Test all locally
		{
			printf("\n\n\nLocal complete: \n");
			break;//Finish test
		}

		//LS not generated from NIC monitoring and received from master,
		//generate a random LS report
		if(ifMonitorLocalNICChange==0 && cotestWithMaster!=2 )
		{
			if(totalLSUpdateReportACKed==totalLSUpdateReported	
				&& totalLSUpdateReported<testTimes)// new LS only after former one has all been processed
			{
				generateAFakeLSReport(para,&tempMNInfo);
				totalLSUpdateReported++;
				if(totalLSUpdateReported%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
				{
					printCurrentTime();
					printf("++++ %d LS report ",totalLSUpdateReported);
					if(cotestWithMaster!=0)
						printf("sending to master:\t");
					else
						printf("to process locally:\t");
			  		printMNinfo(tempMNInfo);
			  		printf("core(%d) ++++\n",sched_getcpu());
			  		fflush(stdout);
			  	}
			  	gettimeofday(&tvAllStart,NULL);

			  	if(cotestWithMaster==1)//Send this fake LS report to master
				{
					gettimeofday(&tvStartReport,NULL);
					sendMessageToSock(tempMNInfo,MainChannelSock);
					gettimeofday(&tvAfterReport,NULL);
					timeElapsedTillLSReportSentOut=timeElapsedTillLSReportSentOut+time_diff(tvAllStart,tvAfterReport);
				}
				else//Not test with master, just calc locally
				{
					totalLSUpdateReceived++;
					updateLinkTableAndPathTableAndRoute(tempMNInfo.link,tempMNInfo.linkState,tempMNInfo.eventID,testPrimus,ifDoSwitchCalc);
					totalLSUpdateReportACKed++;

					gettimeofday(&tvAfterReceiveAck,NULL);
					totalTimeElapsed=totalTimeElapsed+time_diff(tvAllStart,tvAfterReceiveAck);
					timeElapsedTillLSACKReceived=timeElapsedTillLSACKReceived+time_diff(tvAllStart,tvAfterReceiveAck);
			
					if(totalLSUpdateReportACKed%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
			  		{
			  			printCurrentTime();
						printf("++++ %d LS ack all processing done locally:\t",totalLSUpdateReportACKed);
				  		printMNinfo(tempMNInfo);
				  		printf("core(%d)  ++++\n",sched_getcpu());
				  		fflush(stdout);
				  	}
				  	continue;
				}
			}
		}
		///**************Code before this are for test mode
		
		//Get recv events from epoll
		//Can be LS message from master, and NIC change message from kernel
		if(epoll_fd!=0)
			received_socks_num=epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS_PER_RECV, -1);
		
		for(int i=0;i<received_socks_num;i++)
		{
			recvSock=events[i].data.fd;//The socks having coming data
			if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)))
			{
				fprintf(stderr,"Epoll event: Recv sock(%d) closed... Stop the recv thread.\n",recvSock);
				shutdown(recvSock,SHUT_RDWR);
				close(recvSock);
				return;
			}

			// printf("Receive epoll event from sock(%d), RTNetlinkSockForNICMonitor(%d), MainChannelSock(%d)\n"
			// 	,recvSock,RTNetlinkSockForNICMonitor,MainChannelSock);
				

			if(recvSock==RTNetlinkSockForNICMonitor)//If NIC change events
			{
				if(IF_PRINT_DEBUG_INFO)
					printf("Receive RT_NETLINK events from RTNetlinkSockForNICMonitor(%d)\n",RTNetlinkSockForNICMonitor);
				
				int ret=0;
				if(ifMonitorLocalNICChange==1)
					ret=readLinkStateFromNetlinkSock(para, &tempMNInfo);
				//Else ignore link changes

				if(ret==1)//New LS generate
				{
					totalLSUpdateReported++;
					if(totalLSUpdateReported%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
				  	{
						printCurrentTime();
						printf("++++ %d LS report ",totalLSUpdateReported);
						if(cotestWithMaster!=0)
							printf("sending to master:\t");
						else
							printf("to process locally:\t");
				  		printMNinfo(tempMNInfo);
				  		printf("core(%d) ++++\n",sched_getcpu());
				  		fflush(stdout);
				  	}

				  	gettimeofday(&tvAllStart,NULL);
					if(cotestWithMaster==1)//Report to master
			  		{
				  		gettimeofday(&tvStartReport,NULL);
				  		sendMessageToSock(tempMNInfo,MainChannelSock);
				  		gettimeofday(&tvAfterReport,NULL);
				  		timeElapsedTillLSReportSentOut=timeElapsedTillLSReportSentOut+time_diff(tvAllStart,tvAfterReport);
				  	}
				  	else if(cotestWithMaster==0)//Test calc locally
				  	{
				  		totalLSUpdateReceived++;
				  		updateLinkTableAndPathTableAndRoute(tempMNInfo.link,tempMNInfo.linkState,tempMNInfo.eventID,para.testPrimus,para.ifDoSwitchCalc);
				  		totalLSUpdateReportACKed++;

						gettimeofday(&tvAfterReceiveAck,NULL);
						totalTimeElapsed=totalTimeElapsed+time_diff(tvAllStart,tvAfterReceiveAck);
						timeElapsedTillLSACKReceived=timeElapsedTillLSACKReceived+time_diff(tvAllStart,tvAfterReceiveAck);
				  		
				  		if(totalLSUpdateReportACKed%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
				  		{
				  			printCurrentTime();
							printf("++++ %d LS ack all processing done locally:\t",totalLSUpdateReportACKed);
					  		printMNinfo(tempMNInfo);
					  		printf("core(%d)  ++++\n",sched_getcpu());
					  		fflush(stdout);
					  	}
				  	}
				  	//else: Only recv/send mode, ignore this link-state change
				}
			}
			else if(recvSock==MainChannelSock)//If is recv from master event
			{
				if(cotestWithMaster==2)//Every thing starts from receiving LS from master
					gettimeofday(&tvAllStart,NULL);
				gettimeofday(&tvAfterEpollNotified,NULL);
				
				// printf("tvAllStart: <%ld.%06ld>\n",tvAllStart.tv_sec,tvAllStart.tv_usec);
						
				recvMessageFromSock(&tempMNInfo,recvSock);
				if(tempMNInfo.bye)
				{
					printf("\n\nTEST end: Receive bye from master....\n\n");
					return;
				}

				switch(tempMNInfo.ReptorDelvorResporAck)
				{
					case 0://LS report. 
					//Should only happen when forwarding other switches' report
					//Can be either received TCP main message (through indirect channel) or UDP copies
					break;
					case 1://LS delivery from master.
					//Should update local route table
					//Or forward to other switches (either TCP main message through indirect channel, or, UDP copies) 
						timeElapsedTillLSEpollNotified=timeElapsedTillLSEpollNotified+time_diff(tvAllStart,tvAfterEpollNotified);
						
						gettimeofday(&tvAfterReceiveUpdate,NULL);
						timeElapsedTillLSUpdateReceived=timeElapsedTillLSUpdateReceived+time_diff(tvAllStart,tvAfterReceiveUpdate);

						totalLSUpdateReceived++;

						if(IF_PRINT_DEBUG_INFO)
						{
							printTime(tvAfterEpollNotified);
							printf("---- %d LS update received epoll notified:\t",totalLSUpdateReceived);
					  		printMNinfo(tempMNInfo);
					  		printf("core(%d) ----\n",sched_getcpu());
					  	}

						if(totalLSUpdateReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
						{
							printCurrentTime();
							printf("---- %d LS update received from TCP recv:\t",totalLSUpdateReceived);
					  		printMNinfo(tempMNInfo);
					  		printf("core(%d) ----\n",sched_getcpu());
					  		fflush(stdout);
					  	}

					  	updateLinkTableAndPathTableAndRoute(tempMNInfo.link,tempMNInfo.linkState,tempMNInfo.eventID,testPrimus,ifDoSwitchCalc);
						
						tempMNInfo.ReptorDelvorResporAck=2;//Send response back
						tempMNInfo.responseNode.level=mySwitchNode.level;
						tempMNInfo.responseNode.ID=mySwitchNode.ID;
						tempMNInfo.lastSwitchTimeFromEpollNotifyToTCPRecvDone=lastSwitchTimeFromEpollNotifyToTCPRecvDone;
						tempMNInfo.lastSwitchTimeForRouteUpdateElapsed=lastSwitchTimeForRouteUpdateElapsed;
						tempMNInfo.lastSwitchTimeFromRouteUpdateDoneToTCPSendDone=lastSwitchTimeFromRouteUpdateDoneToTCPSendDone;

						gettimeofday(&tvBeforeSendResponse,NULL);

						sendMessageToSock(tempMNInfo, recvSock);

						gettimeofday(&tvAfterSendResponse,NULL);
						timeElapsedTillLSResponseSentOut=timeElapsedTillLSResponseSentOut+time_diff(tvAllStart,tvAfterSendResponse);
						
						lastSwitchTimeFromEpollNotifyToTCPRecvDone=time_diff(tvAfterEpollNotified,tvAfterReceiveUpdate);
						lastSwitchTimeForRouteUpdateElapsed=time_diff(tvAfterReceiveUpdate,tvBeforeSendResponse);
						lastSwitchTimeFromRouteUpdateDoneToTCPSendDone=time_diff(tvBeforeSendResponse,tvAfterSendResponse);
						totalSwitchTimeFromRouteUpdateDoneToTCPSendDone+=lastSwitchTimeFromRouteUpdateDoneToTCPSendDone;

						if(cotestWithMaster==2)
							totalTimeElapsed=totalTimeElapsed+time_diff(tvAllStart,tvAfterSendResponse);	
						
						if(totalLSUpdateReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
						{
							printCurrentTime();
							printf("---- %d LS update response to master:\t",totalLSUpdateReceived);
					  		printMNinfo(tempMNInfo);
					  		printf("core(%d) ----\n",sched_getcpu());
					  		fflush(stdout);
					    }
					break;
					case 2://LS response from switch. 
					//Should only happen when forwarding other switches' response through indirect TCP channel 
					//Note that response has no UDP copies
					break;
					case 3://LS ack to reporting switch. 
					//Should only happen when the LS is reported by this switch 
					//or forwarding ack to other switches through indirect TCP channel 
					//Note that ack has no UDP copies
					    totalLSUpdateReportACKed++;
						gettimeofday(&tvAfterReceiveAck,NULL);
						totalTimeElapsed=totalTimeElapsed+time_diff(tvAllStart,tvAfterReceiveAck);
						timeElapsedTillLSACKReceived=timeElapsedTillLSACKReceived+time_diff(tvAllStart,tvAfterReceiveAck);
				
						if(totalLSUpdateReportACKed%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
				  		{
				  			printCurrentTime();
							printf("++++ %d LS report ACKed by master:\t",totalLSUpdateReportACKed);
					  		printMNinfo(tempMNInfo);
					  		printf("core(%d)  ++++\n",sched_getcpu());
					  		fflush(stdout);
					  	}
					break;
					default:
						fprintf(stderr, "ERROR! Receive unknown MNinfo type %d\n", 
							tempMNInfo.ReptorDelvorResporAck);
						printf("ERROR! Receive unknown MNinfo type %d\n", 
							tempMNInfo.ReptorDelvorResporAck);
						printMNinfo(tempMNInfo);
						exit(1);
					break;
				}
			}
			else//Unknown event
			{
				fprintf(stderr, "ERROR! Receive unknown epoll event on fd %d\n", 
					recvSock);
				printf("ERROR! Receive unknown epoll event on fd %d\n", 
					recvSock);
				exit(1);
			}

			
			// printLinkTable();
			// printPathTable();
			// printRouteTable();
		}
	}
}

struct ResultCountMessage
{
	SwitchNode mySwitchNode;
	int ifMonitorLocalNICChange;
	int cotestWithMaster;
	int ifDoSwitchCalc;
	double avgTimeFromEpollNotifyToTCPRecvDone;
	double avgTimeForRouteUpdateElapsed;
	double avgTimeFromRouteUpdateDoneToTCPSendDone;
};

void returnResultCountToMaster(RecvFuncPara para)
{
	SwitchNode mySwitchNode;
	mySwitchNode.level=para.mySwitchNode.level;
	mySwitchNode.ID=para.mySwitchNode.ID;
  	int cotestWithMaster=para.cotestWithMaster;
	int testPrimus=para.testPrimus;
	int ifDoSwitchCalc=para.ifDoSwitchCalc;
	int testTimes=para.testTimes;
	int ifMonitorLocalNICChange=para.ifMonitorLocalNICChange;

	ResultCountMessage msg;
	msg.mySwitchNode.level=mySwitchNode.level;
	msg.mySwitchNode.ID=mySwitchNode.ID;
	msg.ifMonitorLocalNICChange=ifMonitorLocalNICChange;
	msg.cotestWithMaster=cotestWithMaster;
	msg.ifDoSwitchCalc=ifDoSwitchCalc;

	double avgTimeFromEpollNotifyToTCPRecvDone=timeElapsedTillLSUpdateReceived/totalLSUpdateReceived-timeElapsedTillLSEpollNotified/totalLSUpdateReceived;
 	double avgTimeForRouteUpdateElapsed=totalTimeForRouteUpdateElapsed/totalLSUpdateReceived;
 	double avgTimeFromRouteUpdateDoneToTCPSendDone=timeElapsedTillLSResponseSentOut/totalLSUpdateReceived-timeElapsedTillLSUpdateReceived/totalLSUpdateReceived-avgTimeForRouteUpdateElapsed;
 
	msg.avgTimeFromEpollNotifyToTCPRecvDone=avgTimeFromEpollNotifyToTCPRecvDone;
	msg.avgTimeForRouteUpdateElapsed=avgTimeForRouteUpdateElapsed;
	msg.avgTimeFromRouteUpdateDoneToTCPSendDone=avgTimeFromRouteUpdateDoneToTCPSendDone;

	 //Send mySwitchNode info
    char sendBuf[sizeof(struct ResultCountMessage)];
  	memcpy(sendBuf,&msg,sizeof(struct ResultCountMessage));
	while(send(MainChannelSock,sendBuf,sizeof(struct ResultCountMessage),0)<=0)
	{
		fprintf(stderr,"[%d.%d] Sock send result count back error.\n"
			,mySwitchNode.level,mySwitchNode.ID);
    	exit(1);
	}
	printf("[%d.%d] return result done to master! \n",mySwitchNode.level,mySwitchNode.ID);
	printf("avgTimeFromEpollNotifyToTCPRecvDone:%.0lf us,\tavgTimeForRouteUpdateElapsed:%.0lf us,\tavgTimeFromRouteUpdateDoneToTCPSendDone:%.0lf us\n"
		,avgTimeFromEpollNotifyToTCPRecvDone,avgTimeForRouteUpdateElapsed,avgTimeFromRouteUpdateDoneToTCPSendDone);
}

int main(int argc, char const *argv[])
{
	if(argc!=9)
	{
		printf("usage: executable ifTestPrimus(0|1) ifDoSwitchCalc(0|1) cotestWithMaster(0|1|2) mySwitchID testTimes MASTER_IP ifMonitorLocalNICChange(0|1)\n");
		printf("\tifTestPrimus(0|1): 0 test normal brute-force calc;\t1 test Primus fast calc\n");
		printf("\tifDoSwitchCalc(0|1): 0 no calc;\t1 calc link/path/route table and change kernel route\n");
		printf("\tcotestWithMaster(0|1|2): 0 test local switch calc;\t1 full test with master with LS report update/receive delivery/switch calc/respond delivery/receive ack;\t2 only test link-state delivery receive/response\n");
		printf("\tmySwitchID: The ID of this switch. Only can be ToR switch, since routes in Agg and Core are fixed\n");
		printf("\ttestTimes: How many link-states generated in this test\n");
		printf("\tMaster IP\n");
		printf("\tifMonitorLocalNICChange(0|1): 0 not monitor local NIC link-state change; 1 monitor local NIC link-state change\n");
		printf("\ttestSwitchClientNum: How many switches (clients) join this test\n");
		exit(1);
	}


  	SwitchNode mySwitchNode;
	// generateRandomSwitch(1,&mySwitchNode);
	mySwitchNode.level=1;

	int testPrimus=atoi(argv[1]);
	int ifDoSwitchCalc=atoi(argv[2]);
	int cotestWithMaster=atoi(argv[3]);
	if (cotestWithMaster!=0 
		&& cotestWithMaster!=1 
		&& cotestWithMaster!=2)
	{
		fprintf(stderr,"cotestWithMaster %d ERROR!\n",cotestWithMaster);
		exit(1);
	}
	mySwitchNode.ID=atoi(argv[4]);
	
	if (mySwitchNode.ID<0 || mySwitchNode.ID>=POD_NUM*TOR_PER_POD)
	{
		fprintf(stderr,"mySwitchID %d ERROR!\n",mySwitchNode.ID);
		exit(1);
	}

	int testTimes=atoi(argv[5]);
	if (testTimes<=0)
	{
		fprintf(stderr,"testTimes %d ERROR!\n",testTimes);
		exit(1);
	}

	char MASTER_IP[128]={0};
	memcpy(MASTER_IP,argv[6],strlen(argv[6]));

	printf("master IP: %s\n",MASTER_IP);

	int ifMonitorLocalNICChange=atoi(argv[7]);
	if (ifMonitorLocalNICChange!=0 
		&& ifMonitorLocalNICChange!=1)
	{
		fprintf(stderr,"ifMonitorLocalNICChange %d ERROR!\n",ifMonitorLocalNICChange);
		exit(1);
	}

  	time_t t;
	srand((unsigned) time(&t));

	//**** Normally, the functions after should all execute ***//
	//**** We add some conditions to disable them for testing convenience ***//
	if(ifMonitorLocalNICChange==1)
		RTNetlinkSockForNICMonitor=createInterfaceNetlinkSocket();

	if(ifDoSwitchCalc!=0)//No switch calc, so no need to modify kernel route
		RTNetlinkSockForModifyRoute=createRouteNetlinkSocket();

	//No need to generate link table if only test LS recv/send with master and do no switch calc 
	//Note that we need to initiate link table for some test scenarios since we randomly generate test links locally
	if(cotestWithMaster!=2 || ifDoSwitchCalc!=0)
		generateLinkTable(mySwitchNode);
	if(ifDoSwitchCalc!=0)//No switch calc, so no need to initiate the following things
	{
		generatePathTable(mySwitchNode);
		generateRouteTable(mySwitchNode);
		generateDefaultRoute(mySwitchNode);
		changeKernelRoute(defaultRoute);
	}
		
	// printLinkTable();
	// printPathTable();
	// printRouteTable();

	printf("\n\n\n\n");
	fflush(stdout);


	if(cotestWithMaster!=0)
		connectToMaster(mySwitchNode,MASTER_IP);

	RecvFuncPara para;
	para.mySwitchNode.level=mySwitchNode.level;
	para.mySwitchNode.ID=mySwitchNode.ID;
	para.cotestWithMaster=cotestWithMaster;
	para.testPrimus=testPrimus;
	para.ifDoSwitchCalc=ifDoSwitchCalc;
	para.testTimes=testTimes;
	para.ifMonitorLocalNICChange=ifMonitorLocalNICChange;

	int epoll_fd=0;

	if(cotestWithMaster!=0 || ifMonitorLocalNICChange!=0)//Need to create epoll fd to monitor LS message or NIC message
		epoll_fd=createAndAddRecvEpollEvents(para);

	if(cotestWithMaster!=0)
		initHelloToMasterSayImReadyToWork(para);
	//Hello to master to notify I'm ready to receive LS updates

	recvLSMsgsAndLocalLinkChangesAndProcessAll(para,epoll_fd);
	//Block in this func and run all the switch processing in it
	//Only return when master notifies or LS update times reach testTimes
	//**** Normally, the functions before should all execute ***//

	if(cotestWithMaster!=0)//Test with master
		printf("\n\n\nMaster notified: \n");

	printf("\tTest finished! Receive %d LS updates in total\n\n\n\n\n\n"
		,totalLSUpdateReceived);

	if(testPrimus)
		printf("========= test Primus fast calc (%d) ========\n",testPrimus);
	else
		printf("========= test Normal brute-force calc (%d) ========\n",testPrimus);

  	if(ifDoSwitchCalc==0)
		printf("========= no switch calc (%d) ========\n",ifDoSwitchCalc);
	else
		printf("========= full switch calc and route change (%d) ========\n",ifDoSwitchCalc);

	if(cotestWithMaster==0)
		printf("========= only test switch calc locally (%d) ========\n",cotestWithMaster);
	else if(cotestWithMaster==1)
		printf("========= full test with master with LS report update/receive delivery/switch calc/respond delivery/receive ack (%d) ========\n",cotestWithMaster);
	else
		printf("========= only test link-state delivery receive/response (%d) ========\n",cotestWithMaster);
	
	if(ifMonitorLocalNICChange==0)
		printf("========= not monitor local link-state change (%d) ========\n",ifMonitorLocalNICChange);
	else
		printf("========= monitor local link-state change (%d) ========\n",ifMonitorLocalNICChange);
	

	if(ifDoSwitchCalc!=0)
	{
		printf("Total route update time elapsed:%.0lf us, average time to completely process local routes for a new link-state:%.0lf us\n", totalTimeForRouteUpdateElapsed, totalTimeForRouteUpdateElapsed/totalLSUpdateReceived);
		printf("Link table update time elapsed:%.0lf us, average time to process a new link-state in link table:%.0lf us\n", linkTableTimeElapsed, linkTableTimeElapsed/totalLSUpdateReceived);
		printf("Path table and route update time elapsed:%.0lf us, average time to process a new link-state in path table:%.0lf us\n", pathTableTimeElapsed, pathTableTimeElapsed/totalLSUpdateReceived);
		
		printf("\n----- Kernel route update perf: kernelRouteChangeTimes(%d) avgKernelRouteChangeTimes(%.5f) -----\n"
			,kernelRouteChangeTimes,(double)kernelRouteChangeTimes/totalLSUpdateReceived);
		printf("Kernel route update time elapsed:%.0lf us, average per kernel route update:%.0lf us, average per link-state:%.0lf us\n"
			, kernelRouteChangeTimeElapsed, kernelRouteChangeTimeElapsed/kernelRouteChangeTimes, kernelRouteChangeTimeElapsed/totalLSUpdateReceived);
	}

	printf("\n\n\n----- LS transmission time perf: totalLSUpdateReceived(%d) totalLSUpdateReported(%d) totalLSUpdateReportACKed(%d) -----\n"
		,totalLSUpdateReceived,totalLSUpdateReported,totalLSUpdateReportACKed);
	printf("Total time elapsed:%.0lf us, average time to completely process a new link-state:%.0lf us\n", totalTimeElapsed, totalTimeElapsed/totalLSUpdateReceived);
	if(cotestWithMaster!=0)
	{		
		if(cotestWithMaster==1)
			printf("Time elapsed till link-state report sent out to master:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillLSReportSentOut, timeElapsedTillLSReportSentOut/totalLSUpdateReceived);
		printf("Time elapsed till link-state update notified by epoll:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillLSEpollNotified, timeElapsedTillLSEpollNotified/totalLSUpdateReceived);
		printf("Time elapsed till link-state update received from master:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillLSUpdateReceived, timeElapsedTillLSUpdateReceived/totalLSUpdateReceived);
		printf("Time elapsed till link-state response sent out to master:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillLSResponseSentOut, timeElapsedTillLSResponseSentOut/totalLSUpdateReceived);	
		if(cotestWithMaster==1)	
			printf("Time elapsed till link-state ACK received from master:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillLSACKReceived, timeElapsedTillLSACKReceived/totalLSUpdateReceived);
		if (cotestWithMaster==1)
			printf("Time elapsed in TCP send for link-state response delivery:%.0lf us, average per link-state response per master:%.2lf us\n", totalSwitchTimeFromRouteUpdateDoneToTCPSendDone, totalSwitchTimeFromRouteUpdateDoneToTCPSendDone/totalLSUpdateReceived);

		printf("\n\n\n----- Return result count back to master -----\n");
		returnResultCountToMaster(para);
		usleep(5000000);
		shutdown(MainChannelSock,SHUT_RDWR);
		close(MainChannelSock);
	}

	/* start */
	if (mySwitchNode.ID == 0)
	{
		ofstream write1( "/home/guolab/LFS/NSDI/switch_result_LS--RS_and_kernel.txt", ios::app);
		write1 << "******************************number of switch: " << argv[8] << "******************************" << std::endl;
		write1 << "switch recv LS--send reponse(average): " << timeElapsedTillLSResponseSentOut/totalLSUpdateReceived -  timeElapsedTillLSUpdateReceived/totalLSUpdateReceived << std::endl;
		write1 << "switch change kernel per LS(average): " << kernelRouteChangeTimeElapsed/totalLSUpdateReceived << std::endl;
		write1.close ();
	}

	char file_path_recv[200];
	strcat (file_path_recv, "/home/guolab/LFS/NSDI/TCP-recv/switch_result_TCP_recv_elapsed_time_num-");
	strcat (file_path_recv, argv[8]);
	strcat (file_path_recv, "/switch-");
	strcat (file_path_recv, argv[4]);
	strcat (file_path_recv, ".txt");

	char file_path_send[200];
	strcat (file_path_send, "/home/guolab/LFS/NSDI/TCP-send/switch_result_TCP_send_elapsed_time_num-");
	strcat (file_path_send, argv[8]);
	strcat (file_path_send, "/switch-");
	strcat (file_path_send, argv[4]);
	strcat (file_path_send, ".txt");

	ofstream write2( file_path_recv, ios::app);
	write2 << (timeElapsedTillLSUpdateReceived/totalLSUpdateReceived - timeElapsedTillLSEpollNotified/totalLSUpdateReceived) << std::endl;
	write2.close ();

	ofstream write3( file_path_send, ios::app);
	write3 << (totalSwitchTimeFromRouteUpdateDoneToTCPSendDone/totalLSUpdateReceived) << std::endl;
	write3.close ();
	/* end */

	return 0;
}