#include "ipv4-global-routing.h"

using namespace std;
pthread_mutex_t mutex;

Ipv4GlobalRouting::~Ipv4GlobalRouting()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // struct pathtableentry *tempPathTableEntry=headPathTableEntry->next;
  // while (tempPathTableEntry)
  // {
  //   Logfout << GetNow() << "jin" << endl;
  //   if (tempPathTableEntry->nodeCounter!=1)
  //   {
  //     DelRoute(tempPathTableEntry->nodeAddr.addr,tempPathTableEntry->nodeAddr.prefixLen);
  //   } 

  //   for (int i=0;i<MAX_ADDR_NUM;i++)
  //   {
  //     if (!strcmp(inet_ntoa(tempPathTableEntry->pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
  //     else
  //     {
  //       DelRoute(tempPathTableEntry->pathAddrSet->addrSet[i].addr,tempPathTableEntry->pathAddrSet->addrSet[i].prefixLen);
  //     }
  //   }
    
  //   tempPathTableEntry=tempPathTableEntry->next;
  // }
  
  Logfout << GetNow() << "Goodbye!" << endl; 
  Logfout.close();
}

Ipv4GlobalRouting::Ipv4GlobalRouting(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultLinkTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << level << "." << position << ".log";
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
  m_defaultLinkTimer=defaultLinkTimer;// master链路定时器
  m_defaultKeepaliveTimer=defaultKeepaliveTimer;
  m_IsCenterRouting=IsCenterRouting;
  m_randomEcmpRouting=randomEcmpRouting;
 
  ident tempNode;
  tempNode.level=-1;
  tempNode.position=-1;

  if (myIdent.level==0 && myIdent.position==1) chiefMaster=true;
  else chiefMaster=false;

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

    headMappingTableEntry->high=tempNode;
    headMappingTableEntry->low=tempNode;
    headMappingTableEntry->address=NULL;
    headMappingTableEntry->next=NULL;
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

  m_tcpRoute=new TCPRoute(this,m_defaultKeepaliveTimer,chiefMaster);
  if(myIdent.level!=0)
  {
    m_udpServer.SetGlobalRouting(this);
    m_udpClient.SetGlobalRouting(this);
    // sonic测试
    // ifstream logfin("/home/tencent/Primus/NICName.txt");
    // string str;
    // while (getline(logfin,str))
    // {
    //   int i=0;
    //   while (str[i++]!=':');
    //   if ((myIdent.level+myIdent.position*0.1)==atof((str.substr(0,i)).c_str()))//
    //   {
    //     int pos=i;
    //     for (;i<str.length();i++)
    //     {
    //       if (str[i]==',') 
    //       {
    //         listenNICName.push_back(str.substr(pos,i-pos));
    //         pos=i+1;
    //       }
    //     }
    //     break;
    //   }
    // }
    // logfin.close();
    // endl
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
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  masterAddress=tempMasterAddress;
  if(myIdent.level==0)
  {
    // cout << "I am master!" << endl;
    if (!chiefMaster)
    {
      m_tcpRoute->SendHelloToChief(masterAddress[1],"eth0");
    }

    // threadparamA *threadParam=new threadparamA();
    // threadParam->tempGlobalRouting=this;
    // threadParam->tempLinkTableEntry=NULL;
    // if(pthread_create(&listenKeepAlive_thread,NULL,ListenKeepAliveThread,(void *)threadParam)<0)
    // {
    //   Logfout << GetNow() << "Master could not check keepAlive." << endl;
    //   exit(0);
    // }
    // pthread_detach(listenKeepAlive_thread);
    m_tcpRoute->StartListen();
  }
  else
  {
    // cout << "I am not master!" << endl;
    m_udpServer.StartApplication();
    ListenNIC();
    m_tcpRoute->StartListen();
  }
  Logfout.close();
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
// 返回值为1表示正常，为2表示keepalive未接收次数超过3次，为3表示套接字不存在
int   
Ipv4GlobalRouting::UpdateKeepAlive(int masterSock,ident masterIdent,bool recvKeepAlive)// keepAliveFlag为真，则表示master回复了keep alive；为假，则表示node发出了keep alive
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (masterSock==masterMapToSock[i].masterSock && SameNode(masterIdent,masterMapToSock[i].masterIdent))
    {
      if (recvKeepAlive==true)// 收到回复
      {
        masterMapToSock[i].keepAliveFaildNum=0;
        masterMapToSock[i].recvKeepAlive=true;
        return 1;
      }
      else if (recvKeepAlive==false)// 发出了keepalive
      {
        // 先检查上次发送的keep alive是否回复
        if (masterMapToSock[i].recvKeepAlive==false)// 表示没有收到上一次的keep alive回复
        {
          masterMapToSock[i].keepAliveFaildNum++;
          if (masterMapToSock[i].keepAliveFaildNum>3)
          {
            Logfout << GetNow() << "KeepAlive timeout,connect with master " << masterIdent.level << "." << masterIdent.position << " failed." << endl;
            return 2;
          }
          return 1;
        }
        else if (masterMapToSock[i].recvKeepAlive==true)
        {
          masterMapToSock[i].recvKeepAlive=false;
          return 1;
        }
      }
    }
  }
  return 3;
  Logfout.close();
}

void* 
Ipv4GlobalRouting::KeepAliveThread(void* tempThreadParam)
{
  // pthread_detach(pthread_self());
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamC *)tempThreadParam)->tempGlobalRouting;
  TCPRoute *tempTCPRoute=((struct threadparamC *)tempThreadParam)->tempTCPRoute;
  ident masterIdent=((struct threadparamC *)tempThreadParam)->masterIdent;
  int masterSock=((struct threadparamC *)tempThreadParam)->masterSock;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->GetMyIdent().level << "." << tempGlobalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  Logfout << GetNow() << "Node start to send keepAlive to master " << masterIdent.level << "." << masterIdent.position << "[sock:" << masterSock << "]." << endl;

  struct MNinfo tempMNInfo;
  tempMNInfo.addr.sin_family=AF_INET;// addr此时无实际意义
  inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
  tempMNInfo.addr.sin_port=htons(0);
  tempMNInfo.destIdent=masterIdent;
  tempMNInfo.srcIdent=tempGlobalRouting->myIdent;
  tempMNInfo.pathNodeIdentA=tempGlobalRouting->myIdent;
  tempMNInfo.pathNodeIdentB=tempGlobalRouting->myIdent;
  tempMNInfo.clusterMaster=false;
  tempMNInfo.chiefMaster=false;
  tempMNInfo.keepAlive=true;
  tempMNInfo.linkFlag=false;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;

  int tempDefaultKeepaliveTimer=tempGlobalRouting->m_defaultKeepaliveTimer;
  int temp=0;
  while (1)
  {
    sleep(tempDefaultKeepaliveTimer);
    tempTCPRoute->SendMessageTo(masterSock,tempMNInfo);
    temp=tempGlobalRouting->UpdateKeepAlive(masterSock,masterIdent,false);
    if (temp==2) // keepalive timeout
    {
      Logfout << GetNow() << "Node stop to send keepAlive to master " << masterIdent.level << "." << masterIdent.position << "[sock:" << masterSock << "]." << endl;
      if (tempGlobalRouting->myIdent.level==0)// common Master处理chief Master宕机
      {
        if (tempGlobalRouting->SameNode(tempGlobalRouting->GetChiefMasterIdent(),masterIdent))// chiefmaster挂了
        {
          Logfout << GetNow() << "ChiefMaster may be DOWN!" << endl;
          tempGlobalRouting->NewChiefMasterElection(masterIdent);// 选举新的chief master
          shutdown(masterSock,SHUT_RDWR);
        }
      }
      else if (tempGlobalRouting->myIdent.level!=0)// node处理chiefmaster宕机
      {
        shutdown(masterSock,SHUT_RDWR);
      }
      break;
    }
    else if (temp==3) // 套接字已经不存在了，可能是换了新的连接
    {
      Logfout << GetNow() << "Node stop to send keepAlive to master " << masterIdent.level << "." << masterIdent.position << "[sock:" << masterSock << "]." << endl;
      shutdown(masterSock,SHUT_RDWR);
      break;
    }
  }
  Logfout.close();
  // pthread_exit(0);
}

void 
Ipv4GlobalRouting::UpdateMasterMapToSock(struct mastermaptosock tempMasterMapToSock,int cmd)//cmd=1，添加；cmd=-1，删除
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (cmd==1)
  {
    bool isFind=false;
    for (int i=0;i<masterMapToSock.size();i++)
    {
      if (masterMapToSock[i].masterAddr==tempMasterMapToSock.masterAddr)// 已经存在了，且master ident无效
      {
        if (tempMasterMapToSock.masterIdent.level==-1)// 属于再次尝试连接
        {
          // 重新覆盖
          masterMapToSock[i].chiefMaster=tempMasterMapToSock.chiefMaster;
          masterMapToSock[i].masterSock=tempMasterMapToSock.masterSock;
          masterMapToSock[i].NICName=tempMasterMapToSock.NICName;
          masterMapToSock[i].direct=tempMasterMapToSock.direct;
          masterMapToSock[i].middleAddr=tempMasterMapToSock.middleAddr;
          masterMapToSock[i].keepAliveFaildNum=tempMasterMapToSock.keepAliveFaildNum;
          masterMapToSock[i].recvKeepAlive=tempMasterMapToSock.recvKeepAlive;
          PrintMasterMapToSock();
          isFind=true;
        }
        else if (tempMasterMapToSock.masterIdent.level==0)// 收到ack后来修改ident
        {
          if (masterMapToSock[i].isStartKeepAlive==false)
          {
            // 收到ACK时调用该函数是不知道网口名称的
            masterMapToSock[i].masterIdent=tempMasterMapToSock.masterIdent;// 收到ack后改ident
            masterMapToSock[i].masterSock=tempMasterMapToSock.masterSock;
            masterMapToSock[i].direct=tempMasterMapToSock.direct;
            masterMapToSock[i].chiefMaster=tempMasterMapToSock.chiefMaster;
            masterMapToSock[i].isStartKeepAlive=true;
            // struct threadparamC *threadParam=(struct threadparamC *)malloc(sizeof(struct threadparamC));
            // threadParam->tempGlobalRouting=this;
            // threadParam->tempTCPRoute=m_tcpRoute;
            // threadParam->masterIdent=tempMasterMapToSock.masterIdent;
            // threadParam->masterSock=tempMasterMapToSock.masterSock;
            // if (pthread_create(&keepalive_thread,NULL,KeepAliveThread,(void *)threadParam)<0)
            // {
            //   Logfout << GetNow() << "Create KeepAliveThread for sock[" << tempMasterMapToSock.masterSock << "] failed." << endl;
            // }
            // pthread_detach(keepalive_thread);
            PrintMasterMapToSock();
          }
          isFind=true;
        }
      }
      else
      {
        if (tempMasterMapToSock.chiefMaster)// 如果确定了chiefMaster，那其他的都必须是common master，主要是当chiefmaster发生变化后
        {
          masterMapToSock[i].chiefMaster=false;
        }
      }
    }
    // 不存在，则添加
    if (isFind==false)
    {
      masterMapToSock.push_back(tempMasterMapToSock);
      PrintMasterMapToSock();
    }
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

ident 
Ipv4GlobalRouting::GetChiefMasterIdent()
{
  if (myIdent.level==0)
  {
    if (chiefMaster) return myIdent;
    else
    {
      for (int i=0;i<clusterMasterInfo.size();i++)
      {
        if (clusterMasterInfo[i].chiefMaster) return clusterMasterInfo[i].masterIdent;
      }
    }
  }
  else 
  {
    for (int i=0;i<masterMapToSock.size();i++)
    {
      if (masterMapToSock[i].chiefMaster) return masterMapToSock[i].masterIdent;
    }
  }
}

void 
Ipv4GlobalRouting::GetLocalAddrByNeighborIdent(struct sockaddr_in *localAddr,ident neighborIdent)
{
  for (int i=0;i<NICInfo.size();i++)
  {
    if (SameNode(NICInfo[i].neighborIdent,neighborIdent)) 
    {
      *localAddr=NICInfo[i].localAddr;
      break;
    }
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

void 
Ipv4GlobalRouting::GetAddrByNICName(struct sockaddr_in *addr,string NICName)// 通过rtnetlink获取网口的地址
{
  struct ifaddrs *ifa;
  if (0!=getifaddrs(&ifa))
  {
    printf("getifaddrs error\n");
    return;
  }
  for (;ifa!=NULL;)
  {
    if (ifa->ifa_name==NICName && ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL)
    {
      *addr=*(struct sockaddr_in *)(ifa->ifa_addr);
      return;
    }
    ifa=ifa->ifa_next;
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
    // 同时判断链路信息是否需要下发Node，因为链路信息可能已经处理了
    bool isNeedToNotice=UpdateMasterLinkTable(high,low,linkFlag);
    // 如果是选举出来的处理信息的master
    if (chiefMaster)
    {
      if (isNeedToNotice) SendMessageToNode(high,low,linkFlag);
    }
  }
  else // node处理从master收到的链路变化信息
  {
    if (low.level==0)// 这是master的直连信息
    {
      ModifyNodeDirConFlag(high,low,linkFlag);
    }
    else
    {
      ModifyPathEntryTable(high,low,linkFlag);   
    }
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

  // 判断neighborIdent、isSwitch和isServer
  if (tempNICInfo.neighborIdent.level!=-1 && tempNICInfo.neighborIdent.position!=-1)
  {
    if (tempNICInfo.isSwitch && !tempNICInfo.isServer) return true;
    else return false;
  }
  else return false;
  Logfout.close();
}

// 完成邻居发现有两个条件：收到邻居发来的ND和向邻居发送ND后收到回复
void 
Ipv4GlobalRouting::NDRecvND(struct NDinfo tempNDInfo)//改neighborIdent，收到邻居发来的ND
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isFindNICInfo=false;
  string remoteAddr=inet_ntoa(tempNDInfo.localAddr.sin_addr);
  
  for (int i=0;i<NICInfo.size();i++)
  {
    if (!strcmp(inet_ntoa(NICInfo[i].neighborAddr.sin_addr),remoteAddr.c_str()))// 存在
    {
      // 还存在一种情况，由于neighbor没有收到ACK，所以会不停的发送ND，这就会导致FreshNeighboorList被不停的调用
      if (NICInfo[i].neighborIdent.level==-1 || NICInfo[i].neighborIdent.position==-1)
      {
        NICInfo[i].neighborIdent=tempNDInfo.myIdent;// 收邻居发来的hello
        isFindNICInfo=true;
        // 判断向邻居发送hello后是否收到回复
        if (NICInfo[i].isServer!=NICInfo[i].isSwitch)// 不是交换机就是服务器，说明已经收到hello的回复了
        {
          FreshNeighboorList(NICInfo[i]);
        }
      }
      break;
    }
  }
  if (isFindNICInfo==false)
  {
    struct NICinfo temp;
    temp.NICName="";
    temp.isMaster=false;
    temp.isServer=false;
    temp.isSwitch=true;// 主动收到对面的ND，肯定是switch
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
  
  for (int i=0;i<NICInfo.size();i++)
  {
    if (!strcmp(inet_ntoa(NICInfo[i].localAddr.sin_addr),localAddr.c_str()))// 存在
    {
      if (tempNDInfo.myIdent.level==-1 && myIdent.level==1)// 发现服务器
      {
        NICInfo[i].isServer=true;
        FreshNeighboorList(NICInfo[i]);
      }
      else if (tempNDInfo.myIdent.level!=-1)// 发现交换机
      {
        NICInfo[i].isSwitch=true;
        if (NICInfo[i].neighborIdent.level!=-1 && NICInfo[i].neighborIdent.position!=-1)// 说明已经收到hello的回复了
        {
          FreshNeighboorList(NICInfo[i]);
        }
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
  // pthread_mutex_lock(&mutex);
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
    tempMNInfo.addr.sin_family=AF_INET;// 此时addr无实际意义
    inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
    tempMNInfo.addr.sin_port=htons(0);
    tempMNInfo.srcIdent=myIdent;
    tempMNInfo.pathNodeIdentA=myIdent;
    tempMNInfo.pathNodeIdentB=tempNICInfo.neighborIdent;
    tempMNInfo.clusterMaster=false;
    tempMNInfo.chiefMaster=false;
    tempMNInfo.keepAlive=false;
    tempMNInfo.linkFlag=true;
    tempMNInfo.hello=false;
    tempMNInfo.ACK=false;

    int value=0;
    for (int j=0;j<masterMapToSock.size();j++)// 向master上报链路信息
    {
      tempMNInfo.destIdent=masterMapToSock[j].masterIdent;
      value=m_tcpRoute->SendMessageTo(masterMapToSock[j].masterSock,tempMNInfo);
      Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdentA.level << "." << tempMNInfo.pathNodeIdentA.position << "--" << tempMNInfo.pathNodeIdentB.level << "." << tempMNInfo.pathNodeIdentB.position << " up to master ";
      Logfout << masterMapToSock[j].masterIdent.level << "." << masterMapToSock[j].masterIdent.position << "[value:" << value << "]." << endl;
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
            usleep(SEND_PATH_INTERVAL);// 防止缓存满了
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

      usleep(SEND_PATH_INTERVAL);// 防止缓存满了
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
            usleep(SEND_PATH_INTERVAL);// 限速保护
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
  // pthread_mutex_unlock(&mutex);
}

void 
Ipv4GlobalRouting::FreshPathTable(struct pathinfo *tempPathInfo,struct sockaddr_in remote_addr)//收到了新的路径
{
  // pthread_mutex_lock(&mutex);
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
          usleep(SEND_PATH_INTERVAL);// 限速保护
          m_udpClient.SendPathInfoTo(NICInfo[i].localAddr,NICInfo[i].neighborAddr,tempPathInfo);
        }
      }
    } 
  }
  Logfout.close();
  // pthread_mutex_unlock(&mutex);
}

void 
Ipv4GlobalRouting::ReconnectWithMaster()// 重连Master，1、如果是本地检测到网口关闭且影响与master的连接，2、master通过其他node来通知重连
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // for (int i=0;i<masterMapToSock.size();i++) shutdown(masterMapToSock[i].masterSock,SHUT_RDWR);
  masterMapToSock.clear();
  // 应该先判断是否可以通过管理网口连接
  // 然后再考虑通过间接路径
  for (int i=0;i<nodeInDirPathTable.size();i++)
  {
    if (selectInDirPathIndex!=i && nodeInDirPathTable[i]->linkCounter==0 && nodeInDirPathTable[i]->dirConFlag==true)
    {
      ident tempIdent=nodeInDirPathTable[i]->pathNodeIdent[nodeInDirPathTable[i]->nodeCounter-1];
      // int port=tempIdent.level*1000+tempIdent.position*100;// sonic test
      string middleAddress=inet_ntoa(nodeInDirPathTable[i]->nodeAddr.addr.sin_addr);
      // tempTCPRoute->SendHelloToMaster(tempMasterAddress,middleAddress,nextHopNICName,port);// sonic test
      selectInDirPathIndex=i;// 记录选中的间接路径，再发送给master
      m_tcpRoute->SendHelloToMaster(masterAddress,middleAddress,GetNICNameByRemoteAddr(nodeInDirPathTable[i]->nodeAddr.addr));
      break;
    }
  }
  Logfout.close();
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
      // 故障网口恢复
      if (NICInfo[i].isServer==true && NICInfo[i].isSwitch==true && NICInfo[i].isMaster==true) 
      {
        NICInfo[i].isServer=false;
        NICInfo[i].isSwitch=false;
        NICInfo[i].isMaster=false;
        return true;
      }
      else return false;//不是新的NIC
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
    // if (ifa->ifa_name[0]=='e') // sonic 
    if (!strcmp(ifa->ifa_name,"eth0"))// vm
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
    // else if (ifa->ifa_name[0]=='E')// sonic
    else// vm 
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
Ipv4GlobalRouting::ListenNICThread(void* tempThreadParam)
{
  // pthread_detach(pthread_self());
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamB *)tempThreadParam)->tempGlobalRouting;
  UDPClient *tempUdpClient=((struct threadparamB *)tempThreadParam)->tempUdpClient;
  TCPRoute *tempTCPRoute=((struct threadparamB *)tempThreadParam)->tempTCPRoute;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->GetMyIdent().level << "." << tempGlobalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  Logfout << GetNow() << "Node ListenInterfaces And Submit......" << endl;

  struct ifaddrs *ifa;

  while (1)
  {
    if (0!=getifaddrs(&ifa))
    {
      Logfout << GetNow() << "Getifaddrs error." << endl;
      break;
    }
    for (;ifa!=NULL;)
    {
      // // 虚拟机
      if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && ifa->ifa_name[0]=='e')
      // // Sonic
      // if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && (ifa->ifa_name[0]=='E' || ifa->ifa_name[0]=='e'))// 交换机
      {
        // Logfout << "get NIC name is " << ifa->ifa_name << endl;
        // if (tempGlobalRouting->isListenNIC(ifa->ifa_name) && tempGlobalRouting->IsNewNIC(ifa))// sonic,判断是否为未记录的网卡，是返回1
        if (tempGlobalRouting->IsNewNIC(ifa))
        {
          // if (ifa->ifa_name[0]=='e')// sonic
          if (!strcmp(ifa->ifa_name,"eth0"))
          {
            Logfout << GetNow() << "New NIC " << ifa->ifa_name << "(" << inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr) << ") UP." << endl;
            // 应该先找出没有连接上的master，放后面再写
            if (tempGlobalRouting->nodeInDirPathTable.size()==0)// 说明没有master连上
            {
              string middleAddress="";
              tempTCPRoute->SendHelloToMaster(tempGlobalRouting->masterAddress,middleAddress,ifa->ifa_name);
            }
          }
          // else if (ifa->ifa_name[0]=='E')// sonic
          else
          {
            Logfout << GetNow() << "New NIC " << ifa->ifa_name << "(" << inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr) << ") UP." << endl;
            struct sockaddr_in localAddr,remoteAddr;
            localAddr=*((struct sockaddr_in *)(ifa->ifa_addr));
            bzero(&remoteAddr,sizeof(struct sockaddr_in));
            remoteAddr.sin_family=AF_INET;
            string address=inet_ntoa(localAddr.sin_addr);
            address[address.size()-1]=(address[address.size()-1]=='1')? '2':'1';
            remoteAddr.sin_addr.s_addr=inet_addr((char*)address.c_str());
            remoteAddr.sin_port=htons(ND_PORT);
            struct NDinfo tempNDInfo;
            tempNDInfo.myIdent=tempGlobalRouting->myIdent;
            tempNDInfo.localAddr=localAddr;
            tempUdpClient->SendNDTo(localAddr,remoteAddr,tempNDInfo);
          }
        }
      }
      ifa=ifa->ifa_next;
    }
    freeifaddrs(ifa);

    for (auto iter=tempGlobalRouting->NICInfo.begin();iter!=tempGlobalRouting->NICInfo.end();)
    {
      // 网口或者链路失效后，邻居关系依然保存，所以以下是判断是否为正常的邻居关系转为异常
      if ((*iter).flag==false && ((*iter).isServer==false || (*iter).isSwitch==false || (*iter).isMaster==false))// 网卡或者链路出故障了
      {
        Logfout << GetNow() << "NIC " << (*iter).NICName << "(" << inet_ntoa((*iter).localAddr.sin_addr) << ") DOWN." << endl; 
        // 判断是否要和master重连
        for (auto tempIter=tempGlobalRouting->masterMapToSock.begin();tempIter!=tempGlobalRouting->masterMapToSock.end();)
        {
          if ((*iter).NICName==(*tempIter).NICName) // 本地网口故障导致master需要重连
          {
            tempGlobalRouting->ReconnectWithMaster();
            break;
          }
          tempIter++;
        }
        
        // 如果是到master的直连挂了，也可以上报，但是还没有写和master有关的ND
        // if ((*iter).NICName[0]!='e')// sonic
        if (strcmp((*iter).NICName.c_str(),"eth0"))// 此处处理非管理网口，管理网口关闭，直连断掉需要另外处理
        {
          // 不需要或者重连完毕
          struct MNinfo tempMNInfo;
          tempMNInfo.addr.sin_family=AF_INET;// 此时addr无实际意义
          inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
          tempMNInfo.addr.sin_port=htons(0);
          tempMNInfo.srcIdent=tempGlobalRouting->myIdent;
          tempMNInfo.pathNodeIdentA=tempGlobalRouting->myIdent;
          tempMNInfo.pathNodeIdentB=(*iter).neighborIdent;
          tempMNInfo.clusterMaster=false;
          tempMNInfo.chiefMaster=false;
          tempMNInfo.keepAlive=false;
          tempMNInfo.linkFlag=false;
          tempMNInfo.hello=false;
          tempMNInfo.ACK=false;
          // 上传信息
          for (int i=0;i<tempGlobalRouting->masterMapToSock.size();i++)
          {
            tempMNInfo.destIdent=tempGlobalRouting->masterMapToSock[i].masterIdent;
            tempTCPRoute->SendMessageTo(tempGlobalRouting->masterMapToSock[i].masterSock,tempMNInfo);
          }
        }

        // iter=tempGlobalRouting->NICInfo.erase(iter);
        // if (iter==tempGlobalRouting->NICInfo.end()) break;
        // 做一些特殊标记，表示之前邻居发现已经完成，由于网口或者链路故障，但继续保留邻居信息
        (*iter).isServer=true;
        (*iter).isSwitch=true;
        (*iter).isMaster=true;
        continue;
      }
      else if ((*iter).flag==true) (*iter).flag=false;
      iter++;
    }
    usleep(NIC_CHECK_INTERVAL);
  }
  Logfout.close();
  // pthread_exit(0);
}

void 
Ipv4GlobalRouting::ListenNIC()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  struct threadparamB *threadParam=(struct threadparamB *)malloc(sizeof(struct threadparamB));
  threadParam->tempGlobalRouting=this;
  threadParam->tempUdpClient=&m_udpClient;
  threadParam->tempTCPRoute=m_tcpRoute;

  if(pthread_create(&listenNIC_thread,NULL,ListenNICThread,(void*)threadParam)<0)
  {
    Logfout << GetNow() << "Create thread for ListenNIC failed!!!!!!!!!" << endl;
    exit(0);
  }
  // pthread_detach(listenNIC_thread);
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
  logFoutPath << "/var/log/PathEntryTable-" << myIdent.level << "." << myIdent.position << ".txt";
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
    else Logfout << "\t";
    // // 服务器地址
    // for (int i=0;i<MAX_ADDR_NUM;i++)
    // {
    //   if (!strcmp(inet_ntoa((*iter).pathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
    //   Logfout << "(" << inet_ntoa((*iter).pathAddrSet->addrSet[i].addr.sin_addr) << ")";
    // }
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
  logFoutPath << "/var/log/MappingTable-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  struct mappingtableentry *tempMappingTableEntry=headMappingTableEntry->next;
  while (tempMappingTableEntry!=NULL)
  {
    Logfout << tempMappingTableEntry->high.level << "." << tempMappingTableEntry->high.position << "--" << tempMappingTableEntry->low.level << "." << tempMappingTableEntry->low.position << "\t\t";
    struct pathtableentry *tempPathTableEntry=tempMappingTableEntry->address;
    for (int j=0;j<tempPathTableEntry->nodeCounter;j++)
    {
      Logfout << tempPathTableEntry->pathNodeIdent[j].level << "." << tempPathTableEntry->pathNodeIdent[j].position << "\t";
    } 
    Logfout << endl;
    tempMappingTableEntry=tempMappingTableEntry->next;
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintMappingTableIndex()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/MappingTableIndex-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  for (int i=0;i<mappingTableEntryIndex.size();i++)
  {
    Logfout << mappingTableEntryIndex[i]->high.level << "." << mappingTableEntryIndex[i]->high.position << "--" << mappingTableEntryIndex[i]->low.level << "." << mappingTableEntryIndex[i]->low.position << "\t\t";
    struct pathtableentry *tempPathTableEntry=mappingTableEntryIndex[i]->address;
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

bool 
Ipv4GlobalRouting::InformUnreachableNode(ident destIdent,ident srcIdent)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 先判断是否有到destIdent的路径
  vector<struct mappingtableentry> tempMappingTable;
  GetMappingTableEntry(destIdent,srcIdent,&tempMappingTable);// 获取相关的映射表
  struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
  int diffLinkCounter=0;// 记录遍历过程中，对应位置链路不同的情况次数，用来做判断终止条件
  for(int i=0;i<tempMappingTable.size();i++)
  {
    tempPathTableEntry=tempMappingTable[i].address;
    while (tempPathTableEntry!=NULL)
    {
      if (SameNode(tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1],destIdent))// 相同的目的节点，即存在这样的路径且可达
      {
        if (tempPathTableEntry->linkCounter==0)
        {
          struct sockaddr_in localAddr,remoteAddr;
          remoteAddr=tempPathTableEntry->nodeAddr.addr;
          localAddr=tempPathTableEntry->nodeAddr.addr;
          remoteAddr.sin_port=htons(ND_PORT);
          GetLocalAddrByNeighborIdent(&localAddr,tempPathTableEntry->pathNodeIdent[1]);
          struct NDinfo tempNDInfo;
          tempNDInfo.myIdent=srcIdent;
          tempNDInfo.localAddr=localAddr;
          m_udpClient.SendNDTo(localAddr,remoteAddr,tempNDInfo);
          return true;
        }
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
  return false;
  Logfout.close();
}

void 
Ipv4GlobalRouting::SendInDirPathToMaster(int sock,ident pathNodeIdentA,ident pathNodeIdentB)// pathNodeIdentA和pathNodeIdentB是上一个hello包发送的
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (selectInDirPathIndex==-1) 
  {
    Logfout << GetNow() << "selectInDirPathIndex:" << selectInDirPathIndex << "." << endl;
    return;
  }

  struct pathtableentry tempPathTableEntry=*(nodeInDirPathTable[selectInDirPathIndex]);
  struct MNinfo helloMNIfo;
  helloMNIfo.addr.sin_family=AF_INET;
  inet_aton("255.255.255.255",&(helloMNIfo.addr.sin_addr));
  helloMNIfo.addr.sin_port=htons(0);
  GetLocalAddrByNeighborIdent(&(helloMNIfo.addr),tempPathTableEntry.pathNodeIdent[0]);
  helloMNIfo.destIdent.level=-1;// 发给所有的master
  helloMNIfo.destIdent.position=-1;
  helloMNIfo.srcIdent=myIdent;

  bool isNeedToSend=false;

  if (pathNodeIdentA.level==-1 || pathNodeIdentB.level==-1)// 第一次发送
  {
    // master通过pathNodeIdentA、pathNodeIdentB来判断连接是否为直接连接
    // 如果是间接连接则pathNodeIdentA、pathNodeIdentB代表间接路径的一部分
    helloMNIfo.pathNodeIdentA=tempPathTableEntry.pathNodeIdent[0];
    helloMNIfo.pathNodeIdentB=tempPathTableEntry.pathNodeIdent[1];
    isNeedToSend=true;
  }
  else 
  {
    for (int i=0;i<tempPathTableEntry.nodeCounter;i++)
    {
      if (SameNode(pathNodeIdentB,tempPathTableEntry.pathNodeIdent[i]))// 上一次发送到这里了
      {
        if (i==tempPathTableEntry.nodeCounter-1)// 已经全部发送完毕
        {
          isNeedToSend=false;
        }
        else 
        {
          helloMNIfo.pathNodeIdentA=pathNodeIdentB;
          helloMNIfo.pathNodeIdentB=tempPathTableEntry.pathNodeIdent[i+1];
          isNeedToSend=true;
        }
        break;
      }
    }
  }
  // master通过pathNodeIdentA、pathNodeIdentB来判断连接是否为直接连接
  // 如果是间接连接则pathNodeIdentA、pathNodeIdentB代表间接路径的一部分
  if (isNeedToSend)
  {
    helloMNIfo.clusterMaster=false;
    helloMNIfo.chiefMaster=chiefMaster;
    helloMNIfo.keepAlive=false;
    helloMNIfo.linkFlag=false;
    helloMNIfo.hello=true;
    helloMNIfo.ACK=false;
    Logfout << GetNow() << "Send inDirPath(" << helloMNIfo.pathNodeIdentA.level << "." << helloMNIfo.pathNodeIdentA.position << "--" << helloMNIfo.pathNodeIdentB.level << "." << helloMNIfo.pathNodeIdentB.position << ")to master." << endl;
    m_tcpRoute->SendMessageTo(sock,helloMNIfo);
  }
  // else Logfout << GetNow() << "Send inDirPath over." << endl;
  Logfout.close();
}

void
Ipv4GlobalRouting::GetHeadPathTableEntry(struct pathtableentry **tempPathTableEntry)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  *tempPathTableEntry=headPathTableEntry;

  Logfout.close();
}

int 
Ipv4GlobalRouting::GetMappingTableEntryIndex(ident high)// 获取映射表的索引
{
  for (int i=0;i<mappingTableEntryIndex.size();i++)
  {
    if (SameNode(high,mappingTableEntryIndex[i]->high))
    {
      return i;
    }
  }
  return -1;
}

void 
Ipv4GlobalRouting::GetMappingTableEntry(ident pathNodeIdentA,ident pathNodeIdentB,vector<struct mappingtableentry> *tempMappingTable)
{
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
    int index=GetMappingTableEntryIndex(high);
    struct mappingtableentry *tempMappingTableEntry=mappingTableEntryIndex[index];
    while (tempMappingTableEntry!=NULL && SameNode(high,tempMappingTableEntry->high))
    {
      if (SameNode(high,tempMappingTableEntry->high) && SameNode(low,tempMappingTableEntry->low))
      {
        tempMappingTable->push_back(*tempMappingTableEntry);
        break;
      }
      tempMappingTableEntry=tempMappingTableEntry->next;
    }
  }
  else// 按照单个结点来获取映射表项，只能遍历
  {
    struct mappingtableentry *tempMappingTableEntry=headMappingTableEntry->next;
    while (tempMappingTableEntry!=NULL)
    {
      if (SameNode(pathNodeIdentA,tempMappingTableEntry->high) || SameNode(pathNodeIdentA,tempMappingTableEntry->low))
      {
        tempMappingTable->push_back(*tempMappingTableEntry);
      }
      tempMappingTableEntry=tempMappingTableEntry->next;
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
  string gateAddr=inet_ntoa(tempNextHopAndWeight.gateAddr.sin_addr);
  if (gateAddr==inet_ntoa(destAddr.sin_addr)) req.r.rtm_scope=RT_SCOPE_LINK;//
  else req.r.rtm_scope=RT_SCOPE_UNIVERSE;//
  req.r.rtm_type=RTN_UNICAST;//
  req.r.rtm_dst_len=prefixLen;//
  
  // mtu没设置
  int bytelen=(req.r.rtm_family==AF_INET)?4:16;

  destAddr.sin_addr.s_addr=Convert(destAddr,prefixLen);
  addattr_l(&req.n,sizeof(req),RTA_DST,&(destAddr.sin_addr.s_addr),bytelen);//目的地址
  addattr32(&req.n,sizeof(req),RTA_PRIORITY,NL_DEFAULT_ROUTE_METRIC);//metric
  addattr_l(&req.n,sizeof(req),RTA_GATEWAY,&(tempNextHopAndWeight.gateAddr.sin_addr.s_addr),bytelen);//网关，单路径好像不要加上网关
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
  // Logfout << "destAddr:" << inet_ntoa(destAddr.sin_addr) << ",prefixLen:" << prefixLen << endl;
  // for (int i=0;i<nextHopAndWeight.size();i++)
  // {
  //   Logfout << "NICName:" << nextHopAndWeight[i].NICName << " weight:" << nextHopAndWeight[i].weight << endl;
  // }
  // end

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
    struct mappingtableentry *tempMappingTableEntry=headMappingTableEntry->next;

    while (tempMappingTableEntry!=NULL)
    {
      if (SameNode(high,tempMappingTableEntry->high))
      {
        if (SameNode(low,tempMappingTableEntry->low))//找到了该映射表项
        {
          isFindMapping=true;
          // 首先判断两条路径的长度
          if (newPathTableEntry->nodeCounter<tempMappingTableEntry->address->nodeCounter)//一定要更新
          {
            tempMappingTableEntry->address=newPathTableEntry;
            break;
          }
          else if (newPathTableEntry->nodeCounter==tempMappingTableEntry->address->nodeCounter)
          {
            // 长度相等，则继续比较其他的结点
            // 先从前面开始比较
            bool isNeedCompareAgain=true;
            ident tempNewPathEntryIdent,tempMapPathEntryIdent;
            for (int k=1;k<i;k++)
            {
              tempNewPathEntryIdent=newPathTableEntry->pathNodeIdent[k];
              tempMapPathEntryIdent=tempMappingTableEntry->address->pathNodeIdent[k];

              if (tempNewPathEntryIdent.level<tempMapPathEntryIdent.level)//一定要更新
              {
                tempMappingTableEntry->address=newPathTableEntry;
                isNeedCompareAgain=false;
                break;
              }
              else if (tempNewPathEntryIdent.level==tempMapPathEntryIdent.level)
              {
                // 继续比较position
                if (tempNewPathEntryIdent.position<tempMapPathEntryIdent.position)//一定要更新
                {
                  tempMappingTableEntry->address=newPathTableEntry;
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
                tempMapPathEntryIdent=tempMappingTableEntry->address->pathNodeIdent[k];

                if (tempNewPathEntryIdent.level<tempMapPathEntryIdent.level)//一定要更新
                {
                  tempMappingTableEntry->address=newPathTableEntry;
                  break;
                }
                else if (tempNewPathEntryIdent.level==tempMapPathEntryIdent.level)
                {
                  // 继续比较position
                  if (tempNewPathEntryIdent.position<tempMapPathEntryIdent.position)//一定要更新
                  {
                    tempMappingTableEntry->address=newPathTableEntry;
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
          else if (newPathTableEntry->nodeCounter>tempMappingTableEntry->address->nodeCounter)//一定不要更新
          {
            break;
          }
          break;
        }
      }
      tempMappingTableEntry=tempMappingTableEntry->next;
    }

    if (isFindMapping==false)// 不存在，需要添加
    {
      struct mappingtableentry *currentMappingTableEntry=headMappingTableEntry;
      struct mappingtableentry *nextMappingTableEntry=headMappingTableEntry->next;
      struct mappingtableentry *newMappingTableEntry=(struct mappingtableentry *)malloc(sizeof(struct mappingtableentry));
      newMappingTableEntry->high=high;
      newMappingTableEntry->low=low;
      newMappingTableEntry->address=newPathTableEntry;
      newMappingTableEntry->next=NULL;
      do 
      {
        if (nextMappingTableEntry==NULL)
        {
          InsertMappingTable(currentMappingTableEntry,nextMappingTableEntry,newMappingTableEntry);
          isFindMapping=true;
          break;
        }
        else if (newMappingTableEntry->high.level<nextMappingTableEntry->high.level)
        {
          InsertMappingTable(currentMappingTableEntry,nextMappingTableEntry,newMappingTableEntry);
          isFindMapping=true;
          break;
        }
        else if (newMappingTableEntry->high.level==nextMappingTableEntry->high.level)
        {
          if (newMappingTableEntry->high.position<nextMappingTableEntry->high.position)
          {
            InsertMappingTable(currentMappingTableEntry,nextMappingTableEntry,newMappingTableEntry);
            isFindMapping=true;
            break;
          }
          else if (newMappingTableEntry->high.position==nextMappingTableEntry->high.position)
          {
            if (newMappingTableEntry->low.level<nextMappingTableEntry->low.level)
            {
              InsertMappingTable(currentMappingTableEntry,nextMappingTableEntry,newMappingTableEntry);
              isFindMapping=true;
              break;
            }
            else if (newMappingTableEntry->low.level==nextMappingTableEntry->low.level)
            {
              if (newMappingTableEntry->low.position<nextMappingTableEntry->low.position)
              {
                InsertMappingTable(currentMappingTableEntry,nextMappingTableEntry,newMappingTableEntry);
                isFindMapping=true;
                break;
              }
            }
          }
        }
        currentMappingTableEntry=currentMappingTableEntry->next;
        nextMappingTableEntry=nextMappingTableEntry->next;
      }while (1);
      PrintMappingTableIndex();
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

  // 更新映射表
  if (newPathTableEntry->nodeCounter!=1) UpdateMappingTableEntry(newPathTableEntry);

  // 插入路径表
  currentPathTableEntry->next=newPathTableEntry;
  newPathTableEntry->next=nextPathTableEntry;
  headPathTableEntry->linkCounter++;
  if (newPathTableEntry->nodeCounter!=1) InsertNodeInDirPathTable(newPathTableEntry);// 避免将自己添加进去

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
  Logfout.close();
}

void 
Ipv4GlobalRouting::InsertMappingTable(struct mappingtableentry *currentMappingTableEntry,struct mappingtableentry *nextMappingTableEntry,struct mappingtableentry *newMappingTableEntry)
{
  currentMappingTableEntry->next=newMappingTableEntry;
  newMappingTableEntry->next=nextMappingTableEntry;
  bool isFindIndex=false;
  for (int i=0;i<mappingTableEntryIndex.size();i++)
  {
    if (SameNode(mappingTableEntryIndex[i]->high,newMappingTableEntry->high))
    {
      // 比较他们的low
      isFindIndex=true;
      if (newMappingTableEntry->low.level<mappingTableEntryIndex[i]->low.level)
      {
        mappingTableEntryIndex[i]=newMappingTableEntry;
        break;
      }
      else if (newMappingTableEntry->low.level==mappingTableEntryIndex[i]->low.level)
      {
        if (newMappingTableEntry->low.position<mappingTableEntryIndex[i]->low.position)
        {
          mappingTableEntryIndex[i]=newMappingTableEntry;
          break;
        }
      }
    }
  }
  if (isFindIndex==false) mappingTableEntryIndex.push_back(newMappingTableEntry);
}

void 
Ipv4GlobalRouting::PathAddNewAddr(struct pathtableentry *nextPathTableEntry,struct pathaddrset *tempPathAddrSet)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (nextPathTableEntry->nodeCounter==1)// 本机（也是一个tor）发现了新的服务器
  {
    for (int i=0;i<MAX_ADDR_NUM;i++)
    {
      if (strcmp(inet_ntoa(tempPathAddrSet->addrSet[i].addr.sin_addr),"255.255.255.255")) // 在新的addrset中添加新的有效的服务器地址
      {
        vector<struct nexthopandweight> nextHopAndWeight;
        struct nexthopandweight tempNextHopAndWeight;
        tempNextHopAndWeight.NICName=GetNICNameByRemoteAddr(tempPathAddrSet->addrSet[i].addr); 
        GetLocalAddrByRemoteAddr(&(tempNextHopAndWeight.srcAddr),tempPathAddrSet->addrSet[i].addr);
        tempNextHopAndWeight.gateAddr=tempPathAddrSet->addrSet[i].addr;
        tempNextHopAndWeight.weight=1;
        nextHopAndWeight.push_back(tempNextHopAndWeight);
        AddSingleRoute(tempPathAddrSet->addrSet[i].addr,tempPathAddrSet->addrSet[i].prefixLen,nextHopAndWeight[0]);
        for (int j=0;j<MAX_ADDR_NUM;j++)
        {
          if (!strcmp(inet_ntoa(nextPathTableEntry->pathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255"))
          {
            nextPathTableEntry->pathAddrSet->addrSet[j]=tempPathAddrSet->addrSet[i];
            break;
          }
        }
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
          //相同的地址直接退出
          if (!strcmp(inet_ntoa(nextPathAddrSet->addrSet[j].addr.sin_addr),tempAddr.c_str())) break;
          if (!strcmp(inet_ntoa(nextPathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255"))
          {
            nextPathAddrSet->addrSet[j]=tempPathAddrSet->addrSet[i];
            vector<struct addrset> addrSet;
            addrSet.push_back(tempPathAddrSet->addrSet[i]);
            ident tempIdent;
            tempIdent.level=-1;
            tempIdent.position=-1;
            UpdateAddrSet(nextPathTableEntry->pathNodeIdent[nextPathTableEntry->nodeCounter-1],tempIdent,nextPathTableEntry->nodeCounter,addrSet);
            break;
          }
        }
      }
      else break;
    }
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::AddNewPathTableEntry(struct pathinfo tempPathInfo)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  
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
        if (nextPathTableEntry!=NULL && nextPathTableEntry->nodeCounter==1) PathAddNewAddr(nextPathTableEntry,newPathTableEntry->pathAddrSet);//找到了一条完全相同的路径
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
}

void 
Ipv4GlobalRouting::PrintNodeInDirPathTable()//打印备用路径
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/NodeInDirPathTable-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Node" << myIdent.level << "." << myIdent.position << "'s NodeInDirPathTable" << endl;
  Logfout << "Node\t" << "Node\t" << "Node\t" << "Node\t" << "Node\t";
  Logfout << "\t\tlinkCounter\t";
  Logfout << "dirConFlag\t";
  Logfout << endl;

  for (int i=0;i<nodeInDirPathTable.size();i++)
  {
    for (int j=0;j<nodeInDirPathTable[i]->nodeCounter;j++)
    {
      Logfout << nodeInDirPathTable[i]->pathNodeIdent[j].level << "." << nodeInDirPathTable[i]->pathNodeIdent[j].position << "\t";
    }
    for (int j=nodeInDirPathTable[i]->nodeCounter;j<MAX_PATH_LEN;j++) Logfout << "\t";
    Logfout << "[" << inet_ntoa(nodeInDirPathTable[i]->nodeAddr.addr.sin_addr) << "]";
    Logfout << "\t" << nodeInDirPathTable[i]->linkCounter;
    Logfout << "\t\t" << nodeInDirPathTable[i]->dirConFlag;
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
  Logfout << "MasterAddr\tMasterIdent\tMasterSock\tNICName\tchiefMaster\tDirect\tMiddleAddr\tKeepAliveFaildNum\tKeepAliveFlag\tisStartKeepAlive" << endl;
  for (int i=0;i<masterMapToSock.size();i++)
  {
    Logfout << masterMapToSock[i].masterAddr << "\t" << masterMapToSock[i].masterIdent.level << "." << masterMapToSock[i].masterIdent.position << "\t\t";
    Logfout << masterMapToSock[i].masterSock << "\t\t" << masterMapToSock[i].NICName << "\t" << masterMapToSock[i].chiefMaster << "\t\t" << masterMapToSock[i].direct << "\t";
    if (masterMapToSock[i].middleAddr=="") Logfout << "\t";
    else Logfout << masterMapToSock[i].middleAddr;
    Logfout << "\t" << masterMapToSock[i].keepAliveFaildNum << "\t\t\t" << masterMapToSock[i].recvKeepAlive << "\t" << masterMapToSock[i].isStartKeepAlive << endl;
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::InsertNodeInDirPathTable(struct pathtableentry *tempPathTableEntry)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isSameNextHop=false;
  for (int i=0;i<nodeInDirPathTable.size();i++)
  {
    if (SameNode(tempPathTableEntry->pathNodeIdent[1],(nodeInDirPathTable[i])->pathNodeIdent[1]))// 判断下一跳是否相同
    {
      isSameNextHop=true;
      if (tempPathTableEntry->nodeCounter<(nodeInDirPathTable[i])->nodeCounter)// 更短
      {
        nodeInDirPathTable[i]=tempPathTableEntry;
        PrintNodeInDirPathTable();
        break;
      }
    }
  }
  if (isSameNextHop==false)// 没有找到相同的下一跳就添加
  {
    nodeInDirPathTable.push_back(tempPathTableEntry);
    PrintNodeInDirPathTable();
  }
  
  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateNodeInDirPathTable()// 修改路径表或者直连标志位后，需要检查间接路径表是否需要更新
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  for (int i=0;i<nodeInDirPathTable.size();i++)
  {
    // 已经是此类型下一跳中最佳备用路径
    if (nodeInDirPathTable[i]->linkCounter==0 && nodeInDirPathTable[i]->dirConFlag==true && nodeInDirPathTable[i]->nodeCounter==2)
    {
      continue;
    }
    // 可用，但不是最佳路径
    else if (nodeInDirPathTable[i]->linkCounter==0 && nodeInDirPathTable[i]->dirConFlag==true && nodeInDirPathTable[i]->nodeCounter!=2)
    {
      vector<struct mappingtableentry> tempMappingTable;
      GetMappingTableEntry(myIdent,nodeInDirPathTable[i]->pathNodeIdent[1],&tempMappingTable);// 获取相关的映射表
      struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
      for(int j=0;j<tempMappingTable.size();j++)//理论上应该只有一条映射表项
      {
        tempPathTableEntry=tempMappingTable[j].address;
        while (tempPathTableEntry!=NULL && SameNode(nodeInDirPathTable[i]->pathNodeIdent[1],tempPathTableEntry->pathNodeIdent[1]))// 不到结尾且有相同的下一跳
        {
          if (tempPathTableEntry->linkCounter==0 && tempPathTableEntry->dirConFlag==true && tempPathTableEntry->nodeCounter<nodeInDirPathTable[i]->nodeCounter)
          {
            nodeInDirPathTable[i]=tempPathTableEntry;
            PrintNodeInDirPathTable();
            break;
          }
          tempPathTableEntry=tempPathTableEntry->next;
        }
      }
    }
    // 备用路径不可用
    else if (nodeInDirPathTable[i]->linkCounter!=0 || nodeInDirPathTable[i]->dirConFlag!=true)
    {
      vector<struct mappingtableentry> tempMappingTable;
      GetMappingTableEntry(myIdent,nodeInDirPathTable[i]->pathNodeIdent[1],&tempMappingTable);// 获取相关的映射表
      struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
      for(int j=0;j<tempMappingTable.size();j++)//理论上应该只有一条映射表项
      {
        tempPathTableEntry=tempMappingTable[j].address;
        while (tempPathTableEntry!=NULL && SameNode(nodeInDirPathTable[i]->pathNodeIdent[1],tempPathTableEntry->pathNodeIdent[1]))// 不到结尾且有相同的下一跳
        {
          if (tempPathTableEntry->linkCounter==0 && tempPathTableEntry->dirConFlag==true)
          {
            nodeInDirPathTable[i]=tempPathTableEntry;
            PrintNodeInDirPathTable();
            break;
          }
          tempPathTableEntry=tempPathTableEntry->next;
        }
      }
    }
  }
  PrintNodeInDirPathTable();
  Logfout.close();
}

void 
Ipv4GlobalRouting::ModifyNodeDirConFlag(ident high,ident low,bool dirConFlag)// 修改路径表，修改dirConFlag
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (!SameNode(high,myIdent))// 不需要修改自身
  {
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
    UpdateNodeInDirPathTable();
    PrintPathEntryTable();
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
    Logfout << GetNow() << "Didn't find mapping entry." << endl;
    return;
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
        if (!strcmp(inet_ntoa(tempPathTableEntry->pathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255")) break;
        tempAddrSet.push_back(tempPathTableEntry->pathAddrSet->addrSet[j]);// 地址合法则添加
      }
      tempDestIdent=tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1];// 记录目的节点
      tempNextIdent=tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-2];// 记录倒数第二个节点
    }

    while (tempPathTableEntry!=NULL)
    {
      // 如果有相同的链路则修改linkCounter
      if (SameNode(high,tempPathTableEntry->pathNodeIdent[locA]) && SameNode(low,tempPathTableEntry->pathNodeIdent[locB]))
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
            UpdateAddrSet(tempDestIdent,tempNextIdent,tempNodeCounter,tempAddrSet);
          }

          tempNodeCounter=tempPathTableEntry->nodeCounter;
          tempNodeAddr=tempPathTableEntry->nodeAddr;// 记录节点地址
          tempAddrSet.clear();
          for (int j=0;j<MAX_ADDR_NUM;j++)// 记录服务器地址
          {
            if (!strcmp(inet_ntoa(tempPathTableEntry->pathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255")) break;
            tempAddrSet.push_back(tempPathTableEntry->pathAddrSet->addrSet[j]);// 地址合法则添加
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
  UpdateNodeInDirPathTable();
  PrintPathEntryTable();
  Logfout.close();
}

/**************************Node**************************/


/**************************Master**************************/
void 
Ipv4GlobalRouting::PrintClusterMasterInfo()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/ClusterMasterInfo-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "My inDirNodeNum:" << inDirNodeNum << endl;
  Logfout << "MasterIdent\tMasterAddr\tchiefMaster\tinDirNodeNum" << endl;
  for (int i=0;i<clusterMasterInfo.size();i++)
  {
    Logfout << clusterMasterInfo[i].masterIdent.level << "." << clusterMasterInfo[i].masterIdent.position << "\t\t";
    Logfout << inet_ntoa(clusterMasterInfo[i].masterAddr.sin_addr) << "\t" << clusterMasterInfo[i].chiefMaster << "\t\t" << clusterMasterInfo[i].inDirNodeNum << endl;
  }
  Logfout.close();
}

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

  Logfout << "Node\tSock\tDirect" << endl;
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    Logfout << nodeMapToSock[i].nodeIdent.level << "." << nodeMapToSock[i].nodeIdent.position << "\t" << nodeMapToSock[i].nodeSock << "\t" << nodeMapToSock[i].direct << endl;
  }
  Logfout << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintMasterInDirPathTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/MasterInDirPathTable-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Node\tinDirPath\t" << endl;
  for (int i=0;i<masterInDirPathTable.size();i++)
  {
    Logfout << masterInDirPathTable[i].pathNodeIdent[0].level << "." << masterInDirPathTable[i].pathNodeIdent[0].position << "\t";
    for (int j=0;j<MAX_PATH_LEN;j++)
    {
      if (masterInDirPathTable[i].pathNodeIdent[j].level!=-1) 
      {
        if (j!=0) Logfout << "--";
        Logfout << masterInDirPathTable[i].pathNodeIdent[j].level << "." << masterInDirPathTable[i].pathNodeIdent[j].position;
      }
      else Logfout << "\t";
    }
    Logfout << endl;
  }

  Logfout.close();
}

void* 
Ipv4GlobalRouting::OldChiefFindNewOneThread(void* threadParam)
{
  // pthread_detach(pthread_self());
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamB *)threadParam)->tempGlobalRouting;
  TCPRoute *tempTCPRoute=((struct threadparamB *)threadParam)->tempTCPRoute;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->myIdent.level << "." << tempGlobalRouting->myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  do
  {
    sleep(3);
    if (tempGlobalRouting->chiefMasterStatusChange==false) 
    {
      tempGlobalRouting->chiefMasterStatusChange=true;
      continue;
    }
    else break;
  }while (1);

  // // chiefMaster询问其他Master是否更优
  struct MNinfo tempMNInfo;
  tempMNInfo.addr.sin_family=AF_INET;
  inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
  tempMNInfo.addr.sin_port=htons(0);
  tempGlobalRouting->GetAddrByNICName(&(tempMNInfo.addr),"eth0");
  tempMNInfo.srcIdent=tempGlobalRouting->myIdent;
  tempMNInfo.pathNodeIdentA.level=-1;// 全部置为-1表示当前的chief Master决定放弃chief Master的地位
  tempMNInfo.pathNodeIdentA.position=-1;
  tempMNInfo.pathNodeIdentB.level=tempGlobalRouting->inDirNodeNum;
  tempMNInfo.pathNodeIdentB.position=tempGlobalRouting->inDirNodeNum;
  tempMNInfo.clusterMaster=true;
  tempMNInfo.chiefMaster=false;
  tempMNInfo.keepAlive=false;
  tempMNInfo.linkFlag=false;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;
  // 理想做法是Chief master询问其他common，待common做出反应后再决定是否放弃
  // 但是现在是不询问，直接放弃当老大
  Logfout << GetNow() << "I am looking for a better master!" << endl;
  for (int i=0;i<tempGlobalRouting->nodeMapToSock.size();i++)
  {
    if (tempGlobalRouting->nodeMapToSock[i].nodeIdent.level==0)// 发给common master
    {
      tempMNInfo.destIdent=tempGlobalRouting->nodeMapToSock[i].nodeIdent;
      tempTCPRoute->SendMessageTo(tempGlobalRouting->GetSockByIdent(tempGlobalRouting->nodeMapToSock[i].nodeIdent),tempMNInfo);
    }
    else break;
  }
  tempGlobalRouting->isStartMasterElection=false;
  Logfout.close();
  // pthread_exit(0);
}

void* 
Ipv4GlobalRouting::ListenKeepAliveThread(void* threadParam)
{
  // pthread_detach(pthread_self());
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamA *)threadParam)->tempGlobalRouting;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->myIdent.level << "." << tempGlobalRouting->myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isEnd=false;
  while (1)
  {
    // 每遍历检查一次，keepAliveFaildNum++；每收到一个keep alive，keepAliveFaildNum清零；keepAliveFaildNum>=3，连接失效，回退BGP
    for (int i=0;i<tempGlobalRouting->nodeMapToSock.size();i++)
    {
      tempGlobalRouting->nodeMapToSock[i].keepAliveFaildNum++;
      if (tempGlobalRouting->nodeMapToSock[i].keepAliveFaildNum>3) 
      {
        Logfout << GetNow() << "Can not connect with node " << tempGlobalRouting->nodeMapToSock[i].nodeIdent.level << "." << tempGlobalRouting->nodeMapToSock[i].nodeIdent.position << "." << endl;
        if (tempGlobalRouting->chiefMaster) 
        {
          Logfout << GetNow() << "Go back to BGP." << endl;
          struct MNinfo endMNInfo;
          endMNInfo.addr.sin_family=AF_INET;
          inet_aton("255.255.255.255",&(endMNInfo.addr.sin_addr));
          endMNInfo.addr.sin_port=htons(0);
          tempGlobalRouting->GetAddrByNICName(&(endMNInfo.addr),"eth0");
          endMNInfo.destIdent.level=-1;// 此时还不知道master的ident
          endMNInfo.destIdent.position=-1;
          endMNInfo.srcIdent=tempGlobalRouting->myIdent;
          endMNInfo.pathNodeIdentA=tempGlobalRouting->myIdent;
          endMNInfo.pathNodeIdentB=tempGlobalRouting->myIdent;
          endMNInfo.clusterMaster=false;
          endMNInfo.chiefMaster=tempGlobalRouting->chiefMaster;
          endMNInfo.keepAlive=false;
          endMNInfo.linkFlag=false;
          endMNInfo.hello=false;
          endMNInfo.ACK=false;
        }
        isEnd=true;
        break;
      }
    }
    if (isEnd==true) break;
    sleep(tempGlobalRouting->m_defaultKeepaliveTimer);
  }
  Logfout.close();
  // pthread_exit(0);
}

void* 
Ipv4GlobalRouting::MasterLinkTimer(void* threadParam)
{
  // pthread_detach(pthread_self());
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
  Logfout << GetNow() << "Master link timer timed out,system time:" << tempGlobalRouting->GetSystemTime() << "s." << endl;

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
  // pthread_exit(0);
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
          // pthread_detach(masterLink_thread);
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
        if (newLinkTableEntry->high.level>nextLinkTableEntry->high.level)//level越大，排序越靠前
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
Ipv4GlobalRouting::NewChiefMasterElection(ident chiefMasterIdent)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (chiefMaster)// old chief master receive message from new chief master 
  {
    // Logfout << GetNow() << "Try to connect with the new chiefMaster......" << endl;
    chiefMaster=false;
    m_tcpRoute->UpdateChiefMaster(false);
    sleep(5);// 等待新的chiefmaster做好准备
    vector<string> masterAddress;
    for (int i=0;i<clusterMasterInfo.size();i++)
    {
      if (SameNode(clusterMasterInfo[i].masterIdent,chiefMasterIdent))
      {
        masterAddress.push_back(inet_ntoa(clusterMasterInfo[i].masterAddr.sin_addr));
        break;
      }
    }
    
    m_tcpRoute->SendHelloToMaster(masterAddress,"","eth0");
    for (auto iter=nodeMapToSock.begin();iter!=nodeMapToSock.end();)
    {
      if (iter->nodeIdent.level==0)
      {
        iter=nodeMapToSock.erase(iter);// 删除该项           
        if (iter==nodeMapToSock.end()) break;
      }
      else break;
      iter++;
    }
    PrintNodeMapToSock();
  }
  else // common master选出新的chief master
  {
    // 先删除chief master
    for (auto iter=clusterMasterInfo.begin();iter!=clusterMasterInfo.end();)
    {
      if (SameNode((*iter).masterIdent,chiefMasterIdent))
      {
        clusterMasterInfo.erase(iter);// 删除该项           
        break;
      }
      iter++;
    }
    PrintClusterMasterInfo();

    struct sockaddr_in newTempMasterAddr;
    newTempMasterAddr.sin_family=AF_INET;
    inet_aton("255.255.255.255",&(newTempMasterAddr.sin_addr));
    newTempMasterAddr.sin_port=htons(0);
    GetAddrByNICName(&(newTempMasterAddr),"eth0");
    ident newTempChiefMasterIdent=myIdent;
    int newTempInDirNodeNum=inDirNodeNum;

    // 再找出一个新的最优master
    for (int i=0;i<clusterMasterInfo.size();i++)
    {
      if (clusterMasterInfo[i].inDirNodeNum<newTempInDirNodeNum)
      {
        newTempMasterAddr=clusterMasterInfo[i].masterAddr;
        newTempChiefMasterIdent=clusterMasterInfo[i].masterIdent;
        newTempInDirNodeNum=clusterMasterInfo[i].inDirNodeNum;
      }
      else if (clusterMasterInfo[i].inDirNodeNum==newTempInDirNodeNum)
      {
        if (clusterMasterInfo[i].masterIdent.position<newTempChiefMasterIdent.position)// 如果inDirNodeNum相等，则选择position小的
        {
          newTempMasterAddr=clusterMasterInfo[i].masterAddr;
          newTempChiefMasterIdent=clusterMasterInfo[i].masterIdent;
          newTempInDirNodeNum=clusterMasterInfo[i].inDirNodeNum;
        }
      }
    }

    int oldChiefMasterSock=0;
    for (int i=0;i<masterMapToSock.size();i++)
    {
      if (SameNode(masterMapToSock[i].masterIdent,chiefMasterIdent))
      {
        oldChiefMasterSock=masterMapToSock[i].masterSock;
        break;
      }
    }

    masterMapToSock.clear();
    clusterMasterInfo.clear();
    Logfout << GetNow() << "The new chiefMaster is " << newTempChiefMasterIdent.level << "." << newTempChiefMasterIdent.position << "(" << inet_ntoa(newTempMasterAddr.sin_addr) << ")." << endl;
    
    if (SameNode(myIdent,newTempChiefMasterIdent))// 如果选择的结果是自己最优
    {
      Logfout << GetNow() << "I am the new chiefMaster!" << endl;
      chiefMaster=true;
      m_tcpRoute->UpdateChiefMaster(true);

      struct MNinfo tempMNInfo;
      tempMNInfo.addr.sin_family=AF_INET;
      inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
      tempMNInfo.addr.sin_port=htons(0);
      GetAddrByNICName(&(tempMNInfo.addr),"eth0");
      tempMNInfo.srcIdent=myIdent;
      tempMNInfo.chiefMaster=true;
      tempMNInfo.keepAlive=false;
      tempMNInfo.linkFlag=false;
      tempMNInfo.hello=false;     

      // 先通知old chief master
      tempMNInfo.pathNodeIdentA.level=-1;
      tempMNInfo.pathNodeIdentA.position=-1;
      tempMNInfo.pathNodeIdentB.level=inDirNodeNum;
      tempMNInfo.pathNodeIdentB.position=inDirNodeNum;
      tempMNInfo.clusterMaster=true;
      tempMNInfo.ACK=false;
      tempMNInfo.destIdent=chiefMasterIdent;
      m_tcpRoute->SendMessageTo(oldChiefMasterSock,tempMNInfo);

      // 通知所有的node，chiefmaster已经发生变化，即发送一个ACK包
      tempMNInfo.pathNodeIdentA=myIdent;
      tempMNInfo.pathNodeIdentB=myIdent; 
      tempMNInfo.clusterMaster=false;
      tempMNInfo.ACK=true;

      for (int i=0;i<nodeMapToSock.size();i++)
      {
        tempMNInfo.destIdent=nodeMapToSock[i].nodeIdent;
        m_tcpRoute->SendMessageTo(nodeMapToSock[i].nodeSock,tempMNInfo);
        Logfout << GetNow() << "Inform node " << nodeMapToSock[i].nodeIdent.level << "." << nodeMapToSock[i].nodeIdent.position << "." << endl;
      }
    }
    else // 其他master最优
    {
      m_tcpRoute->SendHelloToChief(inet_ntoa(newTempMasterAddr.sin_addr),"eth0");
    }
  }
  Logfout.close();
}

void
Ipv4GlobalRouting::UpdateInDirPath(ident nodeIdent,ident pathNodeIdentA,ident pathNodeIdentB,int nodeSock)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  ident tempNode;
  tempNode.level=-1;
  tempNode.position=-1;
          
  bool isFind=false;
  for (int i=0;i<masterInDirPathTable.size();i++)
  {
    if (SameNode(masterInDirPathTable[i].pathNodeIdent[0],nodeIdent))// 判断某个结点的间接路径是否存在
    {
      isFind=true;
      // 通过sock来判断间接连接是否发生过变化
      if (masterInDirPathTable[i].nodeSock!=nodeSock)
      {
        // 先全部清空间接路径，再添加
        for (int j=0;j<MAX_PATH_LEN;j++) masterInDirPathTable[i].pathNodeIdent[j]=tempNode;
        masterInDirPathTable[i].nodeSock=nodeSock;
        if (SameNode(pathNodeIdentA,nodeIdent)) 
        {
          masterInDirPathTable[i].pathNodeIdent[0]=pathNodeIdentA;
          masterInDirPathTable[i].pathNodeIdent[1]=pathNodeIdentB;
        }
        // 检查是否有积压的未发送的MNInfo
        struct accumulationMNinfo *currentAccumulationMNInfo=masterInDirPathTable[i].headAccumulationMNInfo;
        while (currentAccumulationMNInfo->next!=NULL) 
        {
          currentAccumulationMNInfo=currentAccumulationMNInfo->next;
          // Logfout << "send later" << endl;
          m_tcpRoute->SendMessageTo(masterInDirPathTable[i].nodeSock,*(currentAccumulationMNInfo->tempMNInfo));
          // Logfout << "send later over" << endl;
        }
        masterInDirPathTable[i].headAccumulationMNInfo->next=NULL;
      }
      else 
      {
        for (int j=0;j<MAX_PATH_LEN;j++)
        {
          if (masterInDirPathTable[i].pathNodeIdent[j].level==-1) 
          {
            masterInDirPathTable[i].pathNodeIdent[j]=pathNodeIdentB;
            break;
          }
        }
      }
      break;
    }
  }

  if (isFind==false) 
  {
    struct indirpathtableentry *tempInDirPathTableEntry=(struct indirpathtableentry *)malloc(sizeof(struct indirpathtableentry));
    for (int i=0;i<MAX_PATH_LEN;i++) tempInDirPathTableEntry->pathNodeIdent[i]=tempNode;
    if (SameNode(pathNodeIdentA,nodeIdent)) 
    {
      tempInDirPathTableEntry->pathNodeIdent[0]=pathNodeIdentA;
      tempInDirPathTableEntry->pathNodeIdent[1]=pathNodeIdentB;
    }
    tempInDirPathTableEntry->nodeSock=nodeSock;
    struct accumulationMNinfo *tempAccumulationMNInfo=(struct accumulationMNinfo *)malloc(sizeof(struct accumulationMNinfo));
    tempAccumulationMNInfo->tempMNInfo=NULL;
    tempAccumulationMNInfo->next=NULL;
    tempInDirPathTableEntry->headAccumulationMNInfo=tempAccumulationMNInfo;
    masterInDirPathTable.push_back(*tempInDirPathTableEntry);
  }
  PrintMasterInDirPathTable();
  Logfout.close();
}

void
Ipv4GlobalRouting::UpdateNodeMapToSock(ident nodeIdent,int sock,bool direct)// master和起转发的node会使用
{
  // 只有master和node建立直连后才会进行以下两步操作
  // 向该node下发所有已和master连接的node
  // 向所有已经和master连接的node下发，有新的node和master连接上了
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 先判断是否存在
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    if (SameNode(nodeIdent,nodeMapToSock[i].nodeIdent))// 存在，则覆盖
    {
      // 判断 状态
      if (myIdent.level==0 && nodeMapToSock[i].nodeSock!=sock)// master，且新的套接字不能和以前的相等
      {
        // 如果之前是间接连接，现在是直连，则inDirNodeNum--；如果之前是直连，现在是间接连接，则inDirNodeNum++；
        if (nodeMapToSock[i].direct==false && direct==true) 
        {
          if (nodeIdent.level!=0 && chiefMaster==true) SendMessageToNode(nodeIdent,myIdent,direct);
          inDirNodeNum--;

          struct clustermasterinfo tempClusterMasterInfo;
          tempClusterMasterInfo.chiefMaster=chiefMaster;// 
          tempClusterMasterInfo.masterAddr.sin_family=AF_INET;
          inet_aton("255.255.255.255",&(tempClusterMasterInfo.masterAddr.sin_addr));
          tempClusterMasterInfo.masterAddr.sin_port=htons(0);
          GetAddrByNICName(&(tempClusterMasterInfo.masterAddr),"eth0");// common master的地址
          tempClusterMasterInfo.masterIdent=myIdent;// common master的ident
          tempClusterMasterInfo.inDirNodeNum=inDirNodeNum;
          // 主动发出indirnodenum
          SendInDirNodeNumToCommon(tempClusterMasterInfo);
        }
        else if (nodeMapToSock[i].direct==true && direct==false) 
        {
          shutdown(nodeMapToSock[i].nodeSock,SHUT_RDWR);
          if (nodeIdent.level!=0 && chiefMaster==true) SendMessageToNode(nodeIdent,myIdent,direct);
          inDirNodeNum++;
          // test
          // if (chiefMaster) inDirNodeNum+=3;
          // end

          struct clustermasterinfo tempClusterMasterInfo;
          tempClusterMasterInfo.chiefMaster=chiefMaster;//
          tempClusterMasterInfo.masterAddr.sin_family=AF_INET;
          inet_aton("255.255.255.255",&(tempClusterMasterInfo.masterAddr.sin_addr));
          tempClusterMasterInfo.masterAddr.sin_port=htons(0); 
          GetAddrByNICName(&(tempClusterMasterInfo.masterAddr),"eth0");// common master的地址
          tempClusterMasterInfo.masterIdent=myIdent;// common master的ident
          tempClusterMasterInfo.inDirNodeNum=inDirNodeNum;
          // 主动发出indirnodenum
          SendInDirNodeNumToCommon(tempClusterMasterInfo);
        }
        Logfout << GetNow() << "My inDirNodeNum:" << inDirNodeNum << endl;

        nodeMapToSock[i].direct=direct;
        nodeMapToSock[i].nodeSock=sock;
        nodeMapToSock[i].keepAliveFaildNum=0;
        nodeMapToSock[i].recvKeepAlive=false;
        PrintNodeMapToSock();

        // 判断是否为最优的Chief Master
        if (inDirNodeNum>MAX_INDIR_NUM)
        {
          if (chiefMaster==true)
          {
            Logfout << GetNow() << "I may be not the chiefMaster!" << endl;
            // chiefMasterStatusChange=false;// 标志位，chief master判断出自己不是最优后，会等待3s再询问其他master，在这期间如果chief master的状态发生变化，则还要等待3s
            if (isStartMasterElection==false)
            {
              threadparamB *threadParam=new threadparamB();
              threadParam->tempGlobalRouting=this;
              threadParam->tempUdpClient=NULL;
              threadParam->tempTCPRoute=m_tcpRoute;

              if (pthread_create(&oldChiefFindNewOne_thread,NULL,OldChiefFindNewOneThread,(void *)threadParam)<0)
              {
                Logfout << GetNow() << "Create OldChiefFindNewOneThread failed!" << endl;
                exit(0);
              }
              // pthread_detach(oldChiefFindNewOne_thread);
              isStartMasterElection=true;
              chiefMasterStatusChange=true;
            }
            else chiefMasterStatusChange=false;
          }
        }
      }
      Logfout.close();
      return;
    }
  }

  struct nodemaptosock tempNodeMapToSock;
  tempNodeMapToSock.nodeIdent=nodeIdent;
  tempNodeMapToSock.nodeSock=sock;
  tempNodeMapToSock.direct=direct;
  tempNodeMapToSock.keepAliveFaildNum=0;
  tempNodeMapToSock.recvKeepAlive=false;
  nodeMapToSock.push_back(tempNodeMapToSock);

  // 如果是第一次和Master建立连接
  // 先向所有已经建立连接了的Node转发这个新的连接
  // 然后再向新建立连接的Node转发之前已经和Master建立了的连接
  // 可以同时进行
  if (nodeIdent.level!=0 && chiefMaster==true) 
  {
    struct MNinfo tempMNInfo;
    tempMNInfo.addr.sin_family=AF_INET;// 此时addr无实际意义
    inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
    tempMNInfo.addr.sin_port=htons(0);
    tempMNInfo.srcIdent=myIdent;
    tempMNInfo.pathNodeIdentA=nodeIdent;
    tempMNInfo.pathNodeIdentB=myIdent;
    tempMNInfo.clusterMaster=false;
    tempMNInfo.chiefMaster=false;
    tempMNInfo.keepAlive=false;
    tempMNInfo.linkFlag=direct;
    tempMNInfo.hello=false;
    tempMNInfo.ACK=false;

    for (int i=0;i<nodeMapToSock.size();i++)
    {
      if (nodeMapToSock[i].nodeIdent.level!=0) 
      {
        // 先向已经建立连接了的Node转发这个新的连接
        tempMNInfo.destIdent=nodeMapToSock[i].nodeIdent;
        tempMNInfo.pathNodeIdentA=nodeIdent;
        m_tcpRoute->SendMessageTo(GetSockByIdent(nodeMapToSock[i].nodeIdent),tempMNInfo);
        // 然后再向新建立连接的Node转发之前已经和Master建立了的连接
        tempMNInfo.destIdent=nodeIdent;
        tempMNInfo.pathNodeIdentA=nodeMapToSock[i].nodeIdent;
        m_tcpRoute->SendMessageTo(sock,tempMNInfo);
      }
    }
  }

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
  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateKeepAliveFaildNum(ident nodeIdent)
{
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    if (SameNode(nodeMapToSock[i].nodeIdent,nodeIdent)) 
    {
      nodeMapToSock[i].keepAliveFaildNum=0;
      break;
    }
  }
}

void 
Ipv4GlobalRouting::SendInDirNodeNumToCommon(struct clustermasterinfo tempClusterMasterInfo)// 主动发出，不是转发
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  struct MNinfo tempMNInfo;
  tempMNInfo.addr=tempClusterMasterInfo.masterAddr;
  tempMNInfo.srcIdent=myIdent;
  tempMNInfo.pathNodeIdentA=tempClusterMasterInfo.masterIdent;// 
  tempMNInfo.pathNodeIdentB.level=tempClusterMasterInfo.inDirNodeNum;// pathNodeIdentB有效，值为indirnodenum
  tempMNInfo.pathNodeIdentB.position=tempClusterMasterInfo.inDirNodeNum;
  tempMNInfo.clusterMaster=true;
  tempMNInfo.chiefMaster=tempClusterMasterInfo.chiefMaster;
  tempMNInfo.keepAlive=false;
  tempMNInfo.linkFlag=false;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;

  if (chiefMaster)// chiefMaster转发给所有的common master
  {
    for (int i=0;i<nodeMapToSock.size();i++)
    {
      if (nodeMapToSock[i].nodeIdent.level==0 && !SameNode(nodeMapToSock[i].nodeIdent,tempClusterMasterInfo.masterIdent))// common且避免发给源
      {
        tempMNInfo.destIdent=nodeMapToSock[i].nodeIdent;
        m_tcpRoute->SendMessageTo(nodeMapToSock[i].nodeSock,tempMNInfo);
      }
      else if (nodeMapToSock[i].nodeIdent.level!=0) break;
    }
  }
  else // common转发给chiefmaster
  {
    for (int i=0;i<masterMapToSock.size();i++)
    {
      tempMNInfo.destIdent=masterMapToSock[i].masterIdent;
      m_tcpRoute->SendMessageTo(masterMapToSock[i].masterSock,tempMNInfo);
    }
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateClusterMasterInfo(struct clustermasterinfo tempClusterMasterInfo,int cmd)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (cmd==1)// 代表添加和更新
  {
    // 先判断相关信息是否存在
    for (int i=0;i<clusterMasterInfo.size();i++)
    {
      if (SameNode(clusterMasterInfo[i].masterIdent,tempClusterMasterInfo.masterIdent))// 存在则修改，否则添加
      {
        clusterMasterInfo[i].chiefMaster=tempClusterMasterInfo.chiefMaster;
        clusterMasterInfo[i].masterAddr=tempClusterMasterInfo.masterAddr;
        clusterMasterInfo[i].inDirNodeNum=tempClusterMasterInfo.inDirNodeNum;
        if (chiefMaster)// chief 要转发给其他common
        {
          SendInDirNodeNumToCommon(tempClusterMasterInfo);
        }
        PrintClusterMasterInfo();
        Logfout.close();
        return;
      }
    }
    // 不存在，则添加
    if (chiefMaster)
    {
      // chief 要先转发给其他common
      SendInDirNodeNumToCommon(tempClusterMasterInfo);

      // chief 再向新连接的master转发已有的indirnodenum信息
      struct MNinfo tempMNInfo;
      tempMNInfo.addr.sin_family=AF_INET;
      inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
      tempMNInfo.addr.sin_port=htons(0);
      tempMNInfo.destIdent=tempClusterMasterInfo.masterIdent;
      tempMNInfo.srcIdent=myIdent;
      tempMNInfo.clusterMaster=true;
      tempMNInfo.keepAlive=false;
      tempMNInfo.linkFlag=false;
      tempMNInfo.hello=false;
      tempMNInfo.ACK=false;

      int tempSock=GetSockByIdent(tempClusterMasterInfo.masterIdent);
      for (int i=0;i<clusterMasterInfo.size();i++)
      {
        tempMNInfo.addr=clusterMasterInfo[i].masterAddr;
        tempMNInfo.pathNodeIdentA=clusterMasterInfo[i].masterIdent;// 
        tempMNInfo.pathNodeIdentB.level=clusterMasterInfo[i].inDirNodeNum;// pathNodeIdentB有效，值为indirnodenum
        tempMNInfo.pathNodeIdentB.position=clusterMasterInfo[i].inDirNodeNum;
        tempMNInfo.chiefMaster=clusterMasterInfo[i].chiefMaster;
        m_tcpRoute->SendMessageTo(tempSock,tempMNInfo);
      }
      // 最后就是chief自己的indirnodenum
      GetAddrByNICName(&(tempMNInfo.addr),"eth0");
      tempMNInfo.pathNodeIdentA=myIdent;// 
      tempMNInfo.pathNodeIdentB.level=inDirNodeNum;// pathNodeIdentB有效，值为indirnodenum
      tempMNInfo.pathNodeIdentB.position=inDirNodeNum;
      tempMNInfo.chiefMaster=chiefMaster;
      
      m_tcpRoute->SendMessageTo(tempSock,tempMNInfo);
    }
    clusterMasterInfo.push_back(tempClusterMasterInfo);
    PrintClusterMasterInfo();
  }
  else if (cmd==-1)// 删除
  {
    // 
  }
  Logfout.close();
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

int 
Ipv4GlobalRouting::GetInDirNodeNum()
{
  return inDirNodeNum;
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

bool 
Ipv4GlobalRouting::IsUnreachableNode(ident tempNode,vector<ident> effectInDirNode,struct MNinfo tempMNInfo)// 判断Node是否为不可达的间接node
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  for (int i=0;i<effectInDirNode.size();i++)
  {
    if (SameNode(tempNode,effectInDirNode[i]))// 属于不可到达的间接连接的node
    {
      for (int j=0;j<masterInDirPathTable.size();j++)
      {
        if (SameNode(tempNode,masterInDirPathTable[i].pathNodeIdent[0]))
        {
          struct accumulationMNinfo *currentAccumulationMNInfo=masterInDirPathTable[i].headAccumulationMNInfo;
          while (currentAccumulationMNInfo->next!=NULL) currentAccumulationMNInfo=currentAccumulationMNInfo->next;
          struct accumulationMNinfo *newAccumulationMNInfo=(struct accumulationMNinfo *)malloc(sizeof(struct accumulationMNinfo));
          newAccumulationMNInfo->tempMNInfo=new MNinfo();
          *(newAccumulationMNInfo->tempMNInfo)=tempMNInfo;
          newAccumulationMNInfo->next=NULL;
          currentAccumulationMNInfo->next=newAccumulationMNInfo;
          Logfout.close();
          return true;
        }
      }
    }
  }
  Logfout.close();
  return false;
}

void // 求出tempNode内node的上行或者下行链路的另一端结点的ident，存放于tempEffectNode中
Ipv4GlobalRouting::GetEffectNode(vector<ident> effectInDirNode,vector<ident> tempNode,ident noNeedToNotice,string type,vector<ident> *tempEffectNode,struct MNinfo *tempMNInfo)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 求tempNode中Node的下一跳
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
  // 
  for (int i=0;i<tempNode.size();i++)// 先给一部分Node下发
  {
    if (type=="DOWN" || tempEffectNode->size()==0)//
    {
      tempMNInfo->destIdent=tempNode[i];
      if (SameNode(tempMNInfo->destIdent,noNeedToNotice));// 此Node不需要通知
      else if (!IsUnreachableNode(tempMNInfo->destIdent,effectInDirNode,*tempMNInfo))// 不是受影响的node，直接发送
      {
        m_tcpRoute->SendMessageTo(GetSockByIdent(tempNode[i]),*tempMNInfo);
        Logfout << GetNow() << "Send linkInfo to node " << tempNode[i].level << "." << tempNode[i].position << "." << endl;
      }
      else Logfout << GetNow() << "Send linkInfo to node " << tempNode[i].level << "." << tempNode[i].position << " later." << endl;
    }
  }
  if (tempEffectNode->size()==0) return;// 不存在相关的下一跳节点了，直接退出
  tempNode.clear();// 清空
  // 继续求下一跳
  for (int i=0;i<tempEffectNode->size();i++) tempNode.push_back((*tempEffectNode)[i]);
  
  if (type=="UP" && tempNode[0].level!=3)// 还未到Spine
  {
    tempEffectNode->clear();
    GetEffectNode(effectInDirNode,tempNode,noNeedToNotice,"UP",tempEffectNode,tempMNInfo);
  }
  else if (type=="UP" && tempNode[0].level==3)// 到SpineNode后可以开始求下行
  {
    tempEffectNode->clear();
    GetEffectNode(effectInDirNode,tempNode,noNeedToNotice,"DOWN",tempEffectNode,tempMNInfo);
  }
  else if (type=="DOWN" && tempNode[0].level!=1)// 还未到最底层，继续求下行
  {
    tempEffectNode->clear();
    GetEffectNode(effectInDirNode,tempNode,noNeedToNotice,"DOWN",tempEffectNode,tempMNInfo);
  }
  else if (type=="DOWN" && tempNode[0].level==1)// 此处需要考虑，如果是100个leaf，求10000个tor，
  {
    for (int i=0;i<tempEffectNode->size();i++)
    {
      tempMNInfo->destIdent=(*tempEffectNode)[i];
      if (SameNode(tempMNInfo->destIdent,noNeedToNotice));// 此Node不需要通知
      else if (!IsUnreachableNode(tempMNInfo->destIdent,effectInDirNode,*tempMNInfo))// 不是受影响的node，直接发送
      {
        m_tcpRoute->SendMessageTo(GetSockByIdent((*tempEffectNode)[i]),*tempMNInfo);
        Logfout << GetNow() << "Send linkInfo to node " << (*tempEffectNode)[i].level << "." << (*tempEffectNode)[i].position << "." << endl;
      }
      else Logfout << GetNow() << "Send linkInfo to node " << (*tempEffectNode)[i].level << "." << (*tempEffectNode)[i].position << " later." << endl;
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

  // 确定影响范围，这代码写得太垃圾了
  struct MNinfo *tempMNInfo=(struct MNinfo *)malloc(sizeof(struct MNinfo));
  tempMNInfo->addr.sin_family=AF_INET;// addr此时无实际意义
  inet_aton("255.255.255.255",&(tempMNInfo->addr.sin_addr));
  tempMNInfo->addr.sin_port=htons(0);
  tempMNInfo->destIdent=high;// 不可省略，未初始化不能使用
  tempMNInfo->srcIdent=myIdent;
  tempMNInfo->pathNodeIdentA=high;
  tempMNInfo->pathNodeIdentB=low;
  tempMNInfo->clusterMaster=false;
  tempMNInfo->chiefMaster=false;
  tempMNInfo->keepAlive=false;
  tempMNInfo->linkFlag=linkFlag;
  tempMNInfo->hello=false;
  tempMNInfo->ACK=false;

  // 某些间接连接的Node可能会因为这次链路变化而导致master的信息无法传达，所以需要记录
  vector<ident> effectInDirNode;
  ident noNeedToNotice;
  noNeedToNotice.level=-1;
  noNeedToNotice.position=-1;
  if (low.level==0)// 直连链路故障
  {
    noNeedToNotice=high;// 避免将信息发给直连失效的那个结点，因为这会导致它再次尝试重连
    for (int i=0;i<masterInDirPathTable.size();i++)
    {
      if (SameNode(high,masterInDirPathTable[i].pathNodeIdent[0])) continue;// 不需要通知，直连失联的Node会自动重连
      for (int j=MAX_PATH_LEN-1;j>=0;j--)
      {
        if (masterInDirPathTable[i].pathNodeIdent[j].level!=-1) // 间接路径是通过该masterInDirPathTable[i].pathNodeIdent[j]的直接连接与Master通信
        {
          if (SameNode(high,masterInDirPathTable[i].pathNodeIdent[j]))
          {
            // 随机选几个直连的Node尝试通知该间接Node重新和master建立连接
            srand((int)time(0));
            int randIndex=0;
            while (1)
            {
              randIndex=rand()%nodeMapToSock.size();
              if (nodeMapToSock[randIndex].nodeIdent.level!=0 && nodeMapToSock[randIndex].direct==true)
              {
                tempMNInfo->destIdent=masterInDirPathTable[i].pathNodeIdent[0];
                m_tcpRoute->SendMessageTo(nodeMapToSock[randIndex].nodeSock,*tempMNInfo);
                Logfout << GetNow() << "Choose node " << nodeMapToSock[randIndex].nodeIdent.level << "." << nodeMapToSock[randIndex].nodeIdent.position << " to inform node " << tempMNInfo->destIdent.level << "." << tempMNInfo->destIdent.position << "." << endl;
                break;
              }
            }
            effectInDirNode.push_back(masterInDirPathTable[i].pathNodeIdent[0]);
            break;
          }
        }
      }
    }
  }
  else
  {
    for (int i=0;i<masterInDirPathTable.size();i++)
    {
      for (int j=0;j<MAX_PATH_LEN-1;j++)
      {
        if ((SameNode(high,masterInDirPathTable[i].pathNodeIdent[j]) && SameNode(low,masterInDirPathTable[i].pathNodeIdent[j+1])) || (SameNode(low,masterInDirPathTable[i].pathNodeIdent[j]) && SameNode(high,masterInDirPathTable[i].pathNodeIdent[j+1])))
        {
          // 随机选几个直连的Node尝试通知该间接Node重新和master建立连接
          srand((int)time(0));
          int randIndex=0;
          while (1)
          {
            randIndex=rand()%nodeMapToSock.size();
            if (nodeMapToSock[randIndex].nodeIdent.level!=0 && nodeMapToSock[randIndex].direct==true)
            {
              tempMNInfo->destIdent=masterInDirPathTable[i].pathNodeIdent[0];
              m_tcpRoute->SendMessageTo(nodeMapToSock[randIndex].nodeSock,*tempMNInfo);
              Logfout << GetNow() << "Choose node " << nodeMapToSock[randIndex].nodeIdent.level << "." << nodeMapToSock[randIndex].nodeIdent.position << " to inform node " << tempMNInfo->destIdent.level << "." << tempMNInfo->destIdent.position << "." << endl;
              break;
            }
          }
          effectInDirNode.push_back(masterInDirPathTable[i].pathNodeIdent[0]);
          break;
        }
      }
    }
  }

  Logfout << GetNow() << "Master start to send message:" << endl;

  vector<ident> tempNode,tempEffectNode;
  tempNode.push_back(high);

  if (high.level==1) GetEffectNode(effectInDirNode,tempNode,noNeedToNotice,"UP",&tempEffectNode,tempMNInfo);
  else if (high.level==2) GetEffectNode(effectInDirNode,tempNode,noNeedToNotice,"UP",&tempEffectNode,tempMNInfo);
  else if (high.level==3) GetEffectNode(effectInDirNode,tempNode,noNeedToNotice,"DOWN",&tempEffectNode,tempMNInfo);
}

/**************************Master**************************/