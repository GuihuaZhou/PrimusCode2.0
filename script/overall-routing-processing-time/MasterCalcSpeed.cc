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
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <sched.h>
#include <fstream>
#include <iostream>
#include <sys/epoll.h>
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

#define MASTER_LISTEN_PORT 8801
// #define MASTER_IP "127.0.0.1"

#define TCP_BUF_SIZE (2048*1024)
//B

#define CONNECT_INTERVAL 5000000
//us

#define MAX_TCP_PENDING_ACCEPT_CONNS 10240

#define MAX_OUTBOUNDING_EVENTS 1000
//最大的飘在路上的未完成的事件数量

#define MAX_EPOLL_EVENTS_PER_RECV 100

static unsigned int totalLSReportReceived=0;

static double totalTimeElapsed=0;
static double timeElapsedTillLinkStateAllSentOut=0;
static double timeElapsedTillAllResponseRecved=0;
static double timeElapsedTillACKSentOut=0;

static unsigned int CounterForLastSendSwitch[TOR_PER_POD*POD_NUM]={0};
static unsigned int CounterForLastRecvSwitch[TOR_PER_POD*POD_NUM]={0};

static double totalTimeElapsedForSlowestThreadTCPSend=0;
static double totalTimeElapsedForSlowestThreadTCPRecv=0;
static double totalTimeElapsedForSlowestThreadStartToSend=0;
static double totalTimeElapsedForSlowestThreadStartToRecv=0;

static double totalTimeElapsedForTCPSend=0;
static double totalTimeElapsedForTCPRecv=0;

	//tiem perf info at switch side.
static double totalSlowestSwitchTimeFromEpollNotifyToTCPRecvDone=0;
static double totalSlowestSwitchTimeForRouteUpdateElapsed=0;
static double totalSlowestSwitchTimeFromRouteUpdateDoneToTCPSendDone=0;


static pthread_mutex_t mutexPrint=PTHREAD_MUTEX_INITIALIZER;//mutex for all debug print 

static pthread_cond_t condSendThread=PTHREAD_COND_INITIALIZER;                                                             
//Used for wakeup send threads when new LS report comes

static struct timeval lastPrintGlobalTime={0,0};

static bool AllSendRecvThreadExit=false;

struct SwitchNode
{
	int level;
	int ID;
};

static SwitchNode lastSlowestResponseSwitch;

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

//Global
static LinkTableEntry *linkTable;

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
			linkTable[i].affectArea.firstEntry=dstAgg.ID*CORE_PER_AGG;
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

bool checkIfANewLS(Link testLink, unsigned int eventID)
{
	int entryID;
	entryID=getLinkTableEntryID(testLink);
	if(entryID>=TOTAL_LINK_NUM|| entryID<0)//Our smart index algorithm
	{
		fprintf(stderr, "No LinkTableEntry found, ERROR!\n");
		exit(1);
	}

	if(eventID>linkTable[entryID].eventID)
		return true;

	return false;
}

int listenTCP()
{
	struct sockaddr_in serverAddr;// 监听本地地址
	memset(&serverAddr,0,sizeof(serverAddr)); //数据初始化--清零
	serverAddr.sin_family=AF_INET; //设置为IP通信
	serverAddr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
	serverAddr.sin_port=htons(MASTER_LISTEN_PORT);

	int serverSock;
	/*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
	if((serverSock=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		fprintf(stderr,"TCPRoute Create Socket Failed.\n");
    	exit(1);
	}

	int value=1;
	if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
	{
		fprintf(stderr,"Set SO_REUSEPORT error.\n");
      	exit(1);
	}

	if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
	{
		fprintf(stderr,"Set SO_REUSEADDR error.\n");
      	exit(1);
	}

	if (setsockopt(serverSock, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
    {
		fprintf(stderr,"Set TCP_NODELAY error.\n");
      	exit(1);
	}

	/*将套接字绑定到服务器的网络地址上*/
	if(bind(serverSock,(struct sockaddr *)&serverAddr,sizeof(struct sockaddr))<0)
	{
		fprintf(stderr,"Bind Socket Failed.\n");
      	exit(1);
	}

	if(listen(serverSock,MAX_TCP_PENDING_ACCEPT_CONNS)<0)
	{
		fprintf(stderr,"Listen Socket Failed.\n");
      	exit(1);
	}

	return serverSock;
}

struct SwitchSock
{
	SwitchNode switchNode;
	int cotestWithMaster;
	int sock;
};

struct SwitchSockTable 
{
	SwitchSock switchSocks[TOR_PER_POD*POD_NUM];
	int connectedSwitchNum;
};

//Global
static SwitchSockTable switchSockTable;
//switchSockTable stores the mapping of each ToR switch and its TCP sock

void initiateSwitchSockTable()
{
	switchSockTable.connectedSwitchNum=0;
	for(int i=0;i<TOR_PER_POD*POD_NUM;i++)
	{
		switchSockTable.switchSocks[i].switchNode.ID=-1;
		switchSockTable.switchSocks[i].switchNode.level=-1;
		switchSockTable.switchSocks[i].sock=0;
	}
}


void printSwitchSockTable()
{
	printf("------- switchSockTable: connectedSwitchNum %d -------\n",switchSockTable.connectedSwitchNum);
	for(int i=0;i<TOR_PER_POD*POD_NUM;i++)
	{
		if(switchSockTable.switchSocks[i].switchNode.ID!=-1)
		{
			struct sockaddr_in sin;
		    socklen_t len = sizeof(sin);
			if (getpeername(switchSockTable.switchSocks[i].sock, (struct sockaddr *)&sin, &len) == -1)
			    perror("getsockname");
		
			printf("switchSockTable[%5d]: switch(%d.%d),\tcotestWithMaster(%d),\tsock(%d %s:%d)\n"
				,i
				,switchSockTable.switchSocks[i].switchNode.level
				,switchSockTable.switchSocks[i].switchNode.ID
				,switchSockTable.switchSocks[i].cotestWithMaster
				,switchSockTable.switchSocks[i].sock
				,inet_ntoa(sin.sin_addr),ntohs(sin.sin_port)
				);
		}
	}
}

struct HelloMsg
{
	SwitchNode mySwitchNode;
	int cotestWithMaster;
};

//If succeed, return the newly accepted switchSockTable entry ID
int acceptConnectionFromSwitch(int serverSock)
{
	struct sockaddr_in clientAddr;
    unsigned sin_size=sizeof(struct sockaddr_in);

	int clientSock=0;
	if ((clientSock=accept(serverSock,(struct sockaddr *)&(clientAddr),&sin_size))<0)
    {
    	// printSwitchSockTable();
      	fprintf(stderr,"Accept Socket Failed from %s:%d.\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
    	exit(1);
    }
    else
    {
    	if(IF_PRINT_DEBUG_INFO)
    		printf("Accept Socket from %s:%d.\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
    }

    int totalRecvSize=0;
	char recvBuf[sizeof(struct HelloMsg)];
	memset(recvBuf,0,sizeof(recvBuf));
	while(totalRecvSize<sizeof(struct HelloMsg))
	{
		int recvSize=0;
	    if((recvSize=recv(clientSock,recvBuf+totalRecvSize,sizeof(struct HelloMsg)-totalRecvSize,0))<=0)
	    {
	      fprintf(stderr,"Master Sock recv error. totalRecvSize: %d\n"
	      	,totalRecvSize);
	      exit(1);
	    }
	    totalRecvSize=totalRecvSize+recvSize;
	}

	HelloMsg helloMsg;
	memcpy(&helloMsg,recvBuf,sizeof(struct HelloMsg));

    SwitchSock switchSock;
    memcpy(&(switchSock.switchNode),&(helloMsg.mySwitchNode),sizeof(struct SwitchNode));
    switchSock.cotestWithMaster=helloMsg.cotestWithMaster;

    //Check invalid switchNode.ID
    if(switchSock.switchNode.ID<0||switchSock.switchNode.ID>=POD_NUM*TOR_PER_POD)
    {
    	fprintf(stderr, "ERROR: Accept invalid sock from [%d.%d]!\n"
	      	,switchSock.switchNode.level,switchSock.switchNode.ID);
    	exit(1);
    }

	switchSock.sock=clientSock;

	memcpy(&(switchSockTable.switchSocks[switchSock.switchNode.ID]),&switchSock,sizeof(struct SwitchSock));
	switchSockTable.connectedSwitchNum++;

	if(IF_PRINT_DEBUG_INFO)
	{
		printf("Established sock from [%d.%d](%s:%d) cotestWithMaster(%d)! connectedSwitchNum: %d\n"
		      	,switchSock.switchNode.level,switchSock.switchNode.ID
		      	,inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port)
		      	,switchSock.cotestWithMaster
		      	,switchSockTable.connectedSwitchNum);
		// printSwitchSockTable();
		fflush(stdout);
	}
	return switchSock.switchNode.ID;
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

int sendToAllAffectedSwitchesSequential(MNinfo linkStateInfo, int startSwitchID, int endSwitchID)
{
	if(startSwitchID<0 || endSwitchID>=TOR_PER_POD*POD_NUM)
	{
		fprintf(stderr,"ERROR! sendToAllAffectedSwitchesSequential startSwitchID(%d) endSwitchID(%d) out of range.\n"
			,startSwitchID
			,endSwitchID);
		exit(1);
	}

	char sendBuf[sizeof(struct MNinfo)];
	memcpy(sendBuf,&linkStateInfo,sizeof(struct MNinfo));

	int numOfSucessfulSend=0;
	for(int i=startSwitchID;i<=endSwitchID;i++)
	{
		if(switchSockTable.switchSocks[i].switchNode.ID==-1)
			continue;
		if(send(switchSockTable.switchSocks[i].sock,sendBuf,sizeof(struct MNinfo),0)<=0)
		{
			fprintf(stderr,"ERROR! sendToAllAffectedSwitchesSequential send link-state to switch(%d.%d) error.\n"
				,switchSockTable.switchSocks[i].switchNode.level
				,switchSockTable.switchSocks[i].switchNode.ID);
			exit(1);
	    	// usleep(CONNECT_INTERVAL);
		}
		numOfSucessfulSend++;

		if(IF_PRINT_DEBUG_INFO)
		{
			pthread_mutex_lock(&mutexPrint);
			printf("Send message to switch(%d.%d):\t"
					,switchSockTable.switchSocks[i].switchNode.level
					,switchSockTable.switchSocks[i].switchNode.ID);
			printMNinfo(linkStateInfo);
			printf("\n");
			pthread_mutex_unlock(&mutexPrint);
		}
	}
	return numOfSucessfulSend;
}

int recvOneMessageFromOneSwitchSock(MNinfo *linkInfo, int sock)
{
	if(AllSendRecvThreadExit)
	{
		pthread_mutex_lock(&mutexPrint);
		fprintf(stderr,"Exit recv thread(%ld) on core(%d)\n"
				,pthread_self()
				,sched_getcpu());
		fflush(stdout);
		pthread_mutex_unlock(&mutexPrint);
		pthread_exit(NULL);
	}

	int totalRecvSize=0;
	
	char recvBuf[sizeof(struct MNinfo)];
	memset(recvBuf,0,sizeof(recvBuf));
	totalRecvSize=0;
	while(totalRecvSize<sizeof(struct MNinfo))
	{
		int recvSize=recv(sock,recvBuf+totalRecvSize,sizeof(struct MNinfo)-totalRecvSize,0);
	    if(recvSize<0)
	    {
	      fprintf(stderr,"ERROR: thread(%ld) recv response error from sock(%d).\n"
			,pthread_self()
			,sock);
	      exit(1);
	    }
	    else if(recvSize==0)
	    {
	    	fprintf(stderr,"recvOneMessageFromOneSwitchSock: Recv sock(%d) closed... Stop the recv thread.\n",sock);
			shutdown(sock,SHUT_RDWR);
			close(sock);
			pthread_exit(NULL);
	    }
	    totalRecvSize=totalRecvSize+recvSize;
	}
    memcpy(linkInfo,recvBuf,sizeof(struct MNinfo));
    if(linkInfo->ReptorDelvorResporAck!=0 && linkInfo->ReptorDelvorResporAck!=2)
    //0 means LS report, 2 means LS response. Master should only receive these two types of LS
    {
    	fprintf(stderr,"ERROR: thread(%ld) recv response error from sock(%d).\n"
		,pthread_self()
		,sock);
		printf("ERROR: recv response error from thread(%ld).\n"
		,pthread_self());
		pthread_mutex_lock(&mutexPrint);
		printf("Received:\t");
		printMNinfo(*linkInfo);
		fflush(stdout);
		pthread_mutex_unlock(&mutexPrint);
		exit(1);
    }

    if(IF_PRINT_DEBUG_INFO)
	{
		pthread_mutex_lock(&mutexPrint);
		printf("Receive message from sock(%d):\t"
				,sock);
		printMNinfo(*linkInfo);
		printf("\n");
		pthread_mutex_unlock(&mutexPrint);
	}

	return 1;
}


int getNumOfValidSwitchClients(int startSwitchID, int endSwitchID)
{
	int num=0;
	for(int i=startSwitchID;i<=endSwitchID;i++)
	{
		if(switchSockTable.switchSocks[i].switchNode.ID==-1)
			continue;
		num++;
	}
	return num;
}

struct MessageEvent
{
	MNinfo message;
	int numOfSwitchesShouldNotify;
	int numOfSwitchesSent;//Only looked and modified by send thread
	int numOfSwitchesResponsed;//Only looked and modified by recv thread
	//We only record the count instead of which switch, since we always notify all ToR switches
	struct timeval tvStart;//Time when the message is generated (or received from reporting switch) 
	struct timeval tvAllSent;//Time when all send threads send out this event
	struct timeval tvFirstSent;//Time when the first send thread sends out this event
};

struct MessageEventQueue
{
	MessageEvent eventQueue[MAX_OUTBOUNDING_EVENTS]; //Message pending to process
	int head;//First element is head
	int tail;//Last element is tail-1
	int len;
	unsigned int counterIn;//How many events have ever been pushed in
	unsigned int counterOut;//How many events have ever been popped out
	unsigned int nextEventID;//The ID of next event enqueued (to keep the events in sequence based on their ID)
	//messageID start from 1
};

//Global 
static MessageEventQueue messageEventQueue;
//Message pending to process (link-states to deliver and response)
//Main (or recv) thread produce messages, recv thread consume (when the link-states have been responded by all switches)
static pthread_mutex_t MsgQueueMutex=PTHREAD_MUTEX_INITIALIZER;//mutex for this queue
static pthread_mutex_t MsgQueueEventMutex[MAX_OUTBOUNDING_EVENTS];
//mutex for each event position in this queue
//Note mutex have to be global variables, so we don't attach it to each event struct.

void printMessageEvent(MessageEvent msg)
{
	printMNinfo(msg.message);
	printf("numOfSwitchesShouldNotify(%d), numOfSwitchesSent(%d), numOfSwitchesResponsed(%d)\n"
		,msg.numOfSwitchesShouldNotify
		,msg.numOfSwitchesSent
		,msg.numOfSwitchesResponsed);
}

void printMessageEventQueue()
{
	printf("\n---- MessageEventQueue [head(%d), tail(%d), len(%d), counterIn(%d), counterOut(%d), nextEventID(%d)] ----\n"
		,messageEventQueue.head
		,messageEventQueue.tail
		,messageEventQueue.len
		,messageEventQueue.counterIn
		,messageEventQueue.counterOut
		,messageEventQueue.nextEventID);
	int it=messageEventQueue.head;
	for(int i=0;i<messageEventQueue.len;i++)
	{
		printf("[%d]: ",it);
		printMessageEvent(messageEventQueue.eventQueue[it]);
		it=(it+1)%MAX_OUTBOUNDING_EVENTS;
	}
	printf("\n");
}

void initMessageEventQueues()
{
	messageEventQueue.head=0;//First element is head
	messageEventQueue.tail=0;//Last element is tail-1
	messageEventQueue.len=0;
	messageEventQueue.counterIn=0;//How many events have ever been pushed in
	messageEventQueue.counterOut=0;//How many events have ever been popped out
	messageEventQueue.nextEventID=1;

	for (int i = 0; i < MAX_OUTBOUNDING_EVENTS; i++)
	{
        pthread_mutex_init(&MsgQueueEventMutex[i], NULL);
	}

	printf("===== Message event queues initiated =====\n");
	printMessageEventQueue();
	printf("\n\n\n\n");
}

int enqueueMessageIntoEventQueue(MessageEvent msg)
//Return queue len if enqueue succeed, else return -1
{	
	int queueLen=0;
	pthread_mutex_lock(&(MsgQueueMutex));
	if(messageEventQueue.len==MAX_OUTBOUNDING_EVENTS)
	{
		pthread_mutex_unlock(&(MsgQueueMutex));
		return -1;
	}

//Always keep the events in sequence, thus to facilitate the consumer threads to process
	if(messageEventQueue.nextEventID!=msg.message.eventIDforSendResponse)
	{
		pthread_mutex_unlock(&(MsgQueueMutex));
		return -1;
	}

	memcpy(&(messageEventQueue.eventQueue[messageEventQueue.tail]),&msg,sizeof(MessageEvent));

	//Tag the link-state start time
	gettimeofday(&(messageEventQueue.eventQueue[messageEventQueue.tail].tvStart),NULL);

	messageEventQueue.tail=(messageEventQueue.tail+1)%MAX_OUTBOUNDING_EVENTS;
	messageEventQueue.len++;
	messageEventQueue.counterIn++;
	messageEventQueue.nextEventID++;

	queueLen=messageEventQueue.len;
	pthread_mutex_unlock(&(MsgQueueMutex));
	return queueLen;
}

bool dequeueMessageFromEventQueue()
//Return true if dequeue succeed, else return false
{	
	struct timeval now;
	pthread_mutex_lock(&(MsgQueueMutex));
	//lock the queue mutex when checking the queue status, in case of the recv thread is writing to this queue
	
	if(messageEventQueue.len<=0){
		pthread_mutex_unlock(&(MsgQueueMutex));
		return false;
	}

//Perf time
	gettimeofday(&now,NULL);
	totalTimeElapsed=totalTimeElapsed+time_diff(messageEventQueue.eventQueue[messageEventQueue.head].tvStart,now);

	messageEventQueue.len--;
	messageEventQueue.head=(messageEventQueue.head+1)%MAX_OUTBOUNDING_EVENTS;
	messageEventQueue.counterOut++;
	pthread_mutex_unlock(&(MsgQueueMutex));


	return true;
}

void enqueueMessageIntoEventQueueUntilSucceed(MessageEvent msg, int MutexCndWaitOrBusySleepWaitOrBusyYieldWait)
{
	int queueLen=0;
	while(1)
	{
		queueLen=enqueueMessageIntoEventQueue(msg);
		//lock inside

		if(queueLen<=0)
			sched_yield();//send queue is full, wait for the next round
		else
			break;
	}

	if(MutexCndWaitOrBusySleepWaitOrBusyYieldWait==0)//Use cond wait
	{
		if(queueLen==1)//New msg comes, wake all blocked send threads
		{
			if (pthread_cond_broadcast(&condSendThread) != 0) {                                  
			    perror("pthread_cond_timedwait() error");                                   
			    exit(1);                                                                    
			}
			if(IF_PRINT_DEBUG_INFO)
			{
				printf("New LS comes: Wakeup all sleeping send threads\n");
				fflush(stdout);
			}         
		}
	}

	if(IF_PRINT_DEBUG_INFO)
	{
		pthread_mutex_lock(&mutexPrint);
		printf("---- After enqueueMessageIntoEventQueue: Thread(%ld) [main] ----\n"
		,pthread_self());
		fflush(stdout);
		printMessageEventQueue();
		fflush(stdout);
		pthread_mutex_unlock(&mutexPrint);
	}
}

//Specify which switches this send thread sends messages to
//[startSwitchID, ..., endSwitchID]
struct SendMessageThreadParam
{
	int startSwitchID;
	int endSwitchID;
	int MutexCndWaitOrBusySleepWaitOrBusyYieldWait;
};

void *sendMessageThread(void *param)
{
	SendMessageThreadParam *sendMessageThreadParam=(struct SendMessageThreadParam *)param;
	int startSwitchID=sendMessageThreadParam->startSwitchID;
	int endSwitchID=sendMessageThreadParam->endSwitchID;
	int MutexCndWaitOrBusySleepWaitOrBusyYieldWait=sendMessageThreadParam->MutexCndWaitOrBusySleepWaitOrBusyYieldWait;
	free(sendMessageThreadParam);

	pthread_mutex_lock(&mutexPrint);
	printf("Start send thread(%ld) on core(%d), sending to [%d.%d, ..., %d.%d]. MutexCndWaitOrBusySleepWaitOrBusyYieldWait(%d)\n"
			,pthread_self()
			,sched_getcpu()
			,1,startSwitchID
			,1,endSwitchID
			,MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
	fflush(stdout);
	pthread_mutex_unlock(&mutexPrint);

	int thisSendQueueHead=0;//Which send event this thread is processing 
	//All send thread start from 0, because the producer starts to write in 0
	int sendQueueLen=0;
	int sendQueueHead=0;
	int sendQueueTail=0;
	unsigned int sendQueueCounterIn=0;
	unsigned int numOfEventsSentByThisThread=0;
	int thisEventNumOfSwitchesSentByThisThread=0;
	int thisEventNumOfSwitchesSentByThisThreadBefore=0;
	int thisEventNumOfSwitchesSentByThisThreadAfter=0;
	int thisEventNumOfSwitchesShouldNotify=0;

	struct timeval beforeSend,afterSend;

	while(1)
	{
		if(AllSendRecvThreadExit)
		{
			pthread_mutex_lock(&mutexPrint);
			fprintf(stderr,"Exit send thread(%ld) on core(%d)\n"
					,pthread_self()
					,sched_getcpu());
			// fflush(stdout);
			pthread_mutex_unlock(&mutexPrint);
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&(MsgQueueMutex));
		//lock the queue mutex when checking the queue status, in case of the recv thread is writing to this queue
		sendQueueLen=messageEventQueue.len;
		sendQueueHead=messageEventQueue.head;
		sendQueueTail=messageEventQueue.tail;
		sendQueueCounterIn=messageEventQueue.counterIn;
		pthread_mutex_unlock(&(MsgQueueMutex));
		//Unlock the queue before sending a event
		
		if(sendQueueLen<0||sendQueueLen>MAX_OUTBOUNDING_EVENTS)
		{
			fprintf(stderr, "ERROR! sendQueueLen %d\n", sendQueueLen);
			exit(1);
		}

		if(sendQueueLen==0 || numOfEventsSentByThisThread==sendQueueCounterIn)
		//Send queue is empty or all send events have been sent, wait
		{
			//Change to wait_cond later
			if(IF_PRINT_DEBUG_INFO && MutexCndWaitOrBusySleepWaitOrBusyYieldWait==0)
			{
				pthread_mutex_lock(&mutexPrint);
				printf("---- No send, wait: Thread(%ld) [%d.%d, ..., %d.%d] thisSendQueueHead(%d) numOfEventsSentByThisThread(%d) ----\n"
				,pthread_self()
				,1,startSwitchID
				,1,endSwitchID
				,thisSendQueueHead
				,numOfEventsSentByThisThread);
				fflush(stdout);
				pthread_mutex_unlock(&mutexPrint);
			}

			if(MutexCndWaitOrBusySleepWaitOrBusyYieldWait==0)//Cond wait
			{
				pthread_mutex_lock(&(MsgQueueMutex));
				pthread_cond_wait(&condSendThread, &MsgQueueMutex);
				pthread_mutex_unlock(&(MsgQueueMutex));
			}
			else if(MutexCndWaitOrBusySleepWaitOrBusyYieldWait==1)//Busy waiting using sleep
			{
				usleep(10);
			}
			else if(MutexCndWaitOrBusySleepWaitOrBusyYieldWait==2)//Busy waiting using sched_yield to giveup CPU
			{
				sched_yield();
			}
			else
			{
				fprintf(stderr, "ERROR! Unknown MutexCndWaitOrBusySleepWaitOrBusyYieldWait %d\n", 
					MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
				exit(1);
			}
			
			if(IF_PRINT_DEBUG_INFO && MutexCndWaitOrBusySleepWaitOrBusyYieldWait==0)
			{
				pthread_mutex_lock(&mutexPrint);
				printf("---- Wakeup: Thread(%ld) [%d.%d, ..., %d.%d] thisSendQueueHead(%d) numOfEventsSentByThisThread(%d) ----\n"
				,pthread_self()
				,1,startSwitchID
				,1,endSwitchID
				,thisSendQueueHead
				,numOfEventsSentByThisThread);
				fflush(stdout);
				pthread_mutex_unlock(&mutexPrint);
			}

			continue;//Return to the loop begin to fetch the latest event queue status
		}

		if(IF_PRINT_DEBUG_INFO)
		{
			pthread_mutex_lock(&mutexPrint);
			printf("---- Before send: Thread(%ld) [%d.%d, ..., %d.%d] thisSendQueueHead(%d) numOfEventsSentByThisThread(%d) ----\n"
				,pthread_self()
				,1,startSwitchID
				,1,endSwitchID
				,thisSendQueueHead
				,numOfEventsSentByThisThread);
			fflush(stdout);
			printMessageEventQueue();
			fflush(stdout);
			pthread_mutex_unlock(&mutexPrint);
		}

		for(int i=0;i<sendQueueLen;i++)
		{
			if(numOfEventsSentByThisThread==sendQueueCounterIn)//I have sent all events ever pushed in
				break;

			gettimeofday(&beforeSend,NULL);
			thisEventNumOfSwitchesSentByThisThread=sendToAllAffectedSwitchesSequential(messageEventQueue.eventQueue[thisSendQueueHead].message,startSwitchID,endSwitchID);
			//No need to lock anything, since this event will not be cleared until I complete send and change its numOfSwitchesSent
			gettimeofday(&afterSend,NULL);

			totalTimeElapsedForTCPSend=totalTimeElapsedForTCPSend+time_diff(beforeSend,afterSend);

			pthread_mutex_lock(&(MsgQueueEventMutex[thisSendQueueHead]));
			thisEventNumOfSwitchesSentByThisThreadBefore=messageEventQueue.eventQueue[thisSendQueueHead].numOfSwitchesSent;
			messageEventQueue.eventQueue[thisSendQueueHead].numOfSwitchesSent=messageEventQueue.eventQueue[thisSendQueueHead].numOfSwitchesSent+thisEventNumOfSwitchesSentByThisThread;
			thisEventNumOfSwitchesSentByThisThreadAfter=messageEventQueue.eventQueue[thisSendQueueHead].numOfSwitchesSent;
			thisEventNumOfSwitchesShouldNotify=messageEventQueue.eventQueue[thisSendQueueHead].numOfSwitchesShouldNotify;
			
			if(thisEventNumOfSwitchesSentByThisThreadBefore==0)
			//This message has not been sent before
			{	
				//perf time
				memcpy(&(messageEventQueue.eventQueue[thisSendQueueHead].tvFirstSent),&afterSend,sizeof(timeval));
			}

			if(thisEventNumOfSwitchesSentByThisThreadAfter==thisEventNumOfSwitchesShouldNotify)
			//This message has been sent to all switches
			{
				CounterForLastSendSwitch[endSwitchID]++;
				//perf time
				timeElapsedTillLinkStateAllSentOut=timeElapsedTillLinkStateAllSentOut+time_diff(messageEventQueue.eventQueue[thisSendQueueHead].tvStart,afterSend);
				totalTimeElapsedForSlowestThreadTCPSend=totalTimeElapsedForSlowestThreadTCPSend+time_diff(beforeSend,afterSend);
				totalTimeElapsedForSlowestThreadStartToSend=totalTimeElapsedForSlowestThreadStartToSend+time_diff(messageEventQueue.eventQueue[thisSendQueueHead].tvStart,beforeSend);
				memcpy(&(messageEventQueue.eventQueue[thisSendQueueHead].tvAllSent),&afterSend,sizeof(timeval));

				if(totalLSReportReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
		  		{
		  			pthread_mutex_lock(&mutexPrint);
		  			printCurrentTime();
		  			printf("---- %d LS send to all %d switches:\t",totalLSReportReceived,thisEventNumOfSwitchesShouldNotify);
			  		printMNinfo(messageEventQueue.eventQueue[thisSendQueueHead].message);
			  		printf("Thread core(%2d) [%d.%d, ..., %d.%d] ----\n"
			  			,sched_getcpu()
						,1,startSwitchID
						,1,endSwitchID);
			  		fflush(stdout);
			  		pthread_mutex_unlock(&mutexPrint);
			  	}
			}

			pthread_mutex_unlock(&(MsgQueueEventMutex[thisSendQueueHead]));

			thisSendQueueHead=(thisSendQueueHead+1)%MAX_OUTBOUNDING_EVENTS;
			numOfEventsSentByThisThread++;
		}

		if(IF_PRINT_DEBUG_INFO)
		{
			pthread_mutex_lock(&mutexPrint);
			printf("---- After send: Thread(%ld) [%d.%d, ..., %d.%d] thisSendQueueHead(%d) numOfEventsSentByThisThread(%d) ----\n"
				,pthread_self()
				,1,startSwitchID
				,1,endSwitchID
				,thisSendQueueHead
				,numOfEventsSentByThisThread);
			fflush(stdout);
			printMessageEventQueue();
			fflush(stdout);
			pthread_mutex_unlock(&mutexPrint);
		}
	}
}

int createAndAddRecvEpollEvents(int startSwitchID, int endSwitchID)
{
	int epoll_fd;
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
    {
      perror ("epoll_create");
      exit(1);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    for(int i=startSwitchID;i<=endSwitchID;i++)
	{
		if(switchSockTable.switchSocks[i].switchNode.ID==-1)
			continue;
		event.data.fd=switchSockTable.switchSocks[i].sock;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, switchSockTable.switchSocks[i].sock, &event) == -1)
		{
		  perror ("epoll_ctl");
		  exit(1);
		}
	}
	
	return epoll_fd;
}

//Specify which switches this recv thread receives messages from
//[startSwitchID, ..., endSwitchID]
struct RecvMessageThreadParam
{
	int startSwitchID;
	int endSwitchID;
	int ifTestEpoll;
	int MutexCndWaitOrBusySleepWaitOrBusyYieldWait;
	int testSwitchClientNum;
	int localGenerateLSorGetLSFromSwitch;
};

//Thread to receive responses from switches (and receive link-state updates)
void *recvMessageThread(void *param)
{
	RecvMessageThreadParam *recvMessageThreadParam=(struct RecvMessageThreadParam *)param;
	int startSwitchID=recvMessageThreadParam->startSwitchID;
	int endSwitchID=recvMessageThreadParam->endSwitchID;
	int ifTestEpoll=recvMessageThreadParam->ifTestEpoll;
	int MutexCndWaitOrBusySleepWaitOrBusyYieldWait=recvMessageThreadParam->MutexCndWaitOrBusySleepWaitOrBusyYieldWait;
	int testSwitchClientNum=recvMessageThreadParam->testSwitchClientNum;
	int localGenerateLSorGetLSFromSwitch=recvMessageThreadParam->localGenerateLSorGetLSFromSwitch;
	free(recvMessageThreadParam);

	pthread_mutex_lock(&mutexPrint);
	printf("Start recv thread(%ld) on core(%d), receiving from [%d.%d, ..., %d.%d]\n"
			,pthread_self()
			,sched_getcpu()
			,1,startSwitchID
			,1,endSwitchID);
	fflush(stdout);
	pthread_mutex_unlock(&mutexPrint);

	int thisResponseQueueHead=0;//The first response event this thread is processing 
	//All recv thread start from 0, because the producer starts to write in 0
	int responseQueueLen=0;
	int responseQueueHead=0;
	int responseQueueTail=0;
	unsigned int responseQueueCounterIn=0;
	unsigned int numOfEventsFinishedByThisThread=0;
	int thisEventNumOfSwitchesResponsedByThisThreadAfter=0;
	int thisEventNumOfSwitchesShouldNotify=0;
	int numOfValidSwitchesInThisThread=getNumOfValidSwitchClients(startSwitchID,endSwitchID);
	
	unsigned int thisResponseQueueHeadEventID=1;
	int numOfSwitchesResponsedForEvents[MAX_OUTBOUNDING_EVENTS]={0};
	//This is a cache per thread for response receiving status
	//Each thread will first collect all the responses from its switches, then lock the shared event queue and update the event element
	//Without this cache, each thread has to lock the event queue and update event for each responses received from each switch
	int offset=0;

	int epoll_fd;
	struct epoll_event events[MAX_EPOLL_EVENTS_PER_RECV];
	int received_socks_num=0;
	if(ifTestEpoll)
		epoll_fd=createAndAddRecvEpollEvents(startSwitchID,endSwitchID);

	struct timeval beforeRecv,afterRecv,afterSendACK;
	MNinfo tempMNInfo;
	int recvSock;
	while(1)
	{
		if(ifTestEpoll)
			received_socks_num=epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS_PER_RECV, -1);
		else
			received_socks_num=numOfValidSwitchesInThisThread;

		for(int i=0;i<received_socks_num;i++)
		{
			if(ifTestEpoll)
			{
				recvSock=events[i].data.fd;//The socks having coming data
				if ((events[i].events & EPOLLERR) ||
	              (events[i].events & EPOLLHUP) ||
	              (!(events[i].events & EPOLLIN)))
				{
					fprintf(stderr,"Epoll event: Recv sock(%d) closed... Stop the recv thread.\n",recvSock);
					shutdown(recvSock,SHUT_RDWR);
					close(recvSock);
					pthread_exit(NULL);
				}
			}
			else
			{
				if(switchSockTable.switchSocks[i+startSwitchID].switchNode.ID==-1)
					continue;
				recvSock=switchSockTable.switchSocks[i+startSwitchID].sock;
				//Round-robin the valid switch sock within the range, but the data may not come in now
			}
		
			if(IF_PRINT_DEBUG_INFO)
			{
				pthread_mutex_lock(&mutexPrint);
				printf("---- Before recv: Thread(%ld) [%d.%d, ..., %d.%d] thisResponseQueueHead(%d) thisResponseQueueHeadEventID(%d) numOfEventsFinishedByThisThread(%d) ----\n"
					,pthread_self()
					,1,startSwitchID
					,1,endSwitchID
					,thisResponseQueueHead
					,thisResponseQueueHeadEventID
					,numOfEventsFinishedByThisThread);
				fflush(stdout);
				printMessageEventQueue();
				printf("numOfSwitchesResponsedForEvents[]: \n");
				for(int k=0;k<MAX_OUTBOUNDING_EVENTS;k++)
				{
					if(k==thisResponseQueueHead)
						printf("*");
					printf("%d, ",numOfSwitchesResponsedForEvents[k]);
				}
				printf("\n");
				fflush(stdout);
				pthread_mutex_unlock(&mutexPrint);
			}

			//perf time
			gettimeofday(&beforeRecv,NULL);

			recvOneMessageFromOneSwitchSock(&tempMNInfo,recvSock);

			//perf time
			gettimeofday(&afterRecv,NULL);

			switch(tempMNInfo.ReptorDelvorResporAck)
			{
				case 0://LS report
					// if(!checkIfANewLS(tempMNInfo.link,tempMNInfo.eventID))
					// 	break;

					totalLSReportReceived++;
					tempMNInfo.eventIDforSendResponse=totalLSReportReceived;

					if(totalLSReportReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
			  		{
			  			pthread_mutex_lock(&mutexPrint);
			  			printCurrentTime();
			  			printf("++++ %d new LS report:\t",totalLSReportReceived);
				  		printMNinfo(tempMNInfo);
				  		printf("Thread core(%2d) [%d.%d, ..., %d.%d] ----\n"
				  			,sched_getcpu()
							,1,startSwitchID
							,1,endSwitchID);
				  		fflush(stdout);
				  		pthread_mutex_unlock(&mutexPrint);
				  	}

					
					//Send to MessageEventQueue
					MessageEvent msg;
			  		memcpy(&(msg.message),&tempMNInfo,sizeof(MNinfo));
			  		msg.numOfSwitchesShouldNotify=testSwitchClientNum;
			  		msg.numOfSwitchesSent=0;
			  		msg.numOfSwitchesResponsed=0;
			  		msg.message.ReptorDelvorResporAck=1;//Make it a delivery and push into send queue
			  		
			  		enqueueMessageIntoEventQueueUntilSucceed(msg,MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
				break;
				case 1:
				break;
				case 2://LS response
					//Perf time at switch side, for the last time slowest switch
					if(totalLSReportReceived>1)//Jump the first one, since the switch side tag the time perf for last LS
					{
						if(sameNode(tempMNInfo.responseNode,lastSlowestResponseSwitch))
						{
							totalSlowestSwitchTimeFromEpollNotifyToTCPRecvDone=totalSlowestSwitchTimeFromEpollNotifyToTCPRecvDone+tempMNInfo.lastSwitchTimeFromEpollNotifyToTCPRecvDone;
							totalSlowestSwitchTimeForRouteUpdateElapsed=totalSlowestSwitchTimeForRouteUpdateElapsed+tempMNInfo.lastSwitchTimeForRouteUpdateElapsed;
							totalSlowestSwitchTimeFromRouteUpdateDoneToTCPSendDone=totalSlowestSwitchTimeFromRouteUpdateDoneToTCPSendDone+tempMNInfo.lastSwitchTimeFromRouteUpdateDoneToTCPSendDone;
						}
					}

					if(tempMNInfo.eventIDforSendResponse<thisResponseQueueHeadEventID)
					{
						pthread_mutex_lock(&mutexPrint);
						printf("Duplicated LS response! tempMNInfo.eventIDforSendResponse(%d) < thisResponseQueueHeadEventID(%d)\n"
							,tempMNInfo.eventIDforSendResponse
							,thisResponseQueueHeadEventID);
						printMNinfo(tempMNInfo);
						fflush(stdout);
						pthread_mutex_unlock(&mutexPrint);
						continue;
					}

					totalTimeElapsedForTCPRecv=totalTimeElapsedForTCPRecv+time_diff(beforeRecv,afterRecv);

					offset=((tempMNInfo.eventIDforSendResponse-thisResponseQueueHeadEventID)+thisResponseQueueHead)%MAX_OUTBOUNDING_EVENTS;
					numOfSwitchesResponsedForEvents[offset]++;

					if(IF_PRINT_DEBUG_INFO)
					{
						pthread_mutex_lock(&mutexPrint);
						printf("---- One response event recv: Thread(%ld) [%d.%d, ..., %d.%d] thisResponseQueueHead(%d) thisResponseQueueHeadEventID(%d) numOfEventsFinishedByThisThread(%d) ----\n"
							,pthread_self()
							,1,startSwitchID
							,1,endSwitchID
							,thisResponseQueueHead
							,thisResponseQueueHeadEventID
							,numOfEventsFinishedByThisThread);
						fflush(stdout);
						// printMessageEventQueue();
						printf("numOfSwitchesResponsedForEvents[]: \n");
						for(int k=0;k<MAX_OUTBOUNDING_EVENTS;k++)
						{
							if(k==thisResponseQueueHead)
								printf("*");
							printf("%d, ",numOfSwitchesResponsedForEvents[k]);
						}
						printf("\n");
						fflush(stdout);
						pthread_mutex_unlock(&mutexPrint);
					}

					if(numOfValidSwitchesInThisThread==numOfSwitchesResponsedForEvents[offset])
					{
						if(IF_PRINT_DEBUG_INFO)
						{
							pthread_mutex_lock(&mutexPrint);
							printf("---- One response event finish recv: Thread(%ld) [%d.%d, ..., %d.%d] thisResponseQueueHead(%d) thisResponseQueueHeadEventID(%d) numOfEventsFinishedByThisThread(%d) ----\n"
								,pthread_self()
								,1,startSwitchID
								,1,endSwitchID
								,thisResponseQueueHead
								,thisResponseQueueHeadEventID
								,numOfEventsFinishedByThisThread);
							fflush(stdout);
							// printMessageEventQueue();
							printf("numOfSwitchesResponsedForEvents[]: \n");
							for(int k=0;k<MAX_OUTBOUNDING_EVENTS;k++)
							{
								if(k==thisResponseQueueHead)
									printf("*");
								printf("%d, ",numOfSwitchesResponsedForEvents[k]);
							}
							printf("\n");
							fflush(stdout);
							pthread_mutex_unlock(&mutexPrint);
						}

						if(offset!=thisResponseQueueHead)
						{
							pthread_mutex_lock(&mutexPrint);
							printf("Out-of-order LS response! tempMNInfo.eventIDforSendResponse(%d), thisResponseQueueHeadEventID(%d), offset(%d)\n"
							,tempMNInfo.eventIDforSendResponse
							,thisResponseQueueHeadEventID
							,offset);
							printMNinfo(tempMNInfo);
							fflush(stdout);
							pthread_mutex_unlock(&mutexPrint);
							exit(1);
						}
						
						pthread_mutex_lock(&(MsgQueueEventMutex[thisResponseQueueHead]));
						messageEventQueue.eventQueue[thisResponseQueueHead].numOfSwitchesResponsed=messageEventQueue.eventQueue[thisResponseQueueHead].numOfSwitchesResponsed+numOfSwitchesResponsedForEvents[offset];
						thisEventNumOfSwitchesResponsedByThisThreadAfter=messageEventQueue.eventQueue[thisResponseQueueHead].numOfSwitchesResponsed;
						thisEventNumOfSwitchesShouldNotify=messageEventQueue.eventQueue[thisResponseQueueHead].numOfSwitchesShouldNotify;

						if(thisEventNumOfSwitchesResponsedByThisThreadAfter==thisEventNumOfSwitchesShouldNotify)
						//This message has been responded by all switches, clear it in the event queue
						{
							if(totalLSReportReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
					  		{
					  			pthread_mutex_lock(&mutexPrint);
					  			printCurrentTime();
					  			printf("---- %d LS response finish:\t",totalLSReportReceived);
						  		printMNinfo(tempMNInfo);
						  		printf("Thread core(%2d) [%d.%d, ..., %d.%d] ----\n"
						  			,sched_getcpu()
									,1,startSwitchID
									,1,endSwitchID);
						  		fflush(stdout);
						  		pthread_mutex_unlock(&mutexPrint);
						  	}

							CounterForLastRecvSwitch[tempMNInfo.responseNode.ID]++;
							timeElapsedTillAllResponseRecved=timeElapsedTillAllResponseRecved+time_diff(messageEventQueue.eventQueue[thisResponseQueueHead].tvStart,afterRecv);
	
							totalTimeElapsedForSlowestThreadStartToRecv=totalTimeElapsedForSlowestThreadStartToRecv+time_diff(messageEventQueue.eventQueue[thisResponseQueueHead].tvStart,beforeRecv);
							totalTimeElapsedForSlowestThreadTCPRecv=totalTimeElapsedForSlowestThreadTCPRecv+time_diff(messageEventQueue.eventQueue[thisResponseQueueHead].tvAllSent,afterRecv);
							//Note this timestamp may not be accurate, since tvAllSent may not be tagged by send thread now. But the possibility is very low

							if(localGenerateLSorGetLSFromSwitch==1)
							{
								//Send ACK event, blockingly
								messageEventQueue.eventQueue[thisResponseQueueHead].message.ReptorDelvorResporAck=3;
								messageEventQueue.eventQueue[thisResponseQueueHead].message.bye=false;
								sendToAllAffectedSwitchesSequential(messageEventQueue.eventQueue[thisResponseQueueHead].message, messageEventQueue.eventQueue[thisResponseQueueHead].message.reportNode.ID, messageEventQueue.eventQueue[thisResponseQueueHead].message.reportNode.ID);
								
								if(totalLSReportReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
						  		{
						  			pthread_mutex_lock(&mutexPrint);
						  			printCurrentTime();
						  			printf("++++ %d LS ACK sent out:\t",totalLSReportReceived);
							  		printMNinfo(messageEventQueue.eventQueue[thisResponseQueueHead].message);
							  		printf("Thread core(%2d) [%d.%d, ..., %d.%d] ----\n"
							  			,sched_getcpu()
										,1,startSwitchID
										,1,endSwitchID);
							  		fflush(stdout);
							  		pthread_mutex_unlock(&mutexPrint);
							  	}

								if(IF_PRINT_DEBUG_INFO)
								{
									pthread_mutex_lock(&mutexPrint);
									printf("---- Send ACK to reporting switch: Thread(%ld) [%d.%d, ..., %d.%d] thisResponseQueueHead(%d) thisResponseQueueHeadEventID(%d) numOfEventsFinishedByThisThread(%d) ----\n"
										,pthread_self()
										,1,startSwitchID
										,1,endSwitchID
										,thisResponseQueueHead
										,thisResponseQueueHeadEventID
										,numOfEventsFinishedByThisThread);
									fflush(stdout);
									pthread_mutex_unlock(&mutexPrint);
								}
								gettimeofday(&afterSendACK,NULL);
								timeElapsedTillACKSentOut=timeElapsedTillACKSentOut+time_diff(messageEventQueue.eventQueue[thisResponseQueueHead].tvStart,afterSendACK);
							}

							lastSlowestResponseSwitch.level=tempMNInfo.responseNode.level;
							lastSlowestResponseSwitch.ID=tempMNInfo.responseNode.ID;

							if(!dequeueMessageFromEventQueue())
							{
								fprintf(stderr, "dequeueMessageFromEventQueue ERROR!\n");
								exit(1);
							}

							if(IF_PRINT_DEBUG_INFO)
							{
								pthread_mutex_lock(&mutexPrint);
								printf("---- Dequeue message: Thread(%ld) [%d.%d, ..., %d.%d] thisResponseQueueHead(%d) thisResponseQueueHeadEventID(%d) numOfEventsFinishedByThisThread(%d) ----\n"
									,pthread_self()
									,1,startSwitchID
									,1,endSwitchID
									,thisResponseQueueHead
									,thisResponseQueueHeadEventID
									,numOfEventsFinishedByThisThread);
								fflush(stdout);
								// printResponseEventQueue();
								// fflush(stdout);
								printf("Successfully receive link-state response from %d switches ...\n",thisEventNumOfSwitchesShouldNotify);
								fflush(stdout);
								pthread_mutex_unlock(&mutexPrint);
							}
						}
						pthread_mutex_unlock(&(MsgQueueEventMutex[thisResponseQueueHead]));
					
						thisResponseQueueHead=(thisResponseQueueHead+1)%MAX_OUTBOUNDING_EVENTS;
						thisResponseQueueHeadEventID++;
						numOfEventsFinishedByThisThread++;
						numOfSwitchesResponsedForEvents[offset]=0;
					}

					if(IF_PRINT_DEBUG_INFO)
					{
						pthread_mutex_lock(&mutexPrint);
						printf("---- After recv: Thread(%ld) [%d.%d, ..., %d.%d] thisResponseQueueHead(%d) thisResponseQueueHeadEventID(%d) numOfEventsFinishedByThisThread(%d) ----\n"
							,pthread_self()
							,1,startSwitchID
							,1,endSwitchID
							,thisResponseQueueHead
							,thisResponseQueueHeadEventID
							,numOfEventsFinishedByThisThread);
						fflush(stdout);
						printMessageEventQueue();
						printf("numOfSwitchesResponsedForEvents[]: \n");
						for(int k=0;k<MAX_OUTBOUNDING_EVENTS;k++)
						{
							if(k==thisResponseQueueHead)
								printf("*");
							printf("%d, ",numOfSwitchesResponsedForEvents[k]);
						}
						printf("\n");
						fflush(stdout);
						pthread_mutex_unlock(&mutexPrint);
					}
				break;
				case 3:
				break;
				default:
				break;
			}
		}
	}
}

int createSendMessageThread(int threadNum, int testSwitchClientNum, int MutexCndWaitOrBusySleepWaitOrBusyYieldWait)
{
	// printf("createSendMessageThread\n");
	int numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);
    printf("createSendMessageThread.... Number of processors: %d\n", numberOfProcessors);
    // int useCores=numberOfProcessors-2;//Remain 2 cores for system and other processes
	int useCores=numberOfProcessors;

	int startSwitchID=0;
	int switchNum=0;
	int totalSwitchNum=0;
	pthread_t thread_id;
	for(int i=0;i<threadNum;i++)
	{
		SendMessageThreadParam *sendMessageThreadParam=(struct SendMessageThreadParam *)malloc(sizeof(struct SendMessageThreadParam));
		switchNum=0;
		for(int j=startSwitchID;j<TOR_PER_POD*POD_NUM;j++)
		{
			if(switchSockTable.switchSocks[j].switchNode.ID==-1)
				continue;
			if(startSwitchID==0 && totalSwitchNum==0)//Find the first switch client
				startSwitchID=j;
			switchNum++;
			totalSwitchNum++;
			// printf("j %d, startSwitchID %d\n",j, startSwitchID);
			if(switchNum>=int(ceil(double(testSwitchClientNum)/double(threadNum)))
				|| totalSwitchNum>=testSwitchClientNum)
			{
				sendMessageThreadParam->startSwitchID=startSwitchID;
				sendMessageThreadParam->endSwitchID=j;
				startSwitchID=j+1;
				break;
			}
		}

		sendMessageThreadParam->MutexCndWaitOrBusySleepWaitOrBusyYieldWait=MutexCndWaitOrBusySleepWaitOrBusyYieldWait;

		if(useCores>0)//Set thread affinity
		{
			pthread_attr_t attr;
		    cpu_set_t cpus;
		    pthread_attr_init(&attr);
		    CPU_ZERO(&cpus);
	       	CPU_SET(i%useCores, &cpus);//send thread use cores from No.0 to No.useCores-1

		    if(pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus)!=0)
		    {
		      fprintf(stderr, "ERROR! Thread [%d.%d, ..., %d.%d] set affinity failed!\n"
		      	,1,sendMessageThreadParam->startSwitchID
				,1,sendMessageThreadParam->endSwitchID);
		      exit(1);
		    }

		    if(pthread_create(&thread_id,&attr,sendMessageThread,(void*)sendMessageThreadParam)!=0)
		    {
		      fprintf(stderr, "ERROR! Thread [%d.%d, ..., %d.%d] create failed!\n"
		      	,1,sendMessageThreadParam->startSwitchID
				,1,sendMessageThreadParam->endSwitchID);
		      exit(1);
		    }
	    }
	    else
	    {
	    	if(pthread_create(&thread_id,NULL,sendMessageThread,(void*)sendMessageThreadParam)!=0)
		    {
		      fprintf(stderr, "ERROR! Thread [%d.%d, ..., %d.%d] create failed!\n"
		      	,1,sendMessageThreadParam->startSwitchID
				,1,sendMessageThreadParam->endSwitchID);
		      exit(1);
		    }
	    }

	    if(totalSwitchNum>=testSwitchClientNum)
	    	break;
	}
}

int createRecvMessageThread(int threadNum, int testSwitchClientNum, int ifTestEpoll, int MutexCndWaitOrBusySleepWaitOrBusyYieldWait, int localGenerateLSorGetLSFromSwitch)
{
	int numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);
    printf("createRecvMessageThread.... Number of processors: %d\n", numberOfProcessors);
    // int useCores=numberOfProcessors-2;//Remain 2 cores for system and other processes
    int useCores=numberOfProcessors;

	int startSwitchID=0;
	int switchNum=0;
	int totalSwitchNum=0;
	pthread_t thread_id;
	for(int i=0;i<threadNum;i++)
	{
		RecvMessageThreadParam *recvMessageThreadParam=(struct RecvMessageThreadParam *)malloc(sizeof(struct RecvMessageThreadParam));
		recvMessageThreadParam->ifTestEpoll=ifTestEpoll;
		recvMessageThreadParam->MutexCndWaitOrBusySleepWaitOrBusyYieldWait=MutexCndWaitOrBusySleepWaitOrBusyYieldWait;
		recvMessageThreadParam->testSwitchClientNum=testSwitchClientNum;
		recvMessageThreadParam->localGenerateLSorGetLSFromSwitch=localGenerateLSorGetLSFromSwitch;
		
		switchNum=0;
		for(int j=startSwitchID;j<TOR_PER_POD*POD_NUM;j++)
		{
			if(switchSockTable.switchSocks[j].switchNode.ID==-1)
				continue;
			if(startSwitchID==0 && totalSwitchNum==0)//Find the first switch client
				startSwitchID=j;
			switchNum++;
			totalSwitchNum++;
			// printf("j %d, startSwitchID %d\n",j, startSwitchID);
			if(switchNum>=int(ceil(double(testSwitchClientNum)/double(threadNum)))
				|| totalSwitchNum>=testSwitchClientNum)
			{
				recvMessageThreadParam->startSwitchID=startSwitchID;
				recvMessageThreadParam->endSwitchID=j;
				startSwitchID=j+1;
				break;
			}
		}

		if(useCores>0)//Set thread affinity
		{
			pthread_attr_t attr;
		    cpu_set_t cpus;
		    pthread_attr_init(&attr);
		    CPU_ZERO(&cpus);
	       	CPU_SET((useCores-1-i)%useCores, &cpus);//recv thread use cores from No.useCores-1 to No.0

		    if(pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus)!=0)
		    {
		      fprintf(stderr, "ERROR! Thread [%d.%d, ..., %d.%d] set affinity failed!\n"
		      	,1,recvMessageThreadParam->startSwitchID
				,1,recvMessageThreadParam->endSwitchID);
		      exit(1);
		    }

		    if(pthread_create(&thread_id,&attr,recvMessageThread,(void*)recvMessageThreadParam)!=0)
		    {
		      fprintf(stderr, "ERROR! Thread [%d.%d, ..., %d.%d] create failed!\n"
		      	,1,recvMessageThreadParam->startSwitchID
				,1,recvMessageThreadParam->endSwitchID);
		      exit(1);
		    }
	    }
	    else
	    {
	    	if(pthread_create(&thread_id,NULL,recvMessageThread,(void*)recvMessageThreadParam)!=0)
		    {
		      fprintf(stderr, "ERROR! Thread [%d.%d, ..., %d.%d] create failed!\n"
		      	,1,recvMessageThreadParam->startSwitchID
				,1,recvMessageThreadParam->endSwitchID);
		      exit(1);
		    }
	    }

	    if(totalSwitchNum>=testSwitchClientNum)
	    	break;
	}
}

void waitUntilAllReceivedEventsFinished()
{
	while(1)
	{
		pthread_mutex_lock(&(MsgQueueMutex));
		if(messageEventQueue.len>0)
		{
			pthread_mutex_unlock(&(MsgQueueMutex));
			usleep(100);
			continue;
		}
		pthread_mutex_unlock(&(MsgQueueMutex));

		break;//Previous event has all been processed
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

void recvResultCountMessageFromASock(ResultCountMessage *msg, int sock)
{
	int totalRecvSize=0;

	char recvBuf[sizeof(struct ResultCountMessage)];
	memset(recvBuf,0,sizeof(recvBuf));
	totalRecvSize=0;
	while(totalRecvSize<sizeof(struct ResultCountMessage))
	{
		int recvSize=recv(sock,recvBuf+totalRecvSize,sizeof(struct ResultCountMessage)-totalRecvSize,0);
	    if(recvSize<=0)
	    {
	      fprintf(stderr,"ERROR: thread(%ld) recv result count error from sock(%d).\n"
			,pthread_self()
			,sock);
	      exit(1);
	    }
	    totalRecvSize=totalRecvSize+recvSize;
	}
    memcpy(msg,recvBuf,sizeof(struct ResultCountMessage));
    if(IF_PRINT_DEBUG_INFO)
	    printf("Result from switch [%d.%d] (cotestWithMaster(%d) ifMonitorLocalNICChange(%d) ifDoSwitchCalc(%d)): avgTimeFromEpollNotifyToTCPRecvDone:%.0lf us,\tavgTimeForRouteUpdateElapsed:%.0lf us,\tavgTimeFromRouteUpdateDoneToTCPSendDone:%.0lf us\n"
		,msg->mySwitchNode.level,msg->mySwitchNode.ID
		,msg->cotestWithMaster
		,msg->ifMonitorLocalNICChange
		,msg->ifDoSwitchCalc
		,msg->avgTimeFromEpollNotifyToTCPRecvDone
		,msg->avgTimeForRouteUpdateElapsed
		,msg->avgTimeFromRouteUpdateDoneToTCPSendDone);
}

int main(int argc, char const *argv[])
{
	if(argc!=9)
	{
		printf("usage: executable ifTestEpoll(0|1) localGenerateLSorGetLSFromSwitch(0|1) testSwitchClientNum testTimes sendThreadNum recvThreadNum testDelayOrThroughput MutexCndWaitOrBusySleepWaitOrBusyYieldWait\n");
		printf("\tifTestEpoll(0|1): 0 test normal send and wait for response;\t1 test fast asynchronized epoll\n");
		printf("\tlocalGenerateLSorGetLSFromSwitch(0|1): 0 test local master generate LS updates;\t1 full test with switch, with LS update report/deliver/respond/ack transmitted over TCP\n");
		printf("\ttestSwitchClientNum: How many switches (clients) join this test\n");
		printf("\ttestTimes: How many link-states generated in this test\n");
		printf("\tsendThreadNum: Number of threads to send link-states to switches\n");
		printf("\trecvThreadNum: Number of threads to receive responses and link-states from switches\n");
		printf("\ttestDelayOrThroughput: 0 test delay (send another until the previous one has been all responsed); 1 test throughput (saturate the event queue)\n");
		printf("\tMutexCndWaitOrBusySleepWaitOrBusyYieldWait: How send threads fetch new sending events? 0 mutex condition wait; 1 busy waiting with 10us sleep interval; 2 busy waiting using sched yield\n");
		exit(1);
	}

	int ifTestEpoll=atoi(argv[1]);
	int localGenerateLSorGetLSFromSwitch=atoi(argv[2]);
	int testSwitchClientNum=atoi(argv[3]);//For whole network, should be (TOR_PER_POD*POD_NUM)
	if (testSwitchClientNum<0 || testSwitchClientNum>POD_NUM*TOR_PER_POD)
	{
		fprintf(stderr,"testSwitchClientNum %d ERROR!\n",testSwitchClientNum);
		exit(1);
	}
	int testTimes=atoi(argv[4]);
	if (testTimes<=0)
	{
		fprintf(stderr,"testTimes %d ERROR!\n",testTimes);
		exit(1);
	}
	int sendThreadNum=atoi(argv[5]);
	if (sendThreadNum<=0)
	{
		fprintf(stderr,"sendThreadNum %d ERROR!\n",sendThreadNum);
		exit(1);
	}
	int recvThreadNum=atoi(argv[6]);
	if (recvThreadNum<=0)
	{
		fprintf(stderr,"recvThreadNum %d ERROR!\n",recvThreadNum);
		exit(1);
	}
	int testDelayOrThroughput=atoi(argv[7]);

	int MutexCndWaitOrBusySleepWaitOrBusyYieldWait=atoi(argv[8]);
	if (MutexCndWaitOrBusySleepWaitOrBusyYieldWait!=0
		&& MutexCndWaitOrBusySleepWaitOrBusyYieldWait!=1
		&& MutexCndWaitOrBusySleepWaitOrBusyYieldWait!=2)
	{
		fprintf(stderr,"MutexCndWaitOrBusySleepWaitOrBusyYieldWait %d ERROR! Should be 0|1|2\n",MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
		exit(1);
	}


	initMessageEventQueues();

  	time_t t;
	srand((unsigned) time(&t));

	struct timeval tvStart,tvEnd,tvMiddle;

  	Link testLink;
  	bool linkState=true;
  	unsigned int eventID=0;

  	SwitchNode mySwitchNode;
	// generateRandomSwitch(1,&mySwitchNode);
	mySwitchNode.level=1;
	mySwitchNode.ID=0;

  	generateLinkTable(mySwitchNode);
	// printLinkTable();
	initiateSwitchSockTable();

	int serverSock=listenTCP();
	printf("TCP listening, Waiting for switches to connect ...\n");
	fflush(stdout);
	while(1)
	{
		int switchSockTableEntryID=acceptConnectionFromSwitch(serverSock);
		if(switchSockTable.connectedSwitchNum>=testSwitchClientNum)
		{
			if(IF_PRINT_DEBUG_INFO)
				printSwitchSockTable();
			printf("========= All %d switches have connected! =========\n\n\n\n\n",testSwitchClientNum);
			break;
		}
	}
	if(IF_PRINT_DEBUG_INFO)
		printSwitchSockTable();

	createSendMessageThread(sendThreadNum,testSwitchClientNum,MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
	createRecvMessageThread(recvThreadNum,testSwitchClientNum,ifTestEpoll,MutexCndWaitOrBusySleepWaitOrBusyYieldWait,localGenerateLSorGetLSFromSwitch);

	///********Code after this are for test mode
	{
		usleep(3000000);
		printf("\n\nWait for send/recv thread to start ...\n");
		fflush(stdout);

		//Notify LS reporter to start
		char sendBuf[]="ready to roll";
		int numOfSucessfulSend=0;
		for(int i=0;i<(POD_NUM*TOR_PER_POD);i++)
		{
			if(switchSockTable.switchSocks[i].switchNode.ID==-1)
				continue;
			if(switchSockTable.switchSocks[i].cotestWithMaster!=1)//Not a test LS reporter
				continue;
			
			if(send(switchSockTable.switchSocks[i].sock,sendBuf,strlen(sendBuf),0)<=0)
			{
				fprintf(stderr,"ERROR! send ready to roll to switch(%d.%d) error.\n"
					,switchSockTable.switchSocks[i].switchNode.level
					,switchSockTable.switchSocks[i].switchNode.ID);
				exit(1);
		    	// usleep(CONNECT_INTERVAL);
			}
			numOfSucessfulSend++;
			printf("Send ready to roll to switch[%d.%d]...\n",1,i);
			fflush(stdout);
		}

		printf("\n\nSend ready to roll to %d LS reporters. TEST begin!\n",numOfSucessfulSend);
		fflush(stdout);
	}
	///********Code before this are for test mode
		


	if(localGenerateLSorGetLSFromSwitch==0)
	{
		while(totalLSReportReceived<testTimes)
		{
			// usleep(100000);//Push a LS report every 100ms
			// testLink.srcNode.level=1;
			// testLink.srcNode.ID=0;
			// testLink.dstNode.level=2;
			// testLink.dstNode.ID=0;
			generateRandomLink(&testLink);
	  		linkState=!(linkTable[getLinkTableEntryID(testLink)].state);
	  		eventID=linkTable[getLinkTableEntryID(testLink)].eventID+1;

	  		//Update link table
			linkTable[getLinkTableEntryID(testLink)].eventID=eventID;
			linkTable[getLinkTableEntryID(testLink)].state=linkState;

	  		if(testDelayOrThroughput==0)//test delay
				waitUntilAllReceivedEventsFinished();

			//Make sure to generate new link state that needs to update path table
	  		totalLSReportReceived++;
			
	  		gettimeofday(&tvStart,NULL);

	  		MNinfo  tempMNInfo;
			tempMNInfo.link.srcNode.level=testLink.srcNode.level;
			tempMNInfo.link.srcNode.ID=testLink.srcNode.ID;
			tempMNInfo.link.dstNode.level=testLink.dstNode.level;
			tempMNInfo.link.dstNode.ID=testLink.dstNode.ID;
			tempMNInfo.reportNode.level=-1;//Fake LS report
			tempMNInfo.reportNode.ID=-1;//Fake LS report
			tempMNInfo.eventID=eventID;
			tempMNInfo.linkState=linkState;
			tempMNInfo.bye=false;
			tempMNInfo.eventIDforSendResponse=totalLSReportReceived;
			tempMNInfo.ReptorDelvorResporAck=1;//Fake a LS delivery

			if(totalLSReportReceived%PRINT_PGOGRESS_ROUND_INTERVAL==0 || IF_PRINT_DEBUG_INFO)
	  		{
	  			pthread_mutex_lock(&mutexPrint);
	  			printCurrentTime();
	  			printf("++++ %d new LS report:\t",totalLSReportReceived);
		  		printMNinfo(tempMNInfo);
		  		printf("main thread core(%2d) ----\n"
		  			,sched_getcpu());
		  		fflush(stdout);
		  		pthread_mutex_unlock(&mutexPrint);
		  	}

			MessageEvent msg;
	  		memcpy(&(msg.message),&tempMNInfo,sizeof(MNinfo));
	  		msg.numOfSwitchesShouldNotify=testSwitchClientNum;
	  		msg.numOfSwitchesSent=0;
	  		msg.numOfSwitchesResponsed=0;
	  		
	  		enqueueMessageIntoEventQueueUntilSucceed(msg,MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
		}
	}

	while(totalLSReportReceived<testTimes)
		usleep(1000000);
	
	waitUntilAllReceivedEventsFinished();

	if(IF_PRINT_DEBUG_INFO)
	{
		printf("\n\n\n\nSwitchID\tLastSendTimes\tLastRecvTimes\n");
		for(int i=0;i<POD_NUM*TOR_PER_POD;i++)
		{
			if(switchSockTable.switchSocks[i].switchNode.ID==-1)
				continue;

			printf("%6d\t%6d\t%6d\n",i,CounterForLastSendSwitch[i],CounterForLastRecvSwitch[i]);
		}
	}

	printf("\n\n\n\n\n");
	printf("\tTotal %d new LS reports processed\n",totalLSReportReceived);
	
	if(localGenerateLSorGetLSFromSwitch==0)
		printf("========= LS reports locally generated (%d) ========\n",localGenerateLSorGetLSFromSwitch);
	else
		printf("========= LS reports from switch client (%d): (receive from switch/ deliver to switches/ receive response from switches/ send ack to switch) ========\n",localGenerateLSorGetLSFromSwitch);
	
	if(ifTestEpoll)
		printf("========= test epoll recv (%d) ========\n",ifTestEpoll);
	else
		printf("========= test normal recv (%d) ========\n",ifTestEpoll);

	printf("Total time elapsed:%.0lf us, average time to completely process a new link-state:%.0lf us\n", totalTimeElapsed, totalTimeElapsed/totalLSReportReceived);
	printf("Time elapsed till all threads send out link-state to switches:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillLinkStateAllSentOut, timeElapsedTillLinkStateAllSentOut/totalLSReportReceived);
	printf("Time elapsed till all response received from switches:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillAllResponseRecved, timeElapsedTillAllResponseRecved/totalLSReportReceived);
	if(localGenerateLSorGetLSFromSwitch==1)
		printf("Time elapsed till ACK sent out to the reporting switch:%.0lf us, average per link-state:%.0lf us\n", timeElapsedTillACKSentOut, timeElapsedTillACKSentOut/totalLSReportReceived);
	printf("Time elapsed in TCP send for link-state delivery:%.0lf us, average per link-state per switch:%.2lf us\n", totalTimeElapsedForTCPSend, totalTimeElapsedForTCPSend/(totalLSReportReceived*testSwitchClientNum));
	printf("Time elapsed in TCP recv for link-state response:%.0lf us, average per link-state per switch:%.2lf us\n", totalTimeElapsedForTCPRecv, totalTimeElapsedForTCPRecv/(totalLSReportReceived*testSwitchClientNum));
	

	if(sendThreadNum!=0 && recvThreadNum!=0)//Multi-thread mode 
	{
		printf("\n\n\n--- Thread perf: sendThreadNum(%d), recvThreadNum(%d), TestDelayOrThroughput[0 dl|1 tp](%d), MutexCndWaitOrBusySleepWaitOrBusyYieldWait[0 cnd_wait|1 sleep|2 yield](%d) ----\n"
			,sendThreadNum,recvThreadNum,testDelayOrThroughput,MutexCndWaitOrBusySleepWaitOrBusyYieldWait);
		printf("Time elapsed for the slowest thread spent in TCP send:%.0lf us, average time per link-state:%.0lf us\n", totalTimeElapsedForSlowestThreadTCPSend, totalTimeElapsedForSlowestThreadTCPSend/totalLSReportReceived);
		printf("Time elapsed for the slowest thread spent in TCP recv (from the slowest TCP send):%.0lf us, average time per thread per link-state:%.0lf us\n", totalTimeElapsedForSlowestThreadTCPRecv, totalTimeElapsedForSlowestThreadTCPRecv/totalLSReportReceived);

		printf("Time elapsed from link-states generated to the slowest thread starts to send:%.0lf us, average time per link-state:%.0lf us\n", totalTimeElapsedForSlowestThreadStartToSend, totalTimeElapsedForSlowestThreadStartToSend/totalLSReportReceived);
		printf("Time elapsed from link-states generated to the slowest thread starts to recv:%.0lf us, average time per link-state:%.0lf us\n", totalTimeElapsedForSlowestThreadStartToRecv, totalTimeElapsedForSlowestThreadStartToRecv/totalLSReportReceived);
	}

	AllSendRecvThreadExit=true;
	//Let all threads exit

	MNinfo tempMNInfo;
	tempMNInfo.bye=true;
	int numOfSucessfulSend=sendToAllAffectedSwitchesSequential(tempMNInfo,0,POD_NUM*TOR_PER_POD-1);
	printf("\n\nTEST end: Successfully send finish to %d switches.\n",numOfSucessfulSend);
	fflush(stdout);

	usleep(1000000);

	printf("\n\n\n----- Result from switch side -----\n");
	printf("Avg time spent in the slowest switch for each LS:\n");
	printf("avgTimeFromEpollNotifyToTCPRecvDone:%.0lf us,\tavgTimeForRouteUpdateElapsed:%.0lf us,\tavgTimeFromRouteUpdateDoneToTCPSendDone:%.0lf us\n"
		,totalSlowestSwitchTimeFromEpollNotifyToTCPRecvDone/(totalLSReportReceived-1)
		,totalSlowestSwitchTimeForRouteUpdateElapsed/(totalLSReportReceived-1)
		,totalSlowestSwitchTimeFromRouteUpdateDoneToTCPSendDone/(totalLSReportReceived-1));
	fflush(stdout);

	double avgTimeFromEpollNotifyToTCPRecvDone=0;
	double avgTimeForRouteUpdateElapsed=0;
	double avgTimeFromRouteUpdateDoneToTCPSendDone=0;
	int numOfSucessfulRecv=0;
	printf("\n\n");
	for(int i=0;i<POD_NUM*TOR_PER_POD;i++)
	{
		if(switchSockTable.switchSocks[i].switchNode.ID==-1)
			continue;

		int sock=switchSockTable.switchSocks[i].sock;
		ResultCountMessage msg;
		recvResultCountMessageFromASock(&msg, sock);
		shutdown(sock,SHUT_RDWR);
		close(sock);

		if(msg.cotestWithMaster==1)
		{
			printf("\tSwitch %d is LS reporter!",i);
		}
		if(msg.ifMonitorLocalNICChange!=0)
		{
			printf("\tSwitch %d monitors local NIC link-state change!",i);
		}
		if(msg.ifDoSwitchCalc==1)
		{
			printf("\tSwitch %d calculates route update!",i);
		}

		numOfSucessfulRecv++;
		avgTimeFromEpollNotifyToTCPRecvDone=avgTimeFromEpollNotifyToTCPRecvDone+msg.avgTimeFromEpollNotifyToTCPRecvDone;
		avgTimeForRouteUpdateElapsed=avgTimeForRouteUpdateElapsed+msg.avgTimeForRouteUpdateElapsed;
		avgTimeFromRouteUpdateDoneToTCPSendDone=avgTimeFromRouteUpdateDoneToTCPSendDone+msg.avgTimeFromRouteUpdateDoneToTCPSendDone;
	}
	printf("\n\t\tOther switches only do LS receive and response!\n");
	printf("\n");

	printf("\nAvg time spent in all %d switches for each LS:\n",numOfSucessfulRecv);
	printf("avgTimeFromEpollNotifyToTCPRecvDone:%.0lf us,\tavgTimeForRouteUpdateElapsed:%.0lf us,\tavgTimeFromRouteUpdateDoneToTCPSendDone:%.0lf us\n"
		,avgTimeFromEpollNotifyToTCPRecvDone/numOfSucessfulRecv
		,avgTimeForRouteUpdateElapsed/numOfSucessfulRecv
		,avgTimeFromRouteUpdateDoneToTCPSendDone/numOfSucessfulRecv);
	fflush(stdout);


	/* start */
	ofstream write( "/home/guolab/LFS/NSDI/master_result.txt", ios::app);
	write << "******************************number of switch: " << argv[3] << "******************************" << std::endl;
	write << "total time(average): " << totalTimeElapsed/totalLSReportReceived << std::endl;
	write << "master cal(average): " << timeElapsedTillLinkStateAllSentOut/totalLSReportReceived << std::endl;
	write << "master recv LS--send all response(average): " << timeElapsedTillAllResponseRecved/totalLSReportReceived << std::endl;
	write << "tcp send one LS(average): " << totalTimeElapsedForTCPSend/(totalLSReportReceived*testSwitchClientNum) << std::endl;
	write << "tcp recv one LS response(average): " << totalTimeElapsedForTCPRecv/(totalLSReportReceived*testSwitchClientNum) << std::endl;
	write.close ();
	/* end */

	return 0;
}