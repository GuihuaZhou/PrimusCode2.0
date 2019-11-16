#include "ipv4-global-routing.h"

using namespace std;
pthread_mutex_t mutex;

Ipv4GlobalRouting::~Ipv4GlobalRouting()
{
  // ofstream Logfout("/var/log/Primus.log",ios::app);
  // Logfout << GetNow() << "Goodbye!" << endl;
  // unsigned long mask;
  // unsigned int prefixLen;
  // // free(masterLinkTable);
  // // free(mappingEntryTable);
  // struct pathtableentry *tempPathEntryA=headPathTableEntry->next;
  // struct pathtableentry *tempPathEntryB;
  // while (tempPathEntryA!=NULL)
  // {
  //   tempPathEntryB=tempPathEntryA;
  //   tempPathEntryA=tempPathEntryA->next;
  //   if (tempPathEntryB->pathEntry.size()!=1)
  //   {
  //     mask=tempPathEntryB->pathEntry[tempPathEntryB->pathEntry.size()-1].nodeMask.sin_addr.s_addr;
  //     prefixLen=0;//前缀长度
  //     while (mask)
  //     {
  //       mask=mask/2;
  //       prefixLen++;
  //     }
  //     DelRoute(tempPathEntryB->pathEntry[tempPathEntryB->pathEntry.size()-1].nodeAddr,prefixLen);
  //     Logfout << GetNow() << "Del " << inet_ntoa(tempPathEntryB->pathEntry[tempPathEntryB->pathEntry.size()-1].nodeAddr.sin_addr) << "/" << prefixLen << "." << endl;

  //     for (int i=0;i<SERVER_SIZE;i++)
  //     {
  //       if (!strcmp(inet_ntoa(tempPathEntryB->server[i].serverAddr.sin_addr),"255.255.255.255")) break;
  //       else
  //       {
  //         mask=tempPathEntryB->server[i].serverMask.sin_addr.s_addr;
  //         prefixLen=0;//前缀长度
  //         while (mask)
  //         {
  //           mask=mask/2;
  //           prefixLen++;
  //         }
  //         DelRoute(tempPathEntryB->server[i].serverAddr,prefixLen);
  //         Logfout << GetNow() << "Del " << inet_ntoa(tempPathEntryB->server[i].serverAddr.sin_addr) << "/" << prefixLen << "." << endl;
  //       }
  //     }
  //   }
  //   free(tempPathEntryB);
  // }
  // Logfout.close();
}

Ipv4GlobalRouting::Ipv4GlobalRouting(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultLinkTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  myIdent.level=level; 
  myIdent.position=position;
  m_ToRNodes=ToRNodes;
  m_LeafNodes=LeafNodes;
  m_SpineNodes=SpineNodes;
  m_nPods=nPods;
  m_Pod=Pod;
  m_nMaster=nMaster;
  m_Links=Links;
  m_defaultLinkTimer=defaultLinkTimer;
  m_defaultKeepaliveTimer=defaultKeepaliveTimer;
  m_IsCenterRouting=IsCenterRouting;
  m_randomEcmpRouting=randomEcmpRouting;
 
  ident tempNode;
  tempNode.level=-1;
  tempNode.position=-1;

  if (myIdent.level==0 && myIdent.position==0) ChiefMaster=true;
  else ChiefMaster=false;

  if (myIdent.level!=0)//node
  {
    struct sockaddr_in tempAddr;
    tempAddr.sin_family=AF_INET;
    inet_aton("255.255.255.255",&(tempAddr.sin_addr));
    tempAddr.sin_port=htons(0);

    for (int j=0;j<MAX_PATH_LEN;j++) headPathTableEntry->pathNodeIdent[j]=tempNode;
    
    struct addrset tempAddrSet;
    tempAddrSet.addr=tempAddr;
    tempAddrSet.prefixLen=32;

    headPathTableEntry->pathAddrSet=NULL;
    headPathTableEntry->nodeAddr=tempAddrSet;
    headPathTableEntry->nextHopAddr=tempAddrSet;
    headPathTableEntry->dirConFlag=0;
    headPathTableEntry->linkCounter=0;//头节点中的linkcounter用来记录路径表条目数量
    headPathTableEntry->nodeCounter=0;
    headPathTableEntry->next=NULL;
  }
  else if (myIdent.level==0)// master
  {
    headLinkTableEntry->high=tempNode;
    headLinkTableEntry->low=tempNode;
    headLinkTableEntry->linkFlag=false;
    headLinkTableEntry->lastLinkFlag=false;
    headLinkTableEntry->lastUpdateTime=0;//用来统计链路数量
    headLinkTableEntry->linkUpdateTimer=m_defaultLinkTimer;
    headLinkTableEntry->isStartTimer=false;
    headLinkTableEntry->next=NULL;
  }

  m_tcp=new TCPRoute(this,m_defaultKeepaliveTimer);
  if(myIdent.level!=0)
  {
    m_udpServer.SetGlobalRouting(this);
    m_udpClient.SetGlobalRouting(this);
    // sonic测试
    ifstream logfin("/home/tencent/Primus/NICName.txt");
    string str;
    while (getline(logfin,str))
    {
      int i=0;
      while (str[i++]!=':');
      if ((myIdent.level+myIdent.position*0.1)==atof((str.substr(0,i)).c_str()))//
      {
        int pos=i;
        for (;i<str.length();i++)
        {
          if (str[i]==',') 
          {
            listenNICName.push_back(str.substr(pos,i-pos));
            pos=i+1;
          }
        }
        logfin.close();
        break;
      }
    }
  }
  Logfout.close();
}

// sonic test
bool 
Ipv4GlobalRouting::isListenNIC(string ifName)
{
  for (int i=0;i<listenNICName.size();i++)
  {
    if (ifName==listenNICName[i]) return true;
  }
  return false;
}

vector<string>
Ipv4GlobalRouting::GetListenNIC()
{
  return listenNICName;
}
// end

void
Ipv4GlobalRouting::Start(vector<string> tempMasterAddress)
{
  masterAddress=tempMasterAddress;
  if(myIdent.level==0)
  {
    // std::cout << "I am master!" << std::endl;
    m_tcp->StartListen();
    // m_tcpServer.KeepAliveProcess();
  }
  else
  {
    // std::cout << "I am not master!" << std::endl;
    m_udpServer.StartApplication();
    sleep(5);
    ListenInterfacesAndSubmit();
    m_tcp->StartListen();
    // m_tcpClient.KeepAliveTimer(myident.level,myident.position);
    pthread_exit(NULL);
  }
}


ident
Ipv4GlobalRouting::GetMyIdent()
{
  return myIdent;
}

int
Ipv4GlobalRouting::rta_addattr_l(struct rtattr *rta, size_t maxlen, int type, void *data, size_t alen)
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
Ipv4GlobalRouting::addattr_l(struct nlmsghdr *n, size_t maxlen, int type, void *data, size_t alen)
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

int
Ipv4GlobalRouting::addattr32 (struct nlmsghdr *n, size_t maxlen, int type, int data)
{
  size_t len;
  struct rtattr *rta;

  len = RTA_LENGTH (4);

  if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr *) (((char *) n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA (rta), &data, 4);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

int
Ipv4GlobalRouting::OpenNetlink()
{
  struct sockaddr_nl saddr;
  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

  if (sock < 0) {
    perror("Failed to open netlink socket");
    return -1;
  }

  memset(&saddr, 0, sizeof(saddr));

  saddr.nl_family = AF_NETLINK;
  saddr.nl_pid = 0;       //与内核通信,nl_pid设为0
  saddr.nl_pad = 0 ;
  saddr.nl_groups = 0;

  if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("Failed to bind to netlink socket");
    close(sock);
    return -1;
  }

  return sock;
}

string
Ipv4GlobalRouting::GetNow(){
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime(&tt);
  stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

double
Ipv4GlobalRouting::GetSystemTime()
{
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC,&tv);
  return tv.tv_sec+tv.tv_nsec*0.000000001;
}

bool
Ipv4GlobalRouting::SameNode(ident nodeA,ident nodeB)
{
  if (nodeA.level==nodeB.level && nodeA.position==nodeB.position) return true;
  else return false;
}

/**************************Node**************************/
void 
Ipv4GlobalRouting::UpdateMasterMapToSock(struct mastermaptosock tempMasterMapToSock,int cmd)//cmd=1，添加；cmd=-1，删除
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (cmd==1)
  {
    for (int i=0;i<masterMapToSock.size();i++)
    {
      if (masterMapToSock[i].masterSock==tempMasterMapToSock.masterSock && masterMapToSock[i].masterIdent.level==-1)// 已经存在了，且master ident无效
      {
        if (tempMasterMapToSock.masterAddr=="")// 收到ack后来修改ident
        {
          masterMapToSock[i].masterIdent=tempMasterMapToSock.masterIdent;// 收到ack后改ident
          PrintMasterMapToSock();
          return;
        }
      }
    }
    // 不存在，则添加
    masterMapToSock.push_back(tempMasterMapToSock);
    PrintMasterMapToSock();
  } 
  else if (cmd==-1)
  {
    for (auto iter=masterMapToSock.begin();iter!=masterMapToSock.end();iter++)
    {
      if ((*iter).masterSock==tempMasterMapToSock.masterSock)
      {
        iter=masterMapToSock.erase(iter);
        PrintMasterMapToSock();
        break;
      }
    }
  } 
  Logfout.close();
}

string 
Ipv4GlobalRouting::GetNICNameByRemoteAddr(struct sockaddr_in addr)
{
  string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    if (!strcmp(inet_ntoa(NICInfo[i].neighborAddr.sin_addr),remoteAddr.c_str()))
    {
      return NICInfo[i].NICName;
    }
  }

  // NICInfo来不及初始化
  struct ifaddrs *ifa;

  if (0!=getifaddrs(&ifa))
  {
    printf("getifaddrs error\n");
    exit(0);
  }
  for (;ifa!=NULL;)
  {
    if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && ifa->ifa_name[0]=='e')
    {
      remoteAddr=inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr);
      remoteAddr[remoteAddr.size()-1]=(remoteAddr[remoteAddr.size()-1]=='1')?'2':'1';
      if (!strcmp(inet_ntoa(addr.sin_addr),remoteAddr.c_str()))
      {
        return ifa->ifa_name;
      }
    }
    ifa=ifa->ifa_next;
  }
  freeifaddrs(ifa);
  return "";
}

ident 
Ipv4GlobalRouting::GetNeighborIdentByRemoteAddr(struct sockaddr_in addr)
{
  string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    string NICRemoteAddr=inet_ntoa(NICInfo[i].neighborAddr.sin_addr);
    if (!strcmp(NICRemoteAddr.c_str(),remoteAddr.c_str())) return NICInfo[i].neighborIdent;
  } 
}

void 
Ipv4GlobalRouting::GetLocalAddrByRemoteAddr(struct sockaddr_in *localAddr,struct sockaddr_in addr)
{
  string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    string NICRemoteAddr=inet_ntoa(NICInfo[i].neighborAddr.sin_addr);
    if (!strcmp(NICRemoteAddr.c_str(),remoteAddr.c_str())) 
    {
      *localAddr=NICInfo[i].localAddr;
      break;
    }
  } 
}

vector<struct mastermaptosock> 
Ipv4GlobalRouting::GetMasterMapToSock()//或者node和master的连接信息
{
  return masterMapToSock;
}

vector<struct nodemaptosock> 
Ipv4GlobalRouting::GetNodeMapToSock()
{
  return nodeMapToSock;
}

void
Ipv4GlobalRouting::HandleMessage(ident nodeA,ident nodeB,bool linkFlag)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  ident high,low;
  if (nodeA.level>nodeB.level)
  {
    high=nodeA;
    low=nodeB;
  }
  else if (nodeA.level<=nodeB.level)
  {
    high=nodeB;
    low=nodeA;
  }
  if(myIdent.level==0)
  {
    //同时判断链路信息是否需要转发，因为链路信息可能已经处理了
    bool isNeedToNotice=UpdateMasterLinkTable(high,low,linkFlag);
    if (ChiefMaster)//如果是选举出来的处理信息的master
    {
      if (isNeedToNotice) SendMessageToNode(high,low,linkFlag);
    }
  }
  else //node 处理从master收到的链路变化信息
  {

    if (low.level==0)// 这是master的直连信息
    {
      ModifyNodeDirConFlag(high,low,linkFlag);
    }
    else
    {
      ModifyPathEntryTable(high,low,linkFlag);   
    }
    // ModifyPathEntryTable(high,low,linkFlag);
  }
  Logfout.close();
}

bool 
Ipv4GlobalRouting::IsDetectNeighbor(struct sockaddr_in addr)//根据邻居的地址来检测NICInfo中是否有相应的表项
{
  string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    if (!strcmp(inet_ntoa(NICInfo[i].neighborAddr.sin_addr),remoteAddr.c_str()))
    {
      return true;
    }
  }
  return false;
}

bool 
Ipv4GlobalRouting::IsLegalNeighbor(struct NICinfo tempNICInfo)//判断邻居是否合法，如果邻居为无效连接或者服务器，返回false
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // Logfout << "********** " << "neighborIdent is " << tempNICInfo.neighborIdent.level << "." << tempNICInfo.neighborIdent.position << endl;  
  // Logfout << "********** " << "isSwitch:" << tempNICInfo.isSwitch << ",isServer:" << tempNICInfo.isServer << endl;
  if (tempNICInfo.neighborIdent.level==-1 || tempNICInfo.neighborIdent.position==-1 || tempNICInfo.NICName=="") return false;
  else 
  {
    if ((myIdent.level==1 && ((!tempNICInfo.isSwitch && tempNICInfo.isServer) || (tempNICInfo.isSwitch && !tempNICInfo.isServer))) || (myIdent.level!=1 && tempNICInfo.isSwitch==true)) return true;
    else return false;
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::NDRecvHello(struct NDinfo tempNDInfo)//改neighborIdent，收到邻居发来的ND
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isFindNICInfo=false;
  string remoteAddr=inet_ntoa(tempNDInfo.localAddr.sin_addr);
  // Logfout << "********** " << "Recv hello,remoteAddr is " << remoteAddr << endl;
  for (int i=0;i<NICInfo.size();i++)
  {
    if (!strcmp(inet_ntoa(NICInfo[i].neighborAddr.sin_addr),remoteAddr.c_str()))// 存在
    {
      // Logfout << "********** " << "Recv hello,same addr is " << inet_ntoa(NICInfo[i].neighborAddr.sin_addr) << endl;
      // Logfout << "********** " << "there is a NICInfo" << endl;
      NICInfo[i].neighborIdent=tempNDInfo.myIdent;
      isFindNICInfo=true;
      if (IsLegalNeighbor(NICInfo[i])) 
      {
        // Logfout << "********** " << "Recv hello,addr is " << inet_ntoa(NICInfo[i].neighborAddr.sin_addr) << ",name is " << NICInfo[i].NICName << endl;
        FreshNeighboorList(NICInfo[i]);
      }
      break;
    }
  }
  if (isFindNICInfo==false)
  {
    // Logfout << "********** " << "there is no NICInfo" << endl;
    struct NICinfo temp;
    temp.NICName="";
    temp.isMaster=false;
    temp.isServer=false;
    temp.isSwitch=true;// 主动收到对面的hello，肯定是switch
    temp.flag=true;//up
    temp.neighborIdent=tempNDInfo.myIdent;
    temp.neighborAddr=tempNDInfo.localAddr;// 此localAddr是邻居发来的
    temp.localAddr=tempNDInfo.localAddr;

    string localAddr=inet_ntoa(tempNDInfo.localAddr.sin_addr);
    localAddr[localAddr.size()-1]=(localAddr[localAddr.size()-1]=='1')?'2':'1';// 邻居的地址必须是理论上的那个
    temp.localAddr.sin_addr.s_addr=inet_addr((char*)localAddr.c_str());

    temp.localMask=tempNDInfo.localAddr;// 掩码不确定

    NICInfo.push_back(temp);
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::NDRecvACK(struct NDinfo tempNDInfo)//改isSwitch，收到自己发出的ND的ACK
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isFindNICInfo=false;
  string localAddr=inet_ntoa(tempNDInfo.localAddr.sin_addr);//
  // Logfout << "********** " << "Recv ACK,localAddr is " << localAddr << endl;
  for (int i=0;i<NICInfo.size();i++)
  {
    if (!strcmp(inet_ntoa(NICInfo[i].localAddr.sin_addr),localAddr.c_str()))// 存在
    {
      // Logfout << "********** " << "Recv ACK,same addr is " << inet_ntoa(NICInfo[i].localAddr.sin_addr) << endl;
      if (tempNDInfo.myIdent.level==-1 && myIdent.level==1)// 发现服务器
      {
        NICInfo[i].isServer=true;
      }
      else if (tempNDInfo.myIdent.level!=-1)
      {
        NICInfo[i].isSwitch=true;
      }
      if (IsLegalNeighbor(NICInfo[i])) 
      {
        // Logfout << "********** " << "Recv ACK,addr is " << inet_ntoa(NICInfo[i].localAddr.sin_addr) << ",name is " << NICInfo[i].NICName << endl;
        FreshNeighboorList(NICInfo[i]);
      }
      break;
    }
  }
  Logfout.close();
}

bool 
Ipv4GlobalRouting::IsLegalPathInfo(ident tempPathNodeIdent[],ident neighborIdent)
{
  //判断路径中是否包含邻居，如果邻居为spine，路径中还不能包含spine
  for (int i=0;i<MAX_PATH_LEN;i++)
  {
    if (tempPathNodeIdent[i].level==-1 && tempPathNodeIdent[i].position==-1) break;
    if (tempPathNodeIdent[i].level==neighborIdent.level && tempPathNodeIdent[i].position==neighborIdent.position) return false;//路径中包含了邻居
    if (tempPathNodeIdent[i].level==3 && neighborIdent.level==3) return false;//不能发给spine    
    if (tempPathNodeIdent[i].level==2 && neighborIdent.level==2 && myIdent.level==1) return false;//tor不能向leafnode转发包含leafnode的路径
  }
  return true;
}

void 
Ipv4GlobalRouting::FreshNeighboorList(struct NICinfo tempNICInfo)//邻居发现完成
{
  pthread_mutex_lock(&mutex);
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (tempNICInfo.isServer && myIdent.level==1)//邻居是服务器
  {
    Logfout << GetNow() << tempNICInfo.NICName << ",server's ip is " << inet_ntoa(tempNICInfo.neighborAddr.sin_addr) << "." << endl;
  }
  else if (tempNICInfo.isServer && myIdent.level!=1)//邻居无效
  {
    Logfout << GetNow() << tempNICInfo.NICName << ",neighbor is illegal and its ip is " << inet_ntoa(tempNICInfo.neighborAddr.sin_addr) << "." << endl;
  }
  else if (tempNICInfo.isSwitch)//邻居是交换机
  {
    Logfout << GetNow() << tempNICInfo.NICName << ",neighbor's ip is " << inet_ntoa(tempNICInfo.neighborAddr.sin_addr) << ",position is " << tempNICInfo.neighborIdent.level << "." << tempNICInfo.neighborIdent.position << "." << endl;
    
    struct MNinfo tempMNInfo;
    tempMNInfo.srcIdent=myIdent;
    tempMNInfo.pathNodeIdentA=myIdent;
    tempMNInfo.pathNodeIdentB=tempNICInfo.neighborIdent;
    tempMNInfo.keepAlive=false;
    tempMNInfo.linkFlag=true;
    tempMNInfo.hello=false;
    tempMNInfo.ACK=false;

    for (int j=0;j<masterMapToSock.size();j++)// 向master上报链路信息
    {
      if (masterMapToSock[j].masterIdent.level!=-1)
      {
        Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdentA.level << "." << tempMNInfo.pathNodeIdentA.position << "--" << tempMNInfo.pathNodeIdentB.level << "." << tempMNInfo.pathNodeIdentB.position << " up to master ";
        Logfout << masterMapToSock[j].masterIdent.level << "." << masterMapToSock[j].masterIdent.position << "." << endl;
        tempMNInfo.destIdent=masterMapToSock[j].masterIdent;
        m_tcp->SendMessageTo(masterMapToSock[j].masterSock,tempMNInfo);
      }
    }
  }

  if (tempNICInfo.isServer && myIdent.level!=1)//无效邻居直接跳过
  {
    exit(0);
  }
  else
  {
    //路径初始化
    struct pathinfo *tempPathInfo=(struct pathinfo *)malloc(sizeof(struct pathinfo));
    
    ident tempNode;
    tempNode.level=-1;
    tempNode.position=-1;

    // 此处可能出问题
    struct sockaddr_in tempAddr;
    tempAddr.sin_family=AF_INET;
    inet_aton("255.255.255.255",&(tempAddr.sin_addr));
    tempAddr.sin_port=htons(0);

    for (int j=0;j<MAX_PATH_LEN;j++) tempPathInfo->pathNodeIdent[j]=tempNode;

    for (int j=0;j<MAX_ADDR_NUM;j++) 
    {
      tempPathInfo->addrSet[j].addr=tempAddr;
      tempPathInfo->addrSet[j].prefixLen=32;
    }

    tempPathInfo->nodeAddr.addr=tempAddr;
    tempPathInfo->nodeAddr.prefixLen=32;

    tempPathInfo->nextHopAddr.addr=tempAddr;
    tempPathInfo->nextHopAddr.prefixLen=32;

    tempPathInfo->nodeCounter=0;
    if (masterMapToSock.size())// 简单办法，nodeConMasterSock里记录了所有和master相连的套接字
    {
      tempPathInfo->dirConFlag=true;
    }
    else 
    {
      tempPathInfo->dirConFlag=false;
    }
    
    //路径初始化完成

    if (tempNICInfo.isServer && myIdent.level==1)//探测到服务器，则先插入路径表，再向所有合法邻居交换机发送该路径
    {
      tempPathInfo->nodeCounter++;
      tempPathInfo->pathNodeIdent[tempPathInfo->nodeCounter-1]=myIdent;

      tempPathInfo->addrSet[0].addr=tempNICInfo.neighborAddr;
      tempPathInfo->addrSet[0].prefixLen=NetmaskToPrefixlen(tempNICInfo.localMask);

      AddNewPathTableEntry(*tempPathInfo);

      for (int j=0;j<NICInfo.size();j++)
      {
        if (IsLegalNeighbor(NICInfo[j]))
        {
          if (IsLegalPathInfo(tempPathInfo->pathNodeIdent,NICInfo[j].neighborIdent))
          {
            // 填写目的节点的信息
            tempPathInfo->nodeAddr.addr=NICInfo[j].localAddr;
            tempPathInfo->nodeAddr.prefixLen=NetmaskToPrefixlen(NICInfo[j].localMask);
            // 填写下一跳信息
            tempPathInfo->nextHopAddr.addr=NICInfo[j].localAddr;
            tempPathInfo->nextHopAddr.prefixLen=NetmaskToPrefixlen(NICInfo[j].localMask);
            usleep(100000);// 防止缓存满了
            m_udpClient.SendPathInfoTo(NICInfo[j].localAddr,NICInfo[j].neighborAddr,tempPathInfo);
          }
        }
      }
    }
    else// 探测到交换机，向邻居发送探测的这条链路，然后向该邻居发送所有的路径表项
    {
      // 探测到交换机，向邻居发送探测的这条链路
      tempPathInfo->nodeCounter++;
      tempPathInfo->pathNodeIdent[tempPathInfo->nodeCounter-1]=myIdent;

      tempPathInfo->nodeAddr.addr=tempNICInfo.localAddr;
      tempPathInfo->nodeAddr.prefixLen=NetmaskToPrefixlen(tempNICInfo.localMask);

      tempPathInfo->nextHopAddr.addr=tempNICInfo.localAddr;
      tempPathInfo->nextHopAddr.prefixLen=NetmaskToPrefixlen(tempNICInfo.localMask);

      usleep(100000);// 防止缓存满了
      m_udpClient.SendPathInfoTo(tempNICInfo.localAddr,tempNICInfo.neighborAddr,tempPathInfo);

      // 然后向该邻居发送所有的路径表项
      struct pathtableentry **headPathTableEntry=(struct pathtableentry **)malloc(sizeof(struct pathtableentry *));
      GetHeadPathTableEntry(headPathTableEntry);

      if ((*headPathTableEntry)->next!=NULL)
      {
        (*headPathTableEntry)=(*headPathTableEntry)->next;//获得第一条路径的位置
        while ((*headPathTableEntry)!=NULL)
        {
          struct pathtableentry tempPathTableEntry=**headPathTableEntry;
          if (IsLegalPathInfo(tempPathTableEntry.pathNodeIdent,tempNICInfo.neighborIdent))// 判断路径发送给该邻居是否合法
          {
            // 必须先初始化，否则会出现比如上一条路径有4个node，而这条只有2个node，却也会变成4个的错误
            for (int j=0;j<MAX_PATH_LEN;j++) tempPathInfo->pathNodeIdent[j]=tempNode;

            for (int j=0;j<MAX_ADDR_NUM;j++) 
            {
              tempPathInfo->addrSet[j].addr=tempAddr;
              tempPathInfo->addrSet[j].prefixLen=32;
            }

            for (int j=0,k=tempPathTableEntry.nodeCounter-1;k>=0;j++,k--)
            {
              tempPathInfo->pathNodeIdent[j]=tempPathTableEntry.pathNodeIdent[k];
            }

            tempPathInfo->nodeCounter=tempPathTableEntry.nodeCounter;
            tempPathInfo->nodeAddr.addr=tempPathTableEntry.nodeAddr.addr;
            tempPathInfo->nodeAddr.prefixLen=tempPathTableEntry.nodeAddr.prefixLen;

            tempPathInfo->nextHopAddr.addr=tempNICInfo.localAddr;
            tempPathInfo->nextHopAddr.prefixLen=NetmaskToPrefixlen(tempNICInfo.localMask);

            for (int j=0;j<MAX_ADDR_NUM;j++)
            {
              if (!strcmp(inet_ntoa(tempPathTableEntry.pathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255")) break;
              tempPathInfo->addrSet[j]=tempPathTableEntry.pathAddrSet->addrSet[j];
            }

            // Logfout << GetNow() << "The pathinfo size is " << sizeof(*tempPathInfo) << endl;
            usleep(100000);// 防止缓存满了
            m_udpClient.SendPathInfoTo(tempNICInfo.localAddr,tempNICInfo.neighborAddr,tempPathInfo);
          }
          (*headPathTableEntry)=(*headPathTableEntry)->next;
        }
      } 
      else 
      {
        Logfout << GetNow() << "Path table is empty!" << endl;
      }
    }
  }
  Logfout.close();
  pthread_mutex_unlock(&mutex);
}

void 
Ipv4GlobalRouting::FreshPathTable(struct pathinfo *tempPathInfo,struct sockaddr_in remote_addr)//收到了新的路径
{
  pthread_mutex_lock(&mutex);
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  Logfout << GetNow() << "Recv path [ ";
  for (int i=0;i<tempPathInfo->nodeCounter;i++)
  {
    Logfout << tempPathInfo->pathNodeIdent[i].level << "." << tempPathInfo->pathNodeIdent[i].position << "\t";
  } 
  for (int i=0;i<MAX_ADDR_NUM;i++)
  {
    if (!strcmp(inet_ntoa(tempPathInfo->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
    Logfout << "(" << inet_ntoa(tempPathInfo->addrSet[i].addr.sin_addr) << ")";
  }
  Logfout << "] from " << tempPathInfo->pathNodeIdent[tempPathInfo->nodeCounter-1].level << "." << tempPathInfo->pathNodeIdent[tempPathInfo->nodeCounter-1].position << "(" << inet_ntoa(remote_addr.sin_addr) << ")(" << sizeof(*tempPathInfo) << " B)." << endl;

  //添加本地结点
  tempPathInfo->nodeCounter++;
  tempPathInfo->pathNodeIdent[tempPathInfo->nodeCounter-1]=myIdent;

  AddNewPathTableEntry(*tempPathInfo);
 
  if (myIdent.level!=1)//tor收到路径后不再转发
  {
    for (int i=0;i<NICInfo.size();i++)
    {
      if (IsLegalNeighbor(NICInfo[i]))//判断邻居是否合法
      {
        if (IsLegalPathInfo(tempPathInfo->pathNodeIdent,NICInfo[i].neighborIdent))
        {
          tempPathInfo->nextHopAddr.addr=NICInfo[i].localAddr;
          tempPathInfo->nextHopAddr.prefixLen=NetmaskToPrefixlen(NICInfo[i].localMask);
          usleep(100000);// 防止缓存满了
          m_udpClient.SendPathInfoTo(NICInfo[i].localAddr,NICInfo[i].neighborAddr,tempPathInfo);
        }
      }
    } 
  }
  Logfout.close();
  pthread_mutex_unlock(&mutex);
}

bool
Ipv4GlobalRouting::IsNewNIC(struct ifaddrs *ifa)
{
  // 判断条件简单，还需要考虑其他属性，比如IP地址被修改了怎么办
  string tempName=ifa->ifa_name;
  bool isFindNIC=false;
  string localAddr=inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    if (NICInfo[i].NICName==tempName) // 此部分做日常故障检测用
    {
      NICInfo[i].flag=true;
      return false;//不是新卡
    }
    else if (NICInfo[i].NICName=="")//比如是NDRecvHello添加的表项，要继续完善
    {
      if (!strcmp(inet_ntoa(NICInfo[i].localAddr.sin_addr),localAddr.c_str()))
      {
        NICInfo[i].NICName=tempName;
        NICInfo[i].localMask=*((struct sockaddr_in *)(ifa->ifa_netmask));
        isFindNIC=true;
        return true;// 因为要发送ND
      }
    }
  }
  if (isFindNIC==false)// 不存在就添加
  {
    struct NICinfo tempNICInfo;
    if (ifa->ifa_name[0]=='e')
    {
      tempNICInfo.NICName=ifa->ifa_name;
      tempNICInfo.isMaster=true;
      tempNICInfo.isServer=false;
      tempNICInfo.isSwitch=false;
      tempNICInfo.flag=true;//up
      tempNICInfo.neighborIdent.level=0;
      tempNICInfo.neighborIdent.position=0;
      tempNICInfo.localAddr=*((struct sockaddr_in *)(ifa->ifa_addr));
      tempNICInfo.localMask=*((struct sockaddr_in *)(ifa->ifa_netmask));
      tempNICInfo.neighborAddr=*((struct sockaddr_in *)(ifa->ifa_addr));
      NICInfo.push_back(tempNICInfo);
      // 
    }
    else if (ifa->ifa_name[0]=='E')
    {
      tempNICInfo.NICName=ifa->ifa_name;
      tempNICInfo.isMaster=false;
      tempNICInfo.isServer=false;
      tempNICInfo.isSwitch=false;
      tempNICInfo.flag=true;//up
      tempNICInfo.neighborIdent.level=-1;
      tempNICInfo.neighborIdent.position=-1;
      tempNICInfo.localAddr=*((struct sockaddr_in *)(ifa->ifa_addr));
      tempNICInfo.localMask=*((struct sockaddr_in *)(ifa->ifa_netmask));

      string neighborAddr=inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr);
      neighborAddr[neighborAddr.size()-1]=(neighborAddr[neighborAddr.size()-1]=='1')?'2':'1';// 邻居的地址必须是理论上的那个
      tempNICInfo.neighborAddr=*((struct sockaddr_in *)(ifa->ifa_addr));
      tempNICInfo.neighborAddr.sin_addr.s_addr=inet_addr((char*)neighborAddr.c_str());
      NICInfo.push_back(tempNICInfo);
    }
    return true;
  }
}

void*
Ipv4GlobalRouting::ListenInterfaceThread(void* tempParam)
{
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamB *)tempParam)->tempGlobalRouting;
  UDPClient *tempUdpClient=((struct threadparamB *)tempParam)->tempUdpClient;
  TCPRoute *tempTcp=((struct threadparamB *)tempParam)->tempTcp;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->GetMyIdent().level << "." << tempGlobalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  Logfout << GetNow() << "Node ListenInterfaces And Submit......" << std::endl;

  struct ifaddrs *ifa;

  while (1)
  {
    // struct NICinfo temp;
    if (0!=getifaddrs(&ifa))
    {
      printf("getifaddrs error\n");
      break;
    }
    for (;ifa!=NULL;)
    {
      // // 虚拟机
      // if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && ifa->ifa_name[0]=='e' && ifa->ifa_name[3]!='0')
      // // Sonic
      if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && (ifa->ifa_name[0]=='E' || ifa->ifa_name[0]=='e'))// 交换机
      {
        // Logfout << "get NIC name is " << ifa->ifa_name << endl;
        if (tempGlobalRouting->isListenNIC(ifa->ifa_name) && tempGlobalRouting->IsNewNIC(ifa))//判断是否为未记录的网卡，是返回1
        {
          if (ifa->ifa_name[0]=='e')
          {
            Logfout << GetNow() << "New NIC " << ifa->ifa_name << "(" << inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr) << ") UP." << endl;
            // 应该先找出没有连接上的master，放后面再写
            if (tempGlobalRouting->nodeConMasterByNode.size()==0)// 说明没有master连上
            {
              string middleAddress="";
              tempTcp->SendHelloToMaster(tempGlobalRouting->masterAddress,middleAddress,ifa->ifa_name,MN_PORT);
            }
          }
          else if (ifa->ifa_name[0]=='E')
          {
            Logfout << GetNow() << "New NIC " << ifa->ifa_name << "(" << inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr) << ") UP." << endl;
            tempUdpClient->SendNDTo(*((struct sockaddr_in *)(ifa->ifa_addr)));
          }
        }
      }
      ifa=ifa->ifa_next;
    }
    freeifaddrs(ifa);

    for (auto iter=tempGlobalRouting->NICInfo.begin();iter!=tempGlobalRouting->NICInfo.end();)
    {
      if ((*iter).flag==false)// 网卡出故障了
      {
        Logfout << GetNow() << "NIC " << (*iter).NICName << "(" << inet_ntoa((*iter).localAddr.sin_addr) << ") DOWN." << endl; 
        // 判断是否要和master重连
        vector<string> tempMasterAddress;// 需要重联的master的地址
        for (auto tempIter=tempGlobalRouting->masterMapToSock.begin();tempIter!=tempGlobalRouting->masterMapToSock.end();)
        {
          if ((*iter).NICName==(*tempIter).NICName) // master需要重连
          {
            // 先关闭套接字
            tempMasterAddress.push_back((*tempIter).masterAddr);// 再记录需要重联的master地址
            tempIter=tempGlobalRouting->masterMapToSock.erase(tempIter);// 删除该项           
            if (tempIter==tempGlobalRouting->masterMapToSock.end()) break;
            continue;
          }
          tempIter++;
        }
        // 再重连
        if (tempMasterAddress.size()>0)
        {
          // 从间接路径里选一条
          for (int i=0;i<tempGlobalRouting->nodeConMasterByNode.size();i++)
          {
            string nextHopNICName=tempGlobalRouting->GetNICNameByRemoteAddr(tempGlobalRouting->nodeConMasterByNode[i]->nodeAddr.addr);
            if ((*iter).NICName!=nextHopNICName)// 下一跳不能是故障的链路的另一侧节点，此时还没来得及修改路径表
            {
              if (tempGlobalRouting->nodeConMasterByNode[i]->linkCounter==0 && tempGlobalRouting->nodeConMasterByNode[i]->dirConFlag==true)
              {
                ident tempIdent=tempGlobalRouting->nodeConMasterByNode[i]->pathNodeIdent[tempGlobalRouting->nodeConMasterByNode[i]->nodeCounter-1];
                int port=tempIdent.level*1000+tempIdent.position*100;// sonic test
                string middleAddress=inet_ntoa(tempGlobalRouting->nodeConMasterByNode[i]->nodeAddr.addr.sin_addr);
                tempTcp->SendHelloToMaster(tempMasterAddress,middleAddress,nextHopNICName,port);
                break;
              }
            }
          }
        }

        // 不需要或者重连完毕
        struct MNinfo tempMNInfo;
        // tempMNInfo.destIdent=tempGlobalRouting->myIdent;
        tempMNInfo.srcIdent=tempGlobalRouting->myIdent;
        tempMNInfo.pathNodeIdentA=tempGlobalRouting->myIdent;
        tempMNInfo.pathNodeIdentB=(*iter).neighborIdent;
        tempMNInfo.keepAlive=false;
        tempMNInfo.linkFlag=false;
        tempMNInfo.hello=false;
        tempMNInfo.ACK=false;

        for (int i=0;i<tempGlobalRouting->masterMapToSock.size();i++)
        {
          tempMNInfo.destIdent=tempGlobalRouting->masterMapToSock[i].masterIdent;
          tempTcp->SendMessageTo(tempGlobalRouting->masterMapToSock[i].masterSock,tempMNInfo);
        }
      
        iter=tempGlobalRouting->NICInfo.erase(iter);
        if (iter==tempGlobalRouting->NICInfo.end()) break;
        continue;
      }
      else if ((*iter).flag==true) (*iter).flag=false;
      iter++;
    }

    usleep(10000);
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::ListenInterfacesAndSubmit()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  pthread_t listenInterface_thread;
  struct threadparamB *threadParam=(struct threadparamB *)malloc(sizeof(struct threadparamB));
  threadParam->tempGlobalRouting=this;
  threadParam->tempUdpClient=&m_udpClient;
  threadParam->tempTcp=m_tcp;

  if(pthread_create(&listenInterface_thread,NULL,ListenInterfaceThread,(void*)threadParam)<0)
  {
    Logfout << GetNow() << "Create thread for ListenInterfacesAndSubmit failed!!!!!!!!!" << endl;
    exit(0);
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintPathEntry(struct pathtableentry *tempPathTableEntry)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  for (int i=0;i<tempPathTableEntry->nodeCounter;i++)
  {
    Logfout << tempPathTableEntry->pathNodeIdent[i].level << "." << tempPathTableEntry->pathNodeIdent[i].position << "\t";
  }
  Logfout << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintPathEntryTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/PathEntryTable-output-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  struct pathtableentry *iter= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
  iter=headPathTableEntry->next;
  Logfout << "Node" << myIdent.level << "." << myIdent.position << "'s pathentrytable size is " << headPathTableEntry->linkCounter << endl;
  Logfout << "Node\t" << "Node\t" << "Node\t" << "Node\t" << "Node\t";
  Logfout << "\t\tlinkCounter\t";
  Logfout << "dirConFlag\t";
  // Logfout << "weight\t";
  Logfout << endl;
  while (iter!=NULL)
  {
    for (int i=0;i<(*iter).nodeCounter;i++) Logfout << (*iter).pathNodeIdent[i].level << "." << (*iter).pathNodeIdent[i].position << "\t";
    for (int i=(*iter).nodeCounter;i<MAX_PATH_LEN;i++) Logfout << "\t";
    // // 节点地址
    if (strcmp(inet_ntoa((*iter).nodeAddr.addr.sin_addr),"255.255.255.255"))
    {
      Logfout << "[" << inet_ntoa((*iter).nodeAddr.addr.sin_addr) << "]";
    }
    // // 服务器地址
    for (int i=0;i<MAX_ADDR_NUM;i++)
    {
      if (!strcmp(inet_ntoa((*iter).pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
      Logfout << "(" << inet_ntoa((*iter).pathAddrSet->addrSet[i].addr.sin_addr) << ")";
    }
    Logfout << "\t" << (*iter).linkCounter;
    Logfout << "\t\t" << (*iter).dirConFlag;
    Logfout << endl;
    iter=iter->next;
  }
  Logfout << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintMappingTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/MappingTable-output-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  for (int i=0;i<mappingEntryTable.size();i++)
  {
    Logfout << mappingEntryTable[i].high.level << "." << mappingEntryTable[i].high.position << "--" << mappingEntryTable[i].low.level << "." << mappingEntryTable[i].low.position << "\t\t";
    struct pathtableentry *tempPathTableEntry=mappingEntryTable[i].address;
    for (int j=0;j<tempPathTableEntry->nodeCounter;j++)
    {
      Logfout << tempPathTableEntry->pathNodeIdent[j].level << "." << tempPathTableEntry->pathNodeIdent[j].position << "\t";
    } 
    Logfout << endl;
  }
  Logfout.close();
}

unsigned int 
Ipv4GlobalRouting::NetmaskToPrefixlen(struct sockaddr_in netMask)
{
  unsigned int prefixLen=0;
  unsigned long mask=netMask.sin_addr.s_addr;
  while (mask)
  {
    mask=mask/2;
    prefixLen++;
  }
  return prefixLen;
}

void
Ipv4GlobalRouting::GetHeadPathTableEntry(struct pathtableentry **tempPathEntry)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  *tempPathEntry=headPathTableEntry;

  Logfout.close();
}

void 
Ipv4GlobalRouting::GetMappingTableEntry(ident pathNodeIdentA,ident pathNodeIdentB,vector<struct mappingtableentry> *tempMappingTable)
{
  // 此处可优化，但我直接遍历了
  if (pathNodeIdentB.level!=-1 && pathNodeIdentB.level!=0)////根据两个节点(link)获取映射表项
  {
    ident high,low;
    if (pathNodeIdentA.level<pathNodeIdentB.level)
    {
      high=pathNodeIdentB;
      low=pathNodeIdentA;
    }
    else if (pathNodeIdentA.level>pathNodeIdentB.level)
    {
      high=pathNodeIdentA;
      low=pathNodeIdentB;
    }
    for (int i=0;i<mappingEntryTable.size();i++)
    {
      if (SameNode(high,mappingEntryTable[i].high) && SameNode(low,mappingEntryTable[i].low))
      {
        tempMappingTable->push_back(mappingEntryTable[i]);
        break;
      }
    }
  }
  else// 按照单个结点来获取映射表项
  {
    for (int i=0;i<mappingEntryTable.size();i++)
    {
      if (SameNode(pathNodeIdentA,mappingEntryTable[i].high) || SameNode(pathNodeIdentA,mappingEntryTable[i].low))
      {
        tempMappingTable->push_back(mappingEntryTable[i]);
      }
    }
  }
}

unsigned long 
Ipv4GlobalRouting::Convert(struct sockaddr_in destAddr,unsigned int prefixLen)
{
  unsigned long netmask=0;
  unsigned long netaddress=0;
  netmask = (~0) << (32-prefixLen);
  netmask = htonl(netmask);
  netaddress = destAddr.sin_addr.s_addr & netmask;
  
  return netaddress;
}

void
Ipv4GlobalRouting::AddSingleRoute(struct sockaddr_in destAddr,unsigned int prefixLen,struct nexthopandweight tempNextHopAndWeight)
{  
  // ofstream Logfout("/var/log/Primus.log",ios::app);
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  // test
  // Logfout << "++++++++++++++++" << endl;
  // Logfout << "destAddr:" << inet_ntoa(destAddr.sin_addr) << ",prefixLen:" << prefixLen << ",NICName:" << NICName << endl;
  // endl
  struct {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[NL_PKT_BUF_SIZE];
  } req;
  
  int if_index=if_nametoindex(tempNextHopAndWeight.NICName.c_str());    //接口名称转索引
  int rt_sock=OpenNetlink();
  memset(&req,0,sizeof(req));

  req.n.nlmsg_len=NLMSG_LENGTH(sizeof(struct rtmsg));//
  req.n.nlmsg_flags=NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE;//
  req.n.nlmsg_type=RTM_NEWROUTE;//
  req.r.rtm_family=AF_INET;//
  req.r.rtm_table=RT_TABLE_MAIN;//
  req.r.rtm_protocol=RTPROT_ZEBRA;//
  req.r.rtm_scope=RT_SCOPE_UNIVERSE;//
  req.r.rtm_type=RTN_UNICAST;//>?
  req.r.rtm_dst_len=prefixLen;//
  
  // mtu没设置
  int bytelen=(req.r.rtm_family==AF_INET)?4:16;

  destAddr.sin_addr.s_addr=Convert(destAddr,prefixLen);
  addattr_l(&req.n,sizeof(req),RTA_DST,&(destAddr.sin_addr.s_addr),bytelen);//目的地址
  addattr32(&req.n,sizeof(req),RTA_PRIORITY,NL_DEFAULT_ROUTE_METRIC);//metric
  // addattr_l(&req.n,sizeof(req),RTA_GATEWAY,&(tempNextHopAndWeight.gateAddr.sin_addr.s_addr),bytelen);//网关，单路径好像不要加上网关
  addattr_l(&req.n,sizeof(req),RTA_PREFSRC,&(tempNextHopAndWeight.srcAddr.sin_addr.s_addr),bytelen);//src
  addattr_l(&req.n,sizeof(req),RTA_OIF,&if_index,bytelen);//该路由项的输出网络设备索引
     
  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  close(rt_sock);
  // Logfout.close();
}

void
Ipv4GlobalRouting::AddMultiRoute(struct sockaddr_in destAddr,unsigned int prefixLen,vector<struct nexthopandweight> nextHopAndWeight)
{    
  // ofstream Logfout("/var/log/Primus.log",ios::app);
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  // test
  // Logfout << "++++++++++++++++" << endl;
  // Logfout << "destAddr:" << inet_ntoa(destAddr.sin_addr) << ",prefixLen:" << prefixLen << endl;
  // for (int i=0;i<nextHopAndWeight.size();i++)
  // {
  //   Logfout << "NICName:" << nextHopAndWeight[i].NICName << " weight:" << nextHopAndWeight[i].weight << endl;
  // }
  // endl
  struct {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[NL_PKT_BUF_SIZE];
  } req;

  memset(&req,0,sizeof(req));
  
  req.n.nlmsg_len=NLMSG_LENGTH(sizeof(struct rtmsg));//
  req.n.nlmsg_flags=NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE;//
  req.n.nlmsg_type=RTM_NEWROUTE;//
  req.r.rtm_family=AF_INET;//
  req.r.rtm_table=RT_TABLE_MAIN;//
  req.r.rtm_protocol=RTPROT_ZEBRA;//
  req.r.rtm_scope=RT_SCOPE_UNIVERSE;//
  req.r.rtm_type=RTN_UNICAST;
  req.r.rtm_dst_len=prefixLen;// 前缀长度
  
  //内核通信数据包attribute构建
  destAddr.sin_addr.s_addr=Convert(destAddr,prefixLen);

  int bytelen=(req.r.rtm_family==AF_INET)?4:16;
  addattr_l(&req.n,sizeof(req),RTA_DST,&(destAddr.sin_addr.s_addr),bytelen);
  addattr32(&req.n,sizeof(req),RTA_PRIORITY,NL_DEFAULT_ROUTE_METRIC);

  char buf[NL_PKT_BUF_SIZE];
  struct rtattr *rta=(struct rtattr *)((void *)buf);
  struct rtnexthop *rtnh;
  
  rta->rta_type=RTA_MULTIPATH;
  rta->rta_len=RTA_LENGTH(0);
  rtnh=(struct rtnexthop *)RTA_DATA(rta);

  for (int i=0;i<nextHopAndWeight.size();i++)
  {
    rtnh->rtnh_len=sizeof(struct rtnexthop);
    rtnh->rtnh_flags=0;//RTNH_F_ONLINK;
    rtnh->rtnh_hops=nextHopAndWeight[i].weight-1;//系统自动加1？
    rta->rta_len+=rtnh->rtnh_len;
    rta_addattr_l(rta,NL_PKT_BUF_SIZE,RTA_GATEWAY,&(nextHopAndWeight[i].gateAddr.sin_addr.s_addr),bytelen);
    rtnh->rtnh_len+=sizeof(struct rtattr)+bytelen;
    rtnh->rtnh_ifindex=if_nametoindex((nextHopAndWeight[i].NICName).c_str());  
    rtnh=RTNH_NEXT(rtnh);
  }
  
  addattr_l(&req.n,NL_PKT_BUF_SIZE,RTA_MULTIPATH,RTA_DATA(rta),RTA_PAYLOAD(rta));
  int rt_sock=OpenNetlink();
  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  close(rt_sock);
  // Logfout.close();
}

void 
Ipv4GlobalRouting::DelRoute(struct sockaddr_in destAddr,unsigned int prefixLen)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  struct {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[NL_PKT_BUF_SIZE];
  } req;
  
  int rt_sock=OpenNetlink();
  memset(&req,0,sizeof(req));
  req.n.nlmsg_len=NLMSG_LENGTH(sizeof(struct rtmsg));
  req.n.nlmsg_flags=NLM_F_REQUEST | 0;
  req.n.nlmsg_type=RTM_DELROUTE;
  req.r.rtm_family=AF_INET;
  req.r.rtm_table=RT_TABLE_MAIN;
  req.r.rtm_protocol=RTPROT_ZEBRA;
  req.r.rtm_scope=RT_SCOPE_UNIVERSE;
  req.r.rtm_type=RTN_UNICAST;
  req.r.rtm_dst_len=prefixLen;

  destAddr.sin_addr.s_addr=Convert(destAddr,prefixLen);
  addattr_l(&req.n,sizeof(req),RTA_DST,&(destAddr.sin_addr.s_addr),4);
  
  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  close(rt_sock);
  Logfout << GetNow() << "DelRoute--------------" << endl;
  Logfout << GetNow() << inet_ntoa(destAddr.sin_addr) << "/" << prefixLen << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateAddrSet(ident pathNodeIdentA,ident pathNodeIdentB,int nodeCounter,vector<struct addrset> addrSet)//A是目的结点，B是倒数第二个结点
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  vector<struct nexthopandweight> nextHopAndWeight;
  struct pathtableentry *tempPathTableEntry;
  string NICName;

  // 首先添加到该节点的路由，在映射表中找出pathnode[nodecounter-2]--pathnode[nodecounter-1]的映射表项

  vector<struct mappingtableentry> tempMappingTable;
  GetMappingTableEntry(pathNodeIdentA,pathNodeIdentB,&tempMappingTable);
  if (pathNodeIdentB.level==-1 || pathNodeIdentB.level==0)//和某个结点相关的映射表都要考虑
  {
    for (int i=0;i<tempMappingTable.size();i++)
    {
      tempPathTableEntry=tempMappingTable[i].address;
      while (1)
      {
        bool isFindNextHopAndWeight=false;
        if (nodeCounter==tempPathTableEntry->nodeCounter && tempPathTableEntry->linkCounter==0)// 两条路径长度必须相等，且linkCounter为0，0表示故障链路数为0
        {
          // 最后一个结点也必须相同
          if (SameNode(pathNodeIdentA,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]))
          {
            while (1)
            {
              NICName=GetNICNameByRemoteAddr(tempPathTableEntry->nextHopAddr.addr);
              if (NICName=="")// 还没有探测到这个邻居
              {
                Logfout << GetNow() << "!!!!!!!!!!!!!!" << endl;
                usleep(100000);//休眠100ms
              }
              else break;//否则继续进行
            }
            for (int j=0;j<nextHopAndWeight.size();j++)// 此结构体用来存储下一跳和相应的权重信息，先遍历，没有找到再添加
            {
              if (nextHopAndWeight[j].NICName==NICName)// 找了，权重+1
              {
                isFindNextHopAndWeight=true;
                nextHopAndWeight[j].weight++;
                break;
              }
            }
            if (!isFindNextHopAndWeight)//没有找到相应的条目
            {
              struct nexthopandweight tempNextHopAndWeight;
              tempNextHopAndWeight.NICName=NICName;
              GetLocalAddrByRemoteAddr(&(tempNextHopAndWeight.srcAddr),tempPathTableEntry->nextHopAddr.addr);
              tempNextHopAndWeight.gateAddr=tempPathTableEntry->nextHopAddr.addr;
              tempNextHopAndWeight.weight=1;
              nextHopAndWeight.push_back(tempNextHopAndWeight);
            }
            if (tempPathTableEntry->next==NULL) break;
            else tempPathTableEntry=tempPathTableEntry->next;
          }
          else break;// 长度相等，最后目的节点不同，直接退出
        }
        else break;// 长度不相等，直接退出
      }
    }
  }
  else // 找出和一条链路pathNodeIdentA--pathNodeIdentB相关的映射表项，这两个node应该是路径的最后两个node
  {
    for (int i=0;i<tempMappingTable.size();i++)
    {
      tempPathTableEntry=tempMappingTable[i].address;
      while (1)
      {
        bool isFindNextHopAndWeight=false;
        // 两条路径长度必须相等，且linkCounter为0，0表示故障链路数为0
        if (nodeCounter==tempPathTableEntry->nodeCounter && tempPathTableEntry->linkCounter==0)
        {
          // 最后两个结点也必须相同
          if (SameNode(pathNodeIdentA,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]))
          {
            if (SameNode(pathNodeIdentB,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-2]))
            {
              while (1)
              {
                NICName=GetNICNameByRemoteAddr(tempPathTableEntry->nextHopAddr.addr);
                if (NICName=="")// 还没有探测到这个邻居
                {
                  Logfout << GetNow() << "!!!!!!!!!!!!!!" << endl;
                  usleep(100000);//休眠100ms
                }
                else break;//否则继续进行
              }
              for (int j=0;j<nextHopAndWeight.size();j++)
              {
                if (nextHopAndWeight[j].NICName==NICName)//
                {
                  isFindNextHopAndWeight=true;
                  nextHopAndWeight[j].weight++;
                  break;
                }
              }
              if (!isFindNextHopAndWeight)//没有找到相应的条目
              {
                struct nexthopandweight tempNextHopAndWeight;
                tempNextHopAndWeight.NICName=NICName;
                GetLocalAddrByRemoteAddr(&(tempNextHopAndWeight.srcAddr),tempPathTableEntry->nextHopAddr.addr);
                tempNextHopAndWeight.gateAddr=tempPathTableEntry->nextHopAddr.addr;
                tempNextHopAndWeight.weight=1;
                nextHopAndWeight.push_back(tempNextHopAndWeight);
              }
              if (tempPathTableEntry->next==NULL) break;
              else tempPathTableEntry=tempPathTableEntry->next;
            }
            else break;
          }
          else break;
        }
        else break;
      }
    }
  } 

  // 更新路由
  if (nextHopAndWeight.size()==0)
  {
    for (int i=0;i<addrSet.size();i++) DelRoute(addrSet[i].addr,addrSet[i].prefixLen);
  }
  else if (nextHopAndWeight.size()==1)
  {
    for (int i=0;i<addrSet.size();i++) AddSingleRoute(addrSet[i].addr,addrSet[i].prefixLen,nextHopAndWeight[0]); 
  }
  else if (nextHopAndWeight.size()>1)
  {
    for (int i=0;i<addrSet.size();i++) AddMultiRoute(addrSet[i].addr,addrSet[i].prefixLen,nextHopAndWeight);// 添加多路径路由
  }
  // 添加完
  nextHopAndWeight.clear();
  // Logfout << endl << "add route over" << endl << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateMappingTableEntry(struct pathtableentry *newPathTableEntry)
{
  // 链路都有可能是第一次出现
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  ident high,low;
  
  for (int i=0;i<newPathTableEntry->nodeCounter-1;i++)
  {
    if (newPathTableEntry->pathNodeIdent[i].level<newPathTableEntry->pathNodeIdent[i+1].level)
    {
      high=newPathTableEntry->pathNodeIdent[i+1];
      low=newPathTableEntry->pathNodeIdent[i];
    }
    else if (newPathTableEntry->pathNodeIdent[i].level>newPathTableEntry->pathNodeIdent[i+1].level)
    {
      high=newPathTableEntry->pathNodeIdent[i];
      low=newPathTableEntry->pathNodeIdent[i+1];
    }

    bool isFindMapping=false;
    for (int j=0;j<mappingEntryTable.size();j++)
    {
      if (SameNode(high,mappingEntryTable[j].high) && SameNode(low,mappingEntryTable[j].low))//找到了该映射表项，此处可以优化，缩小查找范围
      {
        isFindMapping=true;
        // 首先判断两条路径的长度
        if (newPathTableEntry->nodeCounter<mappingEntryTable[j].address->nodeCounter)//一定要更新
        {
          mappingEntryTable[j].address=newPathTableEntry;
          break;
        }
        else if (newPathTableEntry->nodeCounter==mappingEntryTable[j].address->nodeCounter)
        {
          // 长度相等，则继续比较其他的结点
          // 先从前面开始比较
          bool isNeedCompareAgain=true;
          ident tempNewPathEntryIdent,tempMapPathEntryIdent;
          for (int k=1;k<i;k++)
          {
            tempNewPathEntryIdent=newPathTableEntry->pathNodeIdent[k];
            tempMapPathEntryIdent=mappingEntryTable[j].address->pathNodeIdent[k];

            if (tempNewPathEntryIdent.level<tempMapPathEntryIdent.level)//一定要更新
            {
              mappingEntryTable[j].address=newPathTableEntry;
              isNeedCompareAgain=false;
              break;
            }
            else if (tempNewPathEntryIdent.level==tempMapPathEntryIdent.level)
            {
              // 继续比较position
              if (tempNewPathEntryIdent.position<tempMapPathEntryIdent.position)//一定要更新
              {
                mappingEntryTable[j].address=newPathTableEntry;
                isNeedCompareAgain=false;
                break;
              }
              else if (tempNewPathEntryIdent.position==tempMapPathEntryIdent.position)//
              {
                continue;
              }
              else if (tempNewPathEntryIdent.position>tempMapPathEntryIdent.position)//一定不要更新
              {
                isNeedCompareAgain=false;
                break;
              }
            }
            else if (tempNewPathEntryIdent.level>tempMapPathEntryIdent.level)//一定不要更新
            {
              isNeedCompareAgain=false;
              break;
            }
          }
          // 前面没有比出结果来
          if (isNeedCompareAgain)
          {
            ident tempNewPathEntryIdent,tempMapPathEntryIdent;
            for (int k=i+2;k<newPathTableEntry->nodeCounter;k++)
            {
              tempNewPathEntryIdent=newPathTableEntry->pathNodeIdent[k];
              tempMapPathEntryIdent=mappingEntryTable[j].address->pathNodeIdent[k];

              if (tempNewPathEntryIdent.level<tempMapPathEntryIdent.level)//一定要更新
              {
                mappingEntryTable[j].address=newPathTableEntry;
                break;
              }
              else if (tempNewPathEntryIdent.level==tempMapPathEntryIdent.level)
              {
                // 继续比较position
                if (tempNewPathEntryIdent.position<tempMapPathEntryIdent.position)//一定要更新
                {
                  mappingEntryTable[j].address=newPathTableEntry;
                  break;
                }
                else if (tempNewPathEntryIdent.position==tempMapPathEntryIdent.position)//
                {
                  continue;
                }
                else if (tempNewPathEntryIdent.position>tempMapPathEntryIdent.position)//一定不要更新
                {
                  break;
                }
              }
              else if (tempNewPathEntryIdent.level>tempMapPathEntryIdent.level)//一定不要更新
              {
                break;
              }
            }
          }
        }
        else if (newPathTableEntry->nodeCounter>mappingEntryTable[j].address->nodeCounter)//一定不要更新
        {
          break;
        }
        break;
      }
    }
    if (!isFindMapping)//映射表里没有，则添加
    {
      struct mappingtableentry mappingTableEntry;
      mappingTableEntry.high=high;
      mappingTableEntry.low=low;
      mappingTableEntry.address=newPathTableEntry;
      mappingEntryTable.push_back(mappingTableEntry);
    }
  }

  // 排序
  struct mappingtableentry mappingTemp;
  for (int i=0;i<mappingEntryTable.size()-1;i++)
  {
    for (int j=0;j<mappingEntryTable.size()-1-i;j++)
    {
      if (mappingEntryTable[j+1].high.level<mappingEntryTable[j].high.level)//底层链路排在前
      {
        mappingTemp=mappingEntryTable[j+1];
        mappingEntryTable[j+1]=mappingEntryTable[j];
        mappingEntryTable[j]=mappingTemp;
      }
      else if (mappingEntryTable[j+1].high.level==mappingEntryTable[j].high.level)//若level相等
      {
        if (mappingEntryTable[j+1].high.position<mappingEntryTable[j].high.position)//position按升序排列
        {
          mappingTemp=mappingEntryTable[j+1];
          mappingEntryTable[j+1]=mappingEntryTable[j];
          mappingEntryTable[j]=mappingTemp;
        }
      }
    }
  }
  PrintMappingTable();
  Logfout.close();
}

void
Ipv4GlobalRouting::InsertPathTable(struct pathtableentry *currentPathTableEntry,struct pathtableentry *nextPathTableEntry,struct pathtableentry *newPathTableEntry)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  // Logfout << GetNow() << "InsertPathTable " << endl;

  // 更新映射表
  if (newPathTableEntry->nodeCounter!=1) UpdateMappingTableEntry(newPathTableEntry);
  
  // Logfout << GetNow() << "UpdateMappingTableEntry over" << endl << endl;

  // 插入路径表
  currentPathTableEntry->next=newPathTableEntry;
  newPathTableEntry->next=nextPathTableEntry;
  headPathTableEntry->linkCounter++;
  UpdateNodeConMasterByNode(newPathTableEntry);
  
  // Logfout << GetNow() << "InsertPathTable over" << endl << endl;
  // Logfout << endl << "UpdateForward" << endl;

  // tor下可能会有多个服务器地址，且同一个tor会在路径表内重复出现，所以让重复的tor指向同一个地址，节约空间
  // 找一个相同的目的节点，让newPathTableEntry->pathAddrSet=相同目的地址的pathAddrSet
  if (newPathTableEntry->nodeCounter==1)// tor发现了新的服务器，且之前没有这样的路径，所以不需要遍历路径表
  {
    for (int i=0;i<MAX_ADDR_NUM;i++)
    {
      if (!strcmp(inet_ntoa(newPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) 
      {
        vector<struct nexthopandweight> nextHopAndWeight;
        struct nexthopandweight tempNextHopAndWeight;
        tempNextHopAndWeight.NICName=GetNICNameByRemoteAddr(newPathTableEntry->pathAddrSet->addrSet[i].addr); 
        GetLocalAddrByRemoteAddr(&(tempNextHopAndWeight.srcAddr),newPathTableEntry->pathAddrSet->addrSet[i].addr);
        tempNextHopAndWeight.gateAddr=newPathTableEntry->pathAddrSet->addrSet[i].addr;
        tempNextHopAndWeight.weight=1;
        nextHopAndWeight.push_back(tempNextHopAndWeight);
        AddSingleRoute(newPathTableEntry->pathAddrSet->addrSet[i].addr,newPathTableEntry->pathAddrSet->addrSet[i].prefixLen,nextHopAndWeight[0]);
      }
      else break;
    }
  }
  else
  {
    bool isfind=false;
    struct pathtableentry *tempPathTableEntry=headPathTableEntry->next;
    while (tempPathTableEntry!=NULL)// 遍历整个路径表
    {
      //相同的目的结点
      if (tempPathTableEntry!=newPathTableEntry && SameNode(newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-1],tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]))
      {
        isfind=true;
        struct pathaddrset *tempPathAddrSet=tempPathTableEntry->pathAddrSet;
        // 把新的addrset中存在，已有的addrset中不存在的地址添加进去
        for (int i=0;i<MAX_ADDR_NUM;i++)// 遍历整个新的addrset
        {
          if (strcmp(inet_ntoa(newPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255"))//新的addrSet中找出一个有效地址
          {
            string tempAddr=inet_ntoa(newPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr);
            for (int j=0;j<MAX_ADDR_NUM;j++)// 遍历整个已有的addrset
            {
              if (!strcmp(inet_ntoa(tempPathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255"))//此处可插入
              {
                tempPathAddrSet->addrSet[j]=newPathTableEntry->pathAddrSet->addrSet[i];
                vector<struct addrset> addrSet;// 新添加的地址都加入其中，用来添加路由
                addrSet.push_back(newPathTableEntry->pathAddrSet->addrSet[i]);
                ident tempIdent;
                tempIdent.level=-1;
                tempIdent.position=-1;
                UpdateAddrSet(tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1],tempIdent,tempPathTableEntry->nodeCounter,addrSet);
                break;// 已经插入已有的addrset，无需再继续遍历
              }
              // 相同的地址直接退出
              if (!strcmp(inet_ntoa(tempPathAddrSet->addrSet[i].addr.sin_addr),tempAddr.c_str())) break;
            }
          }
          else break;
        }
        newPathTableEntry->pathAddrSet=tempPathTableEntry->pathAddrSet;// 指向同一个地址
        break;
      }
      // 后移
      tempPathTableEntry=tempPathTableEntry->next;
    }
    if (isfind==false)// 没有找到这样的目的节点
    {
      for (int i=0;i<MAX_ADDR_NUM;i++)
      {
        if (strcmp(inet_ntoa(newPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255"))// 一个有效的地址
        {
          vector<struct addrset> addrSet;// 新添加的地址都加入其中，用来添加路由
          addrSet.push_back(newPathTableEntry->pathAddrSet->addrSet[i]);
          ident tempIdent;
          tempIdent.level=-1;
          tempIdent.position=-1;
          UpdateAddrSet(tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1],tempIdent,tempPathTableEntry->nodeCounter,addrSet);
        }
        else break;
      }
    }

    // 最后，还要添加到目的节点的路由
    vector<struct addrset> addrSet;
    addrSet.push_back(newPathTableEntry->nodeAddr);
    UpdateAddrSet(newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-1],newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2],newPathTableEntry->nodeCounter,addrSet);
  }
  
  // 完

  // // 更新转发表
  // Logfout << "UpdateForward over" << endl << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::PathAddNewAddr(struct pathtableentry *nextPathTableEntry,struct pathaddrset *tempPathAddrSet)
{
  // ofstream Logfout("/var/log/Primus.log",ios::app);
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  // Logfout << GetNow() << "PathAddNewAddr" << endl;
  if (nextPathTableEntry->nodeCounter==1)// 本机（也是一个tor）发现了新的服务器
  {
    for (int i=0;i<MAX_ADDR_NUM;i++)
    {
      if (!strcmp(inet_ntoa(tempPathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) // 在新的addrset中添加新的有效的服务器地址
      {
        // Logfout << inet_ntoa(tempPathAddrSet->addrSet[i].addr.sin_addr) << " is new addr" << endl;
        vector<struct nexthopandweight> nextHopAndWeight;
        struct nexthopandweight tempNextHopAndWeight;
        tempNextHopAndWeight.NICName=GetNICNameByRemoteAddr(tempPathAddrSet->addrSet[i].addr); 
        GetLocalAddrByRemoteAddr(&(tempNextHopAndWeight.srcAddr),tempPathAddrSet->addrSet[i].addr);
        tempNextHopAndWeight.gateAddr=tempPathAddrSet->addrSet[i].addr;
        tempNextHopAndWeight.weight=1;
        nextHopAndWeight.push_back(tempNextHopAndWeight);
        AddSingleRoute(tempPathAddrSet->addrSet[i].addr,tempPathAddrSet->addrSet[i].prefixLen,nextHopAndWeight[0]);
      }
      else break;
    }
  }
  else// 非本机的tor发现了新的服务器
  {
    struct pathaddrset *nextPathAddrSet=nextPathTableEntry->pathAddrSet;
    for (int i=0;i<MAX_ADDR_NUM;i++)
    {
      if (strcmp(inet_ntoa(tempPathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255"))//新的addrSet中找出一个有效地址
      {
        string tempAddr=inet_ntoa(tempPathAddrSet->addrSet[i].addr.sin_addr);
        for (int j=0;j<MAX_ADDR_NUM;j++)// 遍历整个next
        {
          if (!strcmp(inet_ntoa(nextPathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255"))
          {
            // Logfout << tempAddr << " is new addr" << endl;
            nextPathAddrSet->addrSet[j]=tempPathAddrSet->addrSet[i];
            vector<struct addrset> addrSet;
            addrSet.push_back(tempPathAddrSet->addrSet[i]);
            ident tempIdent;
            tempIdent.level=-1;
            tempIdent.position=-1;
            UpdateAddrSet(nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-1],tempIdent,nextPathTableEntry->nodeCounter,addrSet);
            break;
          }
          //相同的地址直接退出
          if (!strcmp(inet_ntoa(nextPathAddrSet->addrSet[i].addr.sin_addr),tempAddr.c_str())) break;
        }
      }
      else break;
    }
  }
  // Logfout << GetNow() << "PathAddNewAddr over" << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::AddNewPathTableEntry(struct pathinfo tempPathInfo)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  // Logfout << GetNow() << "AddNewPathTableEntry" << endl;
  
  struct pathtableentry *currentPathTableEntry=headPathTableEntry;
  struct pathtableentry *nextPathTableEntry=headPathTableEntry->next;
  struct pathtableentry *newPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));

  newPathTableEntry->pathAddrSet=NULL;
  newPathTableEntry->dirConFlag=tempPathInfo.dirConFlag;
  newPathTableEntry->linkCounter=0;
  newPathTableEntry->nodeCounter=0;
  newPathTableEntry->next=NULL;

  ident tempNode;
  tempNode.level=-1;
  tempNode.position=-1;

  for (int j=0;j<MAX_PATH_LEN;j++) newPathTableEntry->pathNodeIdent[j]=tempNode;

  for (int i=0,j=tempPathInfo.nodeCounter-1;j>=0;i++,j--)
  {
    newPathTableEntry->pathNodeIdent[i]=tempPathInfo.pathNodeIdent[j];
    newPathTableEntry->nodeCounter++;
  }

  newPathTableEntry->nodeAddr=tempPathInfo.nodeAddr;
  newPathTableEntry->nextHopAddr=tempPathInfo.nextHopAddr;

  struct pathaddrset *tempPathAddrSet=(struct pathaddrset *)malloc(sizeof(struct pathaddrset));
  for (int i=0;i<MAX_ADDR_NUM;i++)
  {
    tempPathAddrSet->addrSet[i]=tempPathInfo.addrSet[i];
  }

  newPathTableEntry->pathAddrSet=tempPathAddrSet;

  // test
  Logfout << GetNow() << "New path [ ";
  for (int i=0;i<newPathTableEntry->nodeCounter;i++)
  {
    Logfout << newPathTableEntry->pathNodeIdent[i].level << "." << newPathTableEntry->pathNodeIdent[i].position << "\t";
  }
  
  Logfout << "serverAddr:";
  for (int i=0;i<MAX_ADDR_NUM;i++)
  {
    if (!strcmp(inet_ntoa(newPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
    Logfout << "(" << inet_ntoa(newPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr) << ")";
  }
  Logfout << " ]." << endl;
  // end

  ident nextPathEntryDestIdent,newPathEntryDestIdent,tempNewPathEntryIdent,tempNextPathEntryIdent;
  newPathEntryDestIdent=newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-1];

  bool isInsert=false;
  // 方法 1
  // 1、路径越短，排序越靠前
  // 2、level越小，排序越靠前,在此条件下，position越小，排序越靠前
  // 3、相同下一跳的排列在一起
  // 4、条件3下，相同的目的地址排列在一起
  // 5、条件4下，比较目的节点和源节点中间的结点（按条件2）

  if (myIdent.level==1 || myIdent.level==3)
  {
    do
    {
      if (newPathTableEntry->nodeCounter==1)
      {
        if (nextPathTableEntry!=NULL && nextPathTableEntry->nodeCounter==1); // PathAddNewAddr(nextPathTableEntry,newPathTableEntry->pathAddrSet);//找到了一条完全相同的路径
        else InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
        isInsert=true;
        break;
      }
      else if (nextPathTableEntry!=NULL)
      {
        nextPathEntryDestIdent=nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-1];
        if (newPathTableEntry->pathNodeIdent[1].level<nextPathTableEntry->pathNodeIdent[1].level)
        {
          InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
          isInsert=true;
          break;
        }
        else if (newPathTableEntry->pathNodeIdent[1].level==nextPathTableEntry->pathNodeIdent[1].level)
        {
          if (newPathTableEntry->pathNodeIdent[1].position<nextPathTableEntry->pathNodeIdent[1].position)
          {
            InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
            isInsert=true;
            break;
          }
          else if (newPathTableEntry->pathNodeIdent[1].position==nextPathTableEntry->pathNodeIdent[1].position)
          {
            // 再比较路径长度，越短越靠前
            if (newPathTableEntry->nodeCounter<nextPathTableEntry->nodeCounter)
            {
              InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
              isInsert=true;
              break;
            }
            else if (newPathTableEntry->nodeCounter==nextPathTableEntry->nodeCounter)
            {
              // 先比较目的节点的level，再比较position，如果都相等，继续比较下一跳和目的结点间的其他结点
              if (newPathEntryDestIdent.level<nextPathEntryDestIdent.level)//直接插入
              {
                InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                isInsert=true;
                break;
              }
              else if (newPathEntryDestIdent.level==nextPathEntryDestIdent.level)//
              {
                if (newPathEntryDestIdent.position<nextPathEntryDestIdent.position)//position更小，直接插入
                {
                  InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                  isInsert=true;
                  break;
                }
                else if (newPathEntryDestIdent.position==nextPathEntryDestIdent.position)//目的节点相同，则比较下一跳到目的节点间的中间结点
                {
                  int sameCounter=0;
                  for (int i=2;i<newPathTableEntry->nodeCounter-1;i++)//貌似这个时候长度已经相等了，只要一个判断条件就可以了
                  {
                    tempNewPathEntryIdent=newPathTableEntry->pathNodeIdent[i];
                    tempNextPathEntryIdent=nextPathTableEntry->pathNodeIdent[i];
                    if (tempNewPathEntryIdent.level<tempNextPathEntryIdent.level)//
                    {
                      InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                      isInsert=true;
                      break;
                    }
                    else if (tempNewPathEntryIdent.level==tempNextPathEntryIdent.level)
                    {
                      if (tempNewPathEntryIdent.position<tempNextPathEntryIdent.position)
                      {
                        InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                        isInsert=true;
                        break;
                      }
                      else if (tempNewPathEntryIdent.position==tempNextPathEntryIdent.position)
                      {
                        // Logfout << "两条路径可能相同" << endl;
                        sameCounter++;
                        continue;
                      }
                      else if (tempNewPathEntryIdent.position>tempNextPathEntryIdent.position)
                      {
                        currentPathTableEntry=currentPathTableEntry->next;
                        nextPathTableEntry=nextPathTableEntry->next;
                        break;
                      }
                    }
                    else if (tempNewPathEntryIdent.level>tempNextPathEntryIdent.level)
                    {
                      currentPathTableEntry=currentPathTableEntry->next;
                      nextPathTableEntry=nextPathTableEntry->next;
                      break;
                    }
                  }
                  if (isInsert==false && sameCounter>=newPathTableEntry->nodeCounter-1-2)//出现sameCounter小于的情况是，一条路径可能只有2跳，则sameCounter=0，((newPathTableEntry->pathEntry.size()+1)/2-2)=-1.
                  {
                    PathAddNewAddr(nextPathTableEntry,newPathTableEntry->pathAddrSet);//找到了一条完全相同的路径
                    isInsert=true;
                    break;
                  }
                }
                else if (newPathEntryDestIdent.position>nextPathEntryDestIdent.position)//
                {
                  currentPathTableEntry=currentPathTableEntry->next;
                  nextPathTableEntry=nextPathTableEntry->next;
                }
              }
              else if (newPathEntryDestIdent.level>nextPathEntryDestIdent.level)//后移
              {
                currentPathTableEntry=currentPathTableEntry->next;
                nextPathTableEntry=nextPathTableEntry->next;
              }
            }
            else if (newPathTableEntry->nodeCounter>nextPathTableEntry->nodeCounter)
            {
              currentPathTableEntry=currentPathTableEntry->next;
              nextPathTableEntry=nextPathTableEntry->next;
            }
          }
          else if (newPathTableEntry->pathNodeIdent[1].position>nextPathTableEntry->pathNodeIdent[1].position)
          {
            currentPathTableEntry=currentPathTableEntry->next;
            nextPathTableEntry=nextPathTableEntry->next;
          }
        }
        else if (newPathTableEntry->pathNodeIdent[1].level>nextPathTableEntry->pathNodeIdent[1].level)
        {
          currentPathTableEntry=currentPathTableEntry->next;
          nextPathTableEntry=nextPathTableEntry->next;
        }
      }
      if (nextPathTableEntry==NULL && isInsert==false)
      {
        InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
        isInsert=true;
        break;
      }
      if (isInsert) break;
    }while (nextPathTableEntry!=NULL);
  }
  else if (myIdent.level==2)
  {
    do 
    {
      if (nextPathTableEntry!=NULL)
      {
        nextPathEntryDestIdent=nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-1];
        if (newPathTableEntry->nodeCounter<nextPathTableEntry->nodeCounter)
        {
          InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
          isInsert=true;
          break;
        }
        else if (newPathTableEntry->nodeCounter==nextPathTableEntry->nodeCounter)
        {
          if (newPathEntryDestIdent.level<nextPathEntryDestIdent.level)
          {
            InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
            isInsert=true;
            break;
          }
          else if (newPathEntryDestIdent.level==nextPathEntryDestIdent.level)
          {
            if (newPathEntryDestIdent.position<nextPathEntryDestIdent.position)
            {
              InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
              isInsert=true;
              break;
            }
            else if (newPathEntryDestIdent.position==nextPathEntryDestIdent.position)
            {
              if (newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2].level<nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-2].level)
              {
                InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                isInsert=true;
                break;
              }
              else if (newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2].level==nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-2].level)
              {
                if (newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2].position<nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-2].position)
                {
                  InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                  isInsert=true;
                  break;
                }
                else if (newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2].position==nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-2].position)
                {
                  int sameCounter=0;
                  for (int i=1;i<newPathTableEntry->nodeCounter-2;i++)//貌似这个时候长度已经相等了，只要一个判断条件就可以了
                  {
                    tempNewPathEntryIdent=newPathTableEntry->pathNodeIdent[i];
                    tempNextPathEntryIdent=nextPathTableEntry->pathNodeIdent[i];
                    if (tempNewPathEntryIdent.level<tempNextPathEntryIdent.level)//
                    {
                      InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                      isInsert=true;
                      break;
                    }
                    else if (tempNewPathEntryIdent.level==tempNextPathEntryIdent.level)
                    {
                      if (tempNewPathEntryIdent.position<tempNextPathEntryIdent.position)
                      {
                        InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
                        isInsert=true;
                        break;
                      }
                      else if (tempNewPathEntryIdent.position==tempNextPathEntryIdent.position)
                      {
                        // Logfout << "两条路径可能相同" << endl;
                        sameCounter++;
                        continue;
                      }
                      else if (tempNewPathEntryIdent.position>tempNextPathEntryIdent.position)
                      {
                        currentPathTableEntry=currentPathTableEntry->next;
                        nextPathTableEntry=nextPathTableEntry->next;
                        break;
                      }
                    }
                    else if (tempNewPathEntryIdent.level>tempNextPathEntryIdent.level)
                    {
                      currentPathTableEntry=currentPathTableEntry->next;
                      nextPathTableEntry=nextPathTableEntry->next;
                      break;
                    }
                  }
                  if (isInsert==false && sameCounter>=newPathTableEntry->nodeCounter-2-1)//出现sameCounter小于的情况是，一条路径可能只有2跳，则sameCounter=0，((newPathTableEntry->pathEntry.size()+1)/2-2)=-1.
                  {
                    PathAddNewAddr(nextPathTableEntry,newPathTableEntry->pathAddrSet);//找到了一条完全相同的路径
                    isInsert=true;
                    break;
                  }
                }
                else if (newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2].position>nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-2].position)
                {
                  currentPathTableEntry=currentPathTableEntry->next;
                  nextPathTableEntry=nextPathTableEntry->next;
                }
              }
              else if (newPathTableEntry->pathNodeIdent[newPathTableEntry->nodeCounter-2].level>nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-2].level)
              {
                currentPathTableEntry=currentPathTableEntry->next;
                nextPathTableEntry=nextPathTableEntry->next;
              }
            }
            else if (newPathEntryDestIdent.position>nextPathEntryDestIdent.position)
            {
              currentPathTableEntry=currentPathTableEntry->next;
              nextPathTableEntry=nextPathTableEntry->next;
            }
          }
          else if (newPathEntryDestIdent.level>nextPathEntryDestIdent.level)
          {
            currentPathTableEntry=currentPathTableEntry->next;
            nextPathTableEntry=nextPathTableEntry->next;
          }
        }
        else if (newPathTableEntry->nodeCounter>nextPathTableEntry->nodeCounter)
        {
          currentPathTableEntry=currentPathTableEntry->next;
          nextPathTableEntry=nextPathTableEntry->next;
        }
      }
      if (nextPathTableEntry==NULL && isInsert==false)
      {
        InsertPathTable(currentPathTableEntry,nextPathTableEntry,newPathTableEntry);
        isInsert=true;
        break;
      }
      if (isInsert) break;
    }while(nextPathTableEntry!=NULL);
  }
  PrintPathEntryTable();
  // Logfout << GetNow() << "AddNewPathTableEntry over" << endl;
}

void 
Ipv4GlobalRouting::PrintNodeConMasterByNode()//打印备用路径
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/NodeConMasterByNode-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Node" << myIdent.level << "." << myIdent.position << "'s NodeConMasterByNode" << endl;
  Logfout << "Node\t" << "Node\t" << "Node\t" << "Node\t" << "Node\t";
  Logfout << "\t\tlinkCounter\t";
  Logfout << "dirConFlag\t";
  Logfout << endl;

  for (int i=0;i<nodeConMasterByNode.size();i++)
  {
    for (int j=0;j<nodeConMasterByNode[i]->nodeCounter;j++)
    {
      Logfout << nodeConMasterByNode[i]->pathNodeIdent[j].level << "." << nodeConMasterByNode[i]->pathNodeIdent[j].position << "\t";
    }
    for (int j=nodeConMasterByNode[i]->nodeCounter;j<MAX_PATH_LEN;j++) Logfout << "\t";
    Logfout << "[" << inet_ntoa(nodeConMasterByNode[i]->nodeAddr.addr.sin_addr) << "]";
    Logfout << "\t" << nodeConMasterByNode[i]->linkCounter;
    Logfout << "\t\t" << nodeConMasterByNode[i]->dirConFlag;
    Logfout << endl;
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintMasterMapToSock()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/MasterMapToSock-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Node" << myIdent.level << "." << myIdent.position << "'s masterMapToSock" << endl;
  Logfout << "MasterAddr\tMasterIdent\tMasterSock\tNICName\t\tDirect\tMiddleAddr\tKeepAliveFaildNum\tKeepAliveFlag" << endl;
  for (int i=0;i<masterMapToSock.size();i++)
  {
    Logfout << masterMapToSock[i].masterAddr << "\t" << masterMapToSock[i].masterIdent.level << "." << masterMapToSock[i].masterIdent.position << "\t\t";
    Logfout << masterMapToSock[i].masterSock << "\t\t" << masterMapToSock[i].NICName << "\t" << masterMapToSock[i].direct << "\t" << masterMapToSock[i].middleAddr << "\t\t";
    Logfout << masterMapToSock[i].keepAliveFaildNum << "\t\t" << masterMapToSock[i].keepAliveFlag << endl;
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateNodeConMasterByNode(struct pathtableentry *tempPathTableEntry)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isSameNextHop=false;
  for (int i=0;i<nodeConMasterByNode.size();i++)
  {
    if (SameNode(tempPathTableEntry->pathNodeIdent[1],(nodeConMasterByNode[i])->pathNodeIdent[1]))// 判断下一跳是否相同
    {
      isSameNextHop=true;
      if (tempPathTableEntry->nodeCounter<(nodeConMasterByNode[i])->nodeCounter)// 更短
      {
        nodeConMasterByNode[i]=tempPathTableEntry;
        PrintNodeConMasterByNode();
        break;
      }
    }
  }
  // Logfout << "isSameNextHop " << isSameNextHop << endl;
  if (isSameNextHop==false)// 没有找到相同的下一跳就添加
  {
    nodeConMasterByNode.push_back(tempPathTableEntry);
    PrintNodeConMasterByNode();
  }
  
  Logfout.close();
}

void 
Ipv4GlobalRouting::ModifyNodeDirConFlag(ident high,ident low,bool dirConFlag)// 修改路径表，修改dirConFlag
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  vector<struct mappingtableentry> tempMappingTable;
  GetMappingTableEntry(high,low,&tempMappingTable);// 获取相关的映射表
  struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));

  int diffLinkCounter=0;// 记录遍历过程中，对应位置链路不同的情况次数，用来做判断终止条件
  for(int i=0;i<tempMappingTable.size();i++)//理论上应该只有一条映射表项
  {
    tempPathTableEntry=tempMappingTable[i].address;
    while (tempPathTableEntry!=NULL)
    {
      if (SameNode(tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1],high))// 相同的目的节点
      {
        tempPathTableEntry->dirConFlag=dirConFlag;
        diffLinkCounter=0;
      }
      else
      {
        diffLinkCounter++;
        if (diffLinkCounter>=m_LeafNodes || diffLinkCounter>=m_SpineNodes/m_LeafNodes) break;
      }
      tempPathTableEntry=tempPathTableEntry->next;
    }
  }

  Logfout.close();
}

void 
Ipv4GlobalRouting::ModifyPathEntryTable(ident high,ident low,bool linkFlag)// 修改路径表，修改linkCounter
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  vector<struct mappingtableentry> tempMappingTable;
  GetMappingTableEntry(high,low,&tempMappingTable);// 获取相关的映射表
  
  int locA=0,locB=0;
  struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
  if (tempMappingTable.size()>0) 
  {
    // 找出链路在路径中的位置
    tempPathTableEntry=tempMappingTable[0].address;
    for (int i=0;i<tempPathTableEntry->nodeCounter;i++)
    {
      if (SameNode(high,tempPathTableEntry->pathNodeIdent[i])) locA=i;
      else if (SameNode(low,tempPathTableEntry->pathNodeIdent[i])) locB=i;
    }
  }
  else 
  {
    Logfout << GetNow() << "Didn't find mapping table entry." << endl;
    exit(0);
  }

  int diffLinkCounter=0;// 记录遍历过程中，对应位置链路不同的情况次数，用来做判断终止条件
  for(int i=0;i<tempMappingTable.size();i++)//理论上应该只有一条映射表项
  {
    // 一条链路可能会影响一部分路径，可以将目的地址相同的路径的linkcounter修改完后再更新路由
    ident tempDestIdent,tempNextIdent,tempIdent;
    tempDestIdent.level=-1;
    tempDestIdent.position=-1;
    tempNextIdent.level=-1;
    tempNextIdent.position=-1;
    tempIdent.level=-1;
    tempIdent.position=-1;
    //记录某一个目的节点的节点地址和服务器地址
    struct addrset tempNodeAddr;
    vector<struct addrset> tempAddrSet;
    int tempNodeCounter=0;
    bool isNeedToUpdateRoute=false;

    tempPathTableEntry=tempMappingTable[i].address;

    if (tempPathTableEntry!=NULL) // 记录第一条链路的相关信息
    {
      tempNodeCounter=tempPathTableEntry->nodeCounter;
      tempNodeAddr=tempPathTableEntry->nodeAddr;// 记录节点地址
      for (int j=0;j<MAX_ADDR_NUM;j++)// 记录服务器地址
      {
        if (!strcmp(inet_ntoa(tempPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
        tempAddrSet.push_back(tempPathTableEntry->pathAddrSet->addrSet[i]);// 地址合法则添加
      }
      tempDestIdent=tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1];// 记录目的节点
      tempNextIdent=tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-2];// 记录倒数第二个节点
    }

    while (tempPathTableEntry!=NULL)
    {
      // 如果有相同的链路则修改linkCounter
      if ((SameNode(high,tempPathTableEntry->pathNodeIdent[locA]) && SameNode(low,tempPathTableEntry->pathNodeIdent[locB])) || (SameNode(low,tempPathTableEntry->pathNodeIdent[locA]) && SameNode(high,tempPathTableEntry->pathNodeIdent[locB])))
      {
        if (!SameNode(tempDestIdent,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]))// 目的节点不同，则记录，并更新路由
        {
          if (isNeedToUpdateRoute==true)
          {
            // 先更新服务器地址
            UpdateAddrSet(tempDestIdent,tempIdent,tempNodeCounter,tempAddrSet);
            tempAddrSet.clear();
            // 再更新到节点的地址
            tempAddrSet.push_back(tempNodeAddr);
            Logfout << GetNow() << "***************要更新的Node地址：" << inet_ntoa(tempAddrSet[0].addr.sin_addr) << endl;
            UpdateAddrSet(tempDestIdent,tempNextIdent,tempNodeCounter,tempAddrSet);
          }

          tempNodeCounter=tempPathTableEntry->nodeCounter;
          tempNodeAddr=tempPathTableEntry->nodeAddr;// 记录节点地址
          tempAddrSet.clear();
          for (int j=0;j<MAX_ADDR_NUM;j++)// 记录服务器地址
          {
            if (!strcmp(inet_ntoa(tempPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
            tempAddrSet.push_back(tempPathTableEntry->pathAddrSet->addrSet[i]);// 地址合法则添加
          }
          tempDestIdent=tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1];// 记录节点
          tempNextIdent=tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-2];// 记录倒数第二个节点
          isNeedToUpdateRoute=false;// 重置标志位
        }

        diffLinkCounter=0;

        int oldLinkCounter=tempPathTableEntry->linkCounter;
        if (linkFlag==true) tempPathTableEntry->linkCounter+=1;
        else if (linkFlag==false) tempPathTableEntry->linkCounter-=1;
        // 如果linkCounter在0和-1间变化，即路径从无效变为有效，或者从有效变为无效 
        if ((oldLinkCounter==0 && tempPathTableEntry->linkCounter==-1) || (oldLinkCounter==-1 && tempPathTableEntry->linkCounter==0))
        {
          isNeedToUpdateRoute=true;
        }
      }
      else
      {
        diffLinkCounter++;
        if (diffLinkCounter>=m_LeafNodes || diffLinkCounter>=m_SpineNodes/m_LeafNodes) break;
      }
      tempPathTableEntry=tempPathTableEntry->next;
    }
    if (isNeedToUpdateRoute==true)// 终止循环时也要考虑修改路由
    {
      // 先更新服务器地址
      UpdateAddrSet(tempDestIdent,tempIdent,tempNodeCounter,tempAddrSet);
      tempAddrSet.clear();
      // 再更新到节点的地址
      tempAddrSet.push_back(tempNodeAddr);
      Logfout << GetNow() << "***************要更新的Node地址：" << inet_ntoa(tempAddrSet[0].addr.sin_addr) << endl;
      UpdateAddrSet(tempDestIdent,tempNextIdent,tempNodeCounter,tempAddrSet);
    }
  }
  PrintPathEntryTable();
  Logfout.close();
}

/**************************Node**************************/


/**************************Master**************************/
void 
Ipv4GlobalRouting::PrintMasterLinkTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/MasterLinkTable-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
  tempLinkTableEntry=headLinkTableEntry->next;
  Logfout << "Num:" << headLinkTableEntry->lastUpdateTime << endl;//用来统计链路数量
  Logfout << "Node--Node\tlinkFlag\tlastLinkFlag\tisStartTimer" << endl;
  while (tempLinkTableEntry!=NULL)
  {
    Logfout << tempLinkTableEntry->high.level << "." << tempLinkTableEntry->high.position << "--" << tempLinkTableEntry->low.level << "." << tempLinkTableEntry->low.position << "\t";
    Logfout << "\t" << tempLinkTableEntry->linkFlag << "\t\t" << tempLinkTableEntry->lastLinkFlag << "\t\t" << tempLinkTableEntry->isStartTimer << endl;
    tempLinkTableEntry=tempLinkTableEntry->next;
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintNodeMapToSock()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/NodeMapToSock-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Node\tSock" << endl;
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    Logfout << nodeMapToSock[i].nodeIdent.level << "." << nodeMapToSock[i].nodeIdent.position << "\t" << nodeMapToSock[i].nodeSock << endl;
  }
  Logfout << endl;
  Logfout.close();
}

void* 
Ipv4GlobalRouting::MasterLinkTimer(void* threadParam)
{
  Ipv4GlobalRouting *tempGlobalRouting=((threadparamA *)threadParam)->tempGlobalRouting;
  struct linktableentry *tempLinkTableEntry=((threadparamA *)threadParam)->tempLinkTableEntry;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->myIdent.level << "." << tempGlobalRouting->myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  Logfout.setf(ios::fixed,ios::floatfield);
  Logfout.precision(9);//设置保留的小数点位数

  double threadStartTime=tempGlobalRouting->GetSystemTime();
  Logfout << GetNow() << "Master link timer start,system time:" << tempGlobalRouting->GetSystemTime() << "s." << endl;
  while (1)
  {
    usleep(tempGlobalRouting->m_defaultLinkTimer);
    if ((tempGlobalRouting->GetSystemTime()-threadStartTime)*1000000>=tempLinkTableEntry->linkUpdateTimer) break;
  }
  Logfout << GetNow() << "Master link timer timeout,system time:" << tempGlobalRouting->GetSystemTime() << "s." << endl;

  tempLinkTableEntry->isStartTimer=false;
  tempLinkTableEntry->lastUpdateTime=tempGlobalRouting->GetSystemTime();
  tempLinkTableEntry->linkUpdateTimer=tempGlobalRouting->m_defaultLinkTimer;

  if (tempLinkTableEntry->lastLinkFlag!=tempLinkTableEntry->linkFlag)//两个状态不同则处理
  {
    Logfout << GetNow() << "Different link flag." << endl;
    tempLinkTableEntry->linkFlag=tempLinkTableEntry->lastLinkFlag;
    tempGlobalRouting->SendMessageToNode(tempLinkTableEntry->high,tempLinkTableEntry->low,tempLinkTableEntry->lastLinkFlag);
    tempGlobalRouting->PrintMasterLinkTable();//故障定位
  }
  Logfout.close();
}

bool 
Ipv4GlobalRouting::UpdateMasterLinkTable(ident high,ident low,bool linkFlag)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isFindMasterLink=false;// 不存在，如果linkFlag为true，则添加，否则出错
  // 遍历整个链路表，此处可优化
  // 还必须考虑链路更新信息的上报节点，通常一条链路的两端都会上报信息
  // 暂时不写吧
  struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
  tempLinkTableEntry=headLinkTableEntry->next;
  while (tempLinkTableEntry!=NULL)
  {
    if (SameNode(high,tempLinkTableEntry->high) && SameNode(low,tempLinkTableEntry->low))// 存在这样的链路
    {
      isFindMasterLink=true;
      if (tempLinkTableEntry->linkFlag==true)//master的链路表中该条链路此时正常工作
      {
        if (linkFlag==true)// 可能是链路另一端上报的信息
        {
          return false;
        }
        else if (linkFlag==false)// 表示该条链路故障，两次链路变化的时间间隔不管是否大于定时器，都要广播该信息
        {
          tempLinkTableEntry->linkFlag=false;
          tempLinkTableEntry->lastLinkFlag==false;
          tempLinkTableEntry->lastUpdateTime=GetSystemTime(); 
          PrintMasterLinkTable();
          return true;
        }
      }
      else if (tempLinkTableEntry->linkFlag==false)
      {
        // 如果没有开启定时器，则尝试快速恢复，并开启定时器
        tempLinkTableEntry->lastLinkFlag=linkFlag;
        if (tempLinkTableEntry->isStartTimer==false)
        {
          tempLinkTableEntry->linkFlag=linkFlag;
          tempLinkTableEntry->lastUpdateTime=GetSystemTime();
          // 开启定时器
          threadparamA *threadParam=new threadparamA();
          threadParam->tempGlobalRouting=this;
          threadParam->tempLinkTableEntry=tempLinkTableEntry;
          if(pthread_create(&masterLink_thread,NULL,MasterLinkTimer,(void *)threadParam)<0)
          {
            Logfout << GetNow() << "Master could not start timer" << endl;
            exit(0);
          }
          tempLinkTableEntry->isStartTimer=true;         
          //尝试快速恢复
          if (linkFlag==true) 
          {
            PrintMasterLinkTable();
            return true;
          }
          else return false;
        }
        else// 如果已经开启了定时器，则不向node转发信息
        {
          tempLinkTableEntry->linkUpdateTimer+=m_defaultLinkTimer;//等待时间累加
          return false;
        }
      }
      PrintMasterLinkTable();
      break;
    }
    tempLinkTableEntry=tempLinkTableEntry->next;
  }
  //如果没有找到，就要添加一条这样的路径
  if (isFindMasterLink==false)
  {
    // Logfout << GetNow() << "new link: " << high.level << "." << high.position << "--" << low.level << "." << low.position << endl;
    struct linktableentry *newLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
    newLinkTableEntry->high=high;
    newLinkTableEntry->low=low;
    newLinkTableEntry->linkFlag=linkFlag;
    newLinkTableEntry->lastLinkFlag=linkFlag;
    newLinkTableEntry->lastUpdateTime=GetSystemTime();
    newLinkTableEntry->linkUpdateTimer=m_defaultLinkTimer;
    newLinkTableEntry->isStartTimer=false;
    newLinkTableEntry->next=NULL;
    // 应该考虑开启定时器保护
    struct linktableentry *currentLinkTableEntry=headLinkTableEntry;
    struct linktableentry *nextLinkTableEntry=headLinkTableEntry->next;
    do 
    {
      if (nextLinkTableEntry!=NULL)
      {
        if (newLinkTableEntry->high.level>nextLinkTableEntry->high.level)//leve越大，排序越靠前
        {
          newLinkTableEntry->next=currentLinkTableEntry->next;
          currentLinkTableEntry->next=newLinkTableEntry;
          break;
        }
        else if (newLinkTableEntry->high.level==nextLinkTableEntry->high.level)
        {
          if (newLinkTableEntry->high.position<nextLinkTableEntry->high.position)//position越小，排序越靠前
          {
            newLinkTableEntry->next=currentLinkTableEntry->next;
            currentLinkTableEntry->next=newLinkTableEntry;
            break;
          }
          else if (newLinkTableEntry->high.position==nextLinkTableEntry->high.position)
          {
            if (newLinkTableEntry->low.level>nextLinkTableEntry->low.level)
            {
              newLinkTableEntry->next=currentLinkTableEntry->next;
              currentLinkTableEntry->next=newLinkTableEntry;
              break;
            }
            else if (newLinkTableEntry->low.level==nextLinkTableEntry->low.level)
            {
              if (newLinkTableEntry->low.position<nextLinkTableEntry->low.position)
              {
                newLinkTableEntry->next=currentLinkTableEntry->next;
                currentLinkTableEntry->next=newLinkTableEntry;
                break;
              }
              else if (newLinkTableEntry->low.position==nextLinkTableEntry->low.position)
              {
                // 两条一样的链路，不可能出现
              }
              else if (newLinkTableEntry->low.position>nextLinkTableEntry->low.position)
              {
                currentLinkTableEntry=currentLinkTableEntry->next;
                nextLinkTableEntry=nextLinkTableEntry->next;
              }
            }
            else if (newLinkTableEntry->low.level<nextLinkTableEntry->low.level)
            {
              currentLinkTableEntry=currentLinkTableEntry->next;
              nextLinkTableEntry=nextLinkTableEntry->next;
            }
          }
          else if (newLinkTableEntry->high.position>nextLinkTableEntry->high.position)
          {
            currentLinkTableEntry=currentLinkTableEntry->next;
            nextLinkTableEntry=nextLinkTableEntry->next;
          }
        }
        else if (newLinkTableEntry->high.level<nextLinkTableEntry->high.level)
        {
          currentLinkTableEntry=currentLinkTableEntry->next;
          nextLinkTableEntry=nextLinkTableEntry->next;
        }
      }
      if (nextLinkTableEntry==NULL)
      {
        newLinkTableEntry->next=NULL;
        currentLinkTableEntry->next=newLinkTableEntry;
        break;
      }
    }while(nextLinkTableEntry!=NULL);
    headLinkTableEntry->lastUpdateTime++;//用来统计链路数量
    PrintMasterLinkTable();
    return false;
  }
}

void
Ipv4GlobalRouting::UpdateNodeMapToSock(ident nodeIdent,int sock,bool direct)// master和起转发的node会使用
{
  // 只有master和node建立直连后才会进行以下两步操作
  // 向该node下发所有已和master连接的node
  // 向所有已经和master连接的node下发，有新的node和master连接上了
  
  if (myIdent.level==0 && direct)
  {
    struct MNinfo tempMNInfo;
    tempMNInfo.srcIdent=myIdent;
    tempMNInfo.pathNodeIdentA=nodeIdent;
    tempMNInfo.pathNodeIdentB=myIdent;
    tempMNInfo.keepAlive=false;
    tempMNInfo.linkFlag=true;
    tempMNInfo.hello=false;
    tempMNInfo.ACK=false;
    
    for (int i=0;i<nodeMapToSock.size();i++)
    {
      if (nodeMapToSock[i].direct)// 直连
      {
        tempMNInfo.destIdent=nodeIdent;
        tempMNInfo.pathNodeIdentA=nodeMapToSock[i].nodeIdent;// pathNodeIdentB还是master
        m_tcp->SendMessageTo(sock,tempMNInfo);// 向该node下发所有和master直连的node
      }
      // 向所有已经和master连接的node下发，有新的node和master连接上了
      tempMNInfo.destIdent=nodeMapToSock[i].nodeIdent;
      tempMNInfo.pathNodeIdentA=nodeIdent;
      m_tcp->SendMessageTo(nodeMapToSock[i].nodeSock,tempMNInfo);
    }
  }

  // 先判断是否存在
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    if (SameNode(nodeIdent,nodeMapToSock[i].nodeIdent))// 存在，则覆盖
    {
      nodeMapToSock[i].nodeSock=sock;
      PrintNodeMapToSock();
      return;
    }
  }

  struct nodemaptosock sample;
  sample.nodeIdent=nodeIdent;
  sample.nodeSock=sock;
  sample.direct=direct;
  sample.keepAliveFaildNum=0;
  sample.keepAliveFlag=false;
  nodeMapToSock.push_back(sample);

  //sequence
  //需要将nodeMapToSock按照一定的规律组织
  //第一层Node放在最前，然后是第二层，最后是第三层，第一层的Node同一个Pod内的组织在一起，第二层Node对LeafNode取余，相等的放一起
  struct nodemaptosock temp;
  for (int i=0;i<nodeMapToSock.size()-1;i++)
  {
    for (int j=0;j<nodeMapToSock.size()-i-1;j++)
    {
      if (nodeMapToSock[j+1].nodeIdent.level<nodeMapToSock[j].nodeIdent.level)//大体分为一二三层Node
      {
        temp=nodeMapToSock[j+1];
        nodeMapToSock[j+1]=nodeMapToSock[j];
        nodeMapToSock[j]=temp;
      }
      else if (nodeMapToSock[j+1].nodeIdent.level==nodeMapToSock[j].nodeIdent.level)//属于同一层Node
      {
        if (nodeMapToSock[j].nodeIdent.level==1 || nodeMapToSock[j].nodeIdent.level==3)//第一三层Node
        {
          if (nodeMapToSock[j+1].nodeIdent.position<nodeMapToSock[j].nodeIdent.position)//升序排列
          {
            temp=nodeMapToSock[j+1];
            nodeMapToSock[j+1]=nodeMapToSock[j];
            nodeMapToSock[j]=temp;
          }
        }
        else if (nodeMapToSock[j].nodeIdent.level==2)//第二层Node，对LeafNode取余，相等的放一起，同时升序排列
        {
          if (nodeMapToSock[j+1].nodeIdent.position%4<nodeMapToSock[j].nodeIdent.position%4)//此处的4需要注意
          {
            temp=nodeMapToSock[j+1];
            nodeMapToSock[j+1]=nodeMapToSock[j];
            nodeMapToSock[j]=temp;
          }
        }
      }
    }
  }
  PrintNodeMapToSock();
}

int
Ipv4GlobalRouting::GetSockByIdent(ident nodeIdent)
{
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    if (SameNode(nodeIdent,nodeMapToSock[i].nodeIdent)) return nodeMapToSock[i].nodeSock;
  }
  return -1;
}
ident 
Ipv4GlobalRouting::GetIdentBySock(int sock)
{
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    if (sock==nodeMapToSock[i].nodeSock) return nodeMapToSock[i].nodeIdent;
  }
  ident tempIdent;
  tempIdent.level=-1;
  tempIdent.position=-1;
  return tempIdent;
}

void // 求出tempNode内node的上行或者下行链路的另一端结点的ident，存放于tempEffectNode中
Ipv4GlobalRouting::GetEffectNode(vector<ident> tempNode,string type,vector<ident> *tempEffectNode,struct MNinfo *tempLinkInfo)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  for (int i=0;i<tempNode.size();i++)
  {
    Logfout << GetNow() << "Send Message to Node " << tempNode[i].level << "." << tempNode[i].position << "." << endl;
    tempLinkInfo->destIdent=tempNode[i];
    m_tcp->SendMessageTo(GetSockByIdent(tempNode[i]),*tempLinkInfo);
  }

  for (int i=0;i<tempNode.size();i++)
  {
    struct linktableentry *nextLinkTableEntry=headLinkTableEntry;
    if (type=="UP")// 求上行链路
    {
      do
      {
        bool isFindSameNode=false;
        nextLinkTableEntry=nextLinkTableEntry->next;
        if (nextLinkTableEntry!=NULL)// 终止条件还可以进行判断
        {
          if (SameNode(tempNode[i],nextLinkTableEntry->low)) 
          {
            for (int i=0;i<tempEffectNode->size();i++)
            {
              if (SameNode((*tempEffectNode)[i],nextLinkTableEntry->high))// 存在相同的则不再添加，这些地方都必须优化
              {
                isFindSameNode=true;
                break;
              }
            }
            if (!isFindSameNode) tempEffectNode->push_back(nextLinkTableEntry->high);
          }
        }
        else break;
      }while(nextLinkTableEntry!=NULL);
    }
    else if (type=="DOWN")// 求下行链路
    {
      do
      {
        bool isFindSameNode=false;
        nextLinkTableEntry=nextLinkTableEntry->next;
        if (nextLinkTableEntry!=NULL)// 终止条件还可以进行判断
        {
          if (SameNode(tempNode[i],nextLinkTableEntry->high)) 
          {
            for (int i=0;i<tempEffectNode->size();i++)
            {
              if (SameNode((*tempEffectNode)[i],nextLinkTableEntry->low))// 存在相同的则不再添加，这些地方都必须优化
              {
                isFindSameNode=true;
                break;
              }
            }
            if (!isFindSameNode) tempEffectNode->push_back(nextLinkTableEntry->low);
          }
        }
      }while(nextLinkTableEntry!=NULL);
    }
  }
  tempNode.clear();// 清空

  for (int i=0;i<tempEffectNode->size();i++) tempNode.push_back((*tempEffectNode)[i]);
  
  if (tempNode[0].level!=1)// 还未到tor
  {
    tempEffectNode->clear();
    GetEffectNode(tempNode,"DOWN",tempEffectNode,tempLinkInfo);
  }
  else// 此处需要考虑，如果是100个leaf，求10000个tor，
  {
    for (int i=0;i<tempEffectNode->size();i++)
    {
      Logfout << GetNow() << "Send Message to Node " << (*tempEffectNode)[i].level << "." << (*tempEffectNode)[i].position << "." << endl;
      tempLinkInfo->destIdent=(*tempEffectNode)[i];
      m_tcp->SendMessageTo(GetSockByIdent((*tempEffectNode)[i]),*tempLinkInfo);
    }
    return;
  }
  Logfout.close();
}

void
Ipv4GlobalRouting::SendMessageToNode(ident high,ident low,bool linkFlag)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  Logfout << GetNow() << "Master start to send message:" << endl;

  // 确定影响范围，此处可优化，这代码写的太垃圾了
  struct MNinfo *tempLinkInfo=(struct MNinfo *)malloc(sizeof(struct MNinfo));
  tempLinkInfo->destIdent=high;// 不可省略，未初始化不能使用
  tempLinkInfo->srcIdent=myIdent;
  tempLinkInfo->pathNodeIdentA=high;
  tempLinkInfo->pathNodeIdentB=low;
  tempLinkInfo->keepAlive=false;
  tempLinkInfo->linkFlag=linkFlag;
  tempLinkInfo->hello=false;
  tempLinkInfo->ACK=false;

  vector<ident> tempNode,tempEffectNode;
  tempNode.push_back(high);

  if (high.level==2) GetEffectNode(tempNode,"UP",&tempEffectNode,tempLinkInfo);
  else if (high.level==3) GetEffectNode(tempNode,"DOWN",&tempEffectNode,tempLinkInfo);
}

/**************************Master**************************/