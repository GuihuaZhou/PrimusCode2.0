#include "ipv4-global-routing.h"

// using namespace std;
pthread_mutex_t mutexA;

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

  system("/home/guolab/script/stop.sh");
  
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
  m_defaultLinkTimer=defaultLinkTimer*1000;// master链路定时器，配置文件写成了ms，所以要乘以1000，换算成us
  m_defaultKeepaliveTimer=defaultKeepaliveTimer;
  m_IsCenterRouting=IsCenterRouting;
  m_randomEcmpRouting=randomEcmpRouting;
 
  ident tempNode;
  tempNode.level=-1;
  tempNode.position=-1;

  if (myIdent.level==0 && myIdent.position==0) chiefMaster=true;
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

    headNodeLinkTableEntry->high=tempNode;
    headNodeLinkTableEntry->low=tempNode;
    headNodeLinkTableEntry->linkFlag=false;
    headNodeLinkTableEntry->lastLinkFlag=false;
    headNodeLinkTableEntry->lastUpdateTime=0;//用来统计链路数量
    headNodeLinkTableEntry->linkUpdateTimer=m_defaultLinkTimer;
    headNodeLinkTableEntry->isStartTimer=false;
    headNodeLinkTableEntry->next=NULL;
  }
  else if (myIdent.level==0)// master
  {
    headMasterLinkTableEntry->high=tempNode;
    headMasterLinkTableEntry->low=tempNode;
    headMasterLinkTableEntry->linkFlag=false;
    headMasterLinkTableEntry->lastLinkFlag=false;
    headMasterLinkTableEntry->lastUpdateTime=0;//用来统计链路数量
    headMasterLinkTableEntry->linkUpdateTimer=m_defaultLinkTimer;
    headMasterLinkTableEntry->isStartTimer=false;
    headMasterLinkTableEntry->next=NULL;
  }

  m_tcpRoute=new TCPRoute(this,m_defaultKeepaliveTimer,chiefMaster);

  m_udpServer.SetGlobalRouting(this);
  m_udpClient.SetGlobalRouting(this);

  if(myIdent.level!=0)
  {
    // m_udpServer.SetGlobalRouting(this);
    // m_udpClient.SetGlobalRouting(this);
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

void* 
Ipv4GlobalRouting::PrintStampThread(void* tempThreadParam)
{
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamA *)tempThreadParam)->tempGlobalRouting;

  while (1)
  {
    sleep(30);
    if (tempGlobalRouting->stampInfo.size()>0)
    {
      pthread_mutex_lock(&mutexA);
      ofstream Logfout("/home/guolab/output/primusStamp.txt",ios::app);
      Logfout.setf(ios::fixed,ios::floatfield);  // 设定为 fixed 模式，以小数点表示浮点数
      Logfout.precision(6);

      for (int i=0;i<tempGlobalRouting->stampInfo.size();i++)
      {
        Logfout << tempGlobalRouting->stampInfo[i].stamp << "\t" << tempGlobalRouting->stampInfo[i].identA.level << "." << tempGlobalRouting->stampInfo[i].identA.position << "--" << tempGlobalRouting->stampInfo[i].identB.level << "." << tempGlobalRouting->stampInfo[i].identB.position << "\t";
        if (tempGlobalRouting->stampInfo[i].linkFlag==true) Logfout << "\tup";
        else Logfout << "\tdown";
        Logfout << "\t" << tempGlobalRouting->stampInfo[i].note << endl;
      }
      tempGlobalRouting->stampInfo.clear();
      pthread_mutex_unlock(&mutexA);
    }
  }
}

void
Ipv4GlobalRouting::Start(vector<struct masteraddressset> tempMasterAddressSet)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  masterAddressSet=tempMasterAddressSet;

  ident tempIdent;
  tempIdent.level=-1;
  tempIdent.position=-1;

  threadparamA *threadParam=new threadparamA();
  threadParam->tempGlobalRouting=this;
  threadParam->tempLinkTableEntry=NULL;

  if (pthread_create(&printStamp_thread,NULL,PrintStampThread,(void *)threadParam)<0)
  {
    Logfout << GetNow() << "Create PrintStampThread failed." << endl;
    exit(0);
  }

  m_udpServer.StartApplication();

  if(myIdent.level==0)
  {
    // cout << "I am master!" << endl;
    if (!chiefMaster) m_tcpRoute->SendHelloTo(tempIdent,masterAddressSet[0].masterAddress[0]);

    if (pthread_create(&listenKeepAlive_thread,NULL,ListenKeepAliveThread,(void *)threadParam)<0)
    {
      Logfout << GetNow() << "Master could not check keepAlive." << endl;
      exit(0);
    }
    m_tcpRoute->StartListen();
  }
  else
  {
    // cout << "I am not master!" << endl;
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
Ipv4GlobalRouting::GetSockByAddress(string tempAddress)
{
  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (masterMapToSock[i].masterAddr==tempAddress || masterMapToSock[i].middleAddr==tempAddress) return masterMapToSock[i].masterSock;
  }
  return -1;
}

int 
Ipv4GlobalRouting::GetSockByMasterIdent(ident masterIdent)
{
  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (SameNode(masterMapToSock[i].masterIdent,masterIdent)) return masterMapToSock[i].masterSock;
  }
  return -1;
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

void 
Ipv4GlobalRouting::CloseSock(int sock)
{
  bool first=true;
  for (int i=0;i<masterMapToSock.size();i++) 
  {
    if (masterMapToSock[i].masterSock==sock) 
    {
      masterMapToSock[i].masterSock=-1;
      if (first==true) 
      {
        shutdown(sock,SHUT_RDWR);
        first=false;
      }
    }
  }
  for (int i=0;i<nodeMapToSock.size();i++) 
  {
    if (nodeMapToSock[i].nodeSock==sock) 
    {
      nodeMapToSock[i].nodeSock=-1;
      if (first==true) 
      {
        shutdown(sock,SHUT_RDWR);
        first=false;
      }
    }
  }
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
            // Logfout << GetNow() << "KeepAlive timeout,connect with master " << masterIdent.level << "." << masterIdent.position << " failed." << endl;
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
  int masterSock=0;

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
  tempMNInfo.forwardIdent=masterIdent;
  tempMNInfo.srcIdent=tempGlobalRouting->myIdent;
  tempMNInfo.pathNodeIdent[0]=tempGlobalRouting->myIdent;
  tempMNInfo.pathNodeIdent[1]=tempGlobalRouting->myIdent;
  for (int i=2;i<MAX_PATH_LEN;i++)
  {
    tempMNInfo.pathNodeIdent[i].level=-1;
    tempMNInfo.pathNodeIdent[i].position=-1;
  }
  tempMNInfo.eventId=-1;
  tempMNInfo.clusterMaster=false;
  tempMNInfo.chiefMaster=false;
  tempMNInfo.reachable=true;
  tempMNInfo.keepAlive=true;
  tempMNInfo.linkFlag=false;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;
  tempMNInfo.bye=false;

  int tempDefaultKeepaliveTimer=tempGlobalRouting->m_defaultKeepaliveTimer;
  int temp=0;
  while (1)
  {
    sleep(tempDefaultKeepaliveTimer);
    masterSock=tempGlobalRouting->GetSockByMasterIdent(masterIdent);
    if (masterSock==-1) break;// 更换了新的套接字，默默退出
    tempTCPRoute->SendMessageTo(masterSock,tempMNInfo);
    temp=tempGlobalRouting->UpdateKeepAlive(masterSock,masterIdent,false);
    if (temp==2) // keepalive timeout
    {
      if (tempGlobalRouting->myIdent.level==0)// common Master处理chief Master宕机
      {
        if (tempGlobalRouting->SameNode(tempGlobalRouting->GetChiefMasterIdent(),masterIdent))// chiefmaster挂了
        {
          Logfout << GetNow() << "ChiefMaster may be DOWN!" << endl;
          tempGlobalRouting->NewChiefMasterElection(masterIdent);// 选举新的chief master
          tempGlobalRouting->CloseSock(masterSock);
        }
      }
      else if (tempGlobalRouting->myIdent.level!=0)// node处理chiefmaster宕机
      {
        tempGlobalRouting->CloseSock(masterSock);
      }
      break;
    }
    else if (temp==3) // 套接字已经不存在了，可能是换了新的连接
    {
      tempGlobalRouting->CloseSock(masterSock);
      break;
    }
  }

  for (int i=0;i<tempGlobalRouting->masterMapToSock.size();i++)
  {
    if (tempGlobalRouting->SameNode(tempGlobalRouting->masterMapToSock[i].masterIdent,masterIdent))
    {
      tempGlobalRouting->masterMapToSock[i].isStartKeepAlive=false;
      break;
    }
  }
  tempTCPRoute->SendHelloTo(masterIdent,tempGlobalRouting->GetMasterAddrByIdent(masterIdent));
  Logfout << GetNow() << "Node stop to send keepAlive to master " << masterIdent.level << "." << masterIdent.position << "[sock:" << masterSock << "]." << endl;
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
  // 此处有问题
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
          masterMapToSock[i].inDirPath=tempMasterMapToSock.inDirPath;
          PrintMasterMapToSock();
          isFind=true;
        }
        else if (tempMasterMapToSock.masterIdent.level==0)// 收到ack后来修改ident
        {
          int value=0;

          masterMapToSock[i].keepAliveFaildNum=tempMasterMapToSock.keepAliveFaildNum;
          masterMapToSock[i].recvKeepAlive=tempMasterMapToSock.recvKeepAlive;

          if (masterMapToSock[i].isStartKeepAlive==false)
          {
            for (auto iter=delayMNInfo.begin();iter!=delayMNInfo.end();)
            {
              value=m_tcpRoute->SendMessageTo(tempMasterMapToSock.masterSock,*iter);
              // 收到ACK后清理滞留的信息，但这个时候依然有可能发不出去，但我也懒得管了
              Logfout << GetNow() << "Clear delay MNinfo " << iter->pathNodeIdent[0].level << "." << iter->pathNodeIdent[0].position << "--" <<
              iter->pathNodeIdent[1].level << "." << iter->pathNodeIdent[1].position;
              if (iter->linkFlag==true) Logfout << " up";
              else Logfout << " down";
              Logfout << " to Master " << tempMasterMapToSock.masterIdent.level << "." << tempMasterMapToSock.masterIdent.position << "[value:" << value << "][sock:" << tempMasterMapToSock.masterSock << "]." << endl;
              
              if (value>0)
              {
                iter=delayMNInfo.erase(iter);
                if (iter==delayMNInfo.end()) break;
              }
              iter++;
            }
            // 收到ACK时调用该函数是不知道网口名称的
            masterMapToSock[i].masterIdent=tempMasterMapToSock.masterIdent;// 收到ack后改ident
            masterMapToSock[i].masterSock=tempMasterMapToSock.masterSock;
            masterMapToSock[i].direct=tempMasterMapToSock.direct;
            masterMapToSock[i].chiefMaster=tempMasterMapToSock.chiefMaster;
            masterMapToSock[i].isStartKeepAlive=true;

            struct threadparamC *threadParam=(struct threadparamC *)malloc(sizeof(struct threadparamC));
            threadParam->tempGlobalRouting=this;
            threadParam->tempTCPRoute=m_tcpRoute;
            threadParam->masterIdent=tempMasterMapToSock.masterIdent;
            if (pthread_create(&keepalive_thread,NULL,KeepAliveThread,(void *)threadParam)<0)
            {
              Logfout << GetNow() << "Create KeepAliveThread for sock[" << tempMasterMapToSock.masterSock << "] failed." << endl;
            }
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
  // string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    if (NICInfo[i].neighborAddr.sin_addr.s_addr==addr.sin_addr.s_addr)
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
      string remoteAddr;
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
  // string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    // string NICRemoteAddr=inet_ntoa(NICInfo[i].neighborAddr.sin_addr);
    if (NICInfo[i].neighborAddr.sin_addr.s_addr==addr.sin_addr.s_addr) return NICInfo[i].neighborIdent;
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

struct sockaddr_in *
Ipv4GlobalRouting::GetLocalAddrByNeighborIdent(ident neighborIdent)
{
  struct sockaddr_in *tempAddr=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  for (int i=0;i<NICInfo.size();i++)
  {
    if (SameNode(NICInfo[i].neighborIdent,neighborIdent)) 
    {
      *tempAddr=NICInfo[i].localAddr;
      return tempAddr;
    }
  } 
  return NULL;
}

void 
Ipv4GlobalRouting::GetLocalAddrByRemoteAddr(struct sockaddr_in *localAddr,struct sockaddr_in addr)
{
  // string remoteAddr=inet_ntoa(addr.sin_addr);
  for (int i=0;i<NICInfo.size();i++)
  {
    // string NICRemoteAddr=inet_ntoa(NICInfo[i].neighborAddr.sin_addr);
    if (NICInfo[i].neighborAddr.sin_addr.s_addr==addr.sin_addr.s_addr) 
    {
      *localAddr=NICInfo[i].localAddr;
      break;
    }
  } 
}

struct sockaddr_in *
Ipv4GlobalRouting::GetAddrByNICName(string NICName)// 通过rtnetlink获取网口的地址
{
  struct ifaddrs *ifa;
  if (0!=getifaddrs(&ifa))
  {
    printf("getifaddrs error\n");
    return NULL;
  }
  for (;ifa!=NULL;)
  {
    if (ifa->ifa_name==NICName && ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL)
    {
      return (struct sockaddr_in *)(ifa->ifa_addr);
    }
    ifa=ifa->ifa_next;
  }
  freeifaddrs(ifa);
  return NULL;
}

struct pathtableentry *
Ipv4GlobalRouting::GetPathToMaster(struct pathtableentry *lastPath)// 获得一条通往master的路径，可能是直连，可能是间接连接
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 此处有个错误，如果有A，B两条路不通但linkCounter和dirConFlag正常
  // 假设第一次选择A，再次调用时lastPath为A，然后再选择B，再次调用时lastPath为B，这时就会再次选择A，循环往复
  // for (int i=0;i<nodeInDirPathTable.size();i++)
  // {
  //   if (lastPath!=nodeInDirPathTable[i])// 不是上次选的路径
  //   {
  //     if (nodeInDirPathTable[i]->linkCounter==0 && nodeInDirPathTable[i]->dirConFlag==true)
  //     {
  //       // Logfout.close();
  //       return nodeInDirPathTable[i];
  //     }
  //   }
  // }
  struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
  if (lastPath==NULL)// 表示第一次来去取，则返回第一条符合要要的路径
  {
    tempPathTableEntry=headPathTableEntry;
  }
  else 
  {
    tempPathTableEntry=lastPath;
  }

  while (tempPathTableEntry->next!=NULL)
  {
    tempPathTableEntry=tempPathTableEntry->next;
    if (tempPathTableEntry->linkCounter==0 && tempPathTableEntry->dirConFlag==true)
    {
      return tempPathTableEntry;
    }
  }
  // Logfout.close();
  return NULL;
}

struct pathtableentry *
Ipv4GlobalRouting::GetPathToNode(ident destIdent)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (headPathTableEntry->next!=NULL)
  {
    struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
    tempPathTableEntry=headPathTableEntry->next;
    while (tempPathTableEntry!=NULL)
    {
      if (SameNode(destIdent,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]))// 目的node相同
      {
        if (tempPathTableEntry->linkCounter==0)// 可达
        {
          // Logfout.close();
          return tempPathTableEntry;
        }
      }
      tempPathTableEntry=tempPathTableEntry->next;
    }
  }

  // Logfout.close();
  return NULL;
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
Ipv4GlobalRouting::UpdateResponseRecord(int eventId,ident identA,ident identB,int cmd)// cmd==-1，收到了response；cmd==1，发出了mninfo
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  int value=0;
  
  for (auto iter=linkInfoResponse.begin();iter!=linkInfoResponse.end();)
  {
    if (eventId==iter->tempMNInfo.eventId && ((SameNode(iter->tempMNInfo.pathNodeIdent[0],identA) && SameNode(iter->tempMNInfo.pathNodeIdent[1],identB)) || (SameNode(iter->tempMNInfo.pathNodeIdent[0],identB) && SameNode(iter->tempMNInfo.pathNodeIdent[1],identA)))) 
    {
      iter->unRecvNum+=cmd;
      if (iter->unRecvNum==0)// 收到了全部的response
      {
        if (SameNode(iter->tempMNInfo.srcIdent,myIdent))// 整个任务都已经完成
        {
          struct timespec tv;
          clock_gettime(CLOCK_MONOTONIC,&tv);
          tempStampInfo.identA=iter->tempMNInfo.pathNodeIdent[0];
          tempStampInfo.identB=iter->tempMNInfo.pathNodeIdent[1];
          tempStampInfo.linkFlag=iter->tempMNInfo.linkFlag;
          tempStampInfo.note="Recv all response";
          tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          stampInfo.push_back(tempStampInfo);

          Logfout << GetNow() << "Recv all response for event [eventId:" << eventId << "]." << endl;
        }
        else // 发起整个事件的源不是自己
        {
          // 
          ident tempIdent;
          tempIdent=iter->tempMNInfo.destIdent;
          iter->tempMNInfo.destIdent=iter->tempMNInfo.srcIdent;
          iter->tempMNInfo.srcIdent=tempIdent;
          iter->tempMNInfo.ACK=true;

          value=m_tcpRoute->SendMessageTo(GetSockByIdent(iter->tempMNInfo.destIdent),iter->tempMNInfo);

          struct timespec tv;
          clock_gettime(CLOCK_MONOTONIC,&tv);
          tempStampInfo.identA=iter->tempMNInfo.pathNodeIdent[0];
          tempStampInfo.identB=iter->tempMNInfo.pathNodeIdent[1];
          tempStampInfo.linkFlag=iter->tempMNInfo.linkFlag;
          tempStampInfo.note="Send response to srcIdent";
          tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          stampInfo.push_back(tempStampInfo);

          Logfout << GetNow() << "Inform Node " << iter->tempMNInfo.destIdent.level << "." << iter->tempMNInfo.destIdent.position << " that I have received all response for event [eventId:" << eventId << "]." << endl;
        }
        iter=linkInfoResponse.erase(iter);
        break;
      }
    }
    if(iter!=linkInfoResponse.end())
      iter++;
  }
  Logfout.close();
}

void* 
Ipv4GlobalRouting::CheckResponseThread(void* tempThreadParam)
{
  pthread_detach(pthread_self());
  Ipv4GlobalRouting *tempGlobalRouting=((struct threadparamE *)tempThreadParam)->tempGlobalRouting;
  struct MNinfo tempMNInfo=((struct threadparamE *)tempThreadParam)->tempMNInfo;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempGlobalRouting->myIdent.level << "." << tempGlobalRouting->myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  int timeout=0;
  for (int i=0;i<tempGlobalRouting->linkInfoResponse.size();i++)
  {
    if (tempGlobalRouting->linkInfoResponse[i].tempMNInfo.eventId==tempMNInfo.eventId && tempGlobalRouting->SameNode(tempGlobalRouting->linkInfoResponse[i].tempMNInfo.pathNodeIdent[0],tempMNInfo.pathNodeIdent[0]) && tempGlobalRouting->SameNode(tempGlobalRouting->linkInfoResponse[i].tempMNInfo.pathNodeIdent[1],tempMNInfo.pathNodeIdent[1]))
    {
      while (timeout<10)
      {
        usleep(RESP_CHECK_INTERVAL);
        if (tempGlobalRouting->linkInfoResponse[i].unRecvNum<=0)
        {
          break;
        }
        else timeout++;
      }
      if (timeout<10)// 超时前收到全部的response
      {
        // 
      }
      else
      {
        Logfout << GetNow() << "Didn't recv all response[eventId:" << tempMNInfo.eventId << "]." << endl;
      }
      break;
    }
  }
  // Logfout << GetNow() << "CheckResponseThread down[eventId:" << tempMNInfo.eventId << "]." << endl;
  Logfout.close();
  pthread_exit(0);
}

void
Ipv4GlobalRouting::HandleMessage(struct MNinfo tempMNInfo,string type)
{
  pthread_mutex_lock(&mutexA);

  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  int value=0;
  ident high,low;
  if (tempMNInfo.pathNodeIdent[0].level>tempMNInfo.pathNodeIdent[1].level)
  {
    high=tempMNInfo.pathNodeIdent[0];
    low=tempMNInfo.pathNodeIdent[1];
  }
  else if (tempMNInfo.pathNodeIdent[0].level<=tempMNInfo.pathNodeIdent[1].level)
  {
    high=tempMNInfo.pathNodeIdent[1];
    low=tempMNInfo.pathNodeIdent[0];
  }
  // 先处理完，再回复ACK
  
  if (myIdent.level==0)
  {
    tempStampInfo.identA=high;
    tempStampInfo.identB=low;
    tempStampInfo.linkFlag=tempMNInfo.linkFlag;
    tempStampInfo.note="("+type+")HandleMessage start";
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC,&tv);
    tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;

    // 同时判断链路信息是否需要下发Node，因为链路信息可能已经处理了
    bool isNeedToNotice=UpdateMasterLinkTable(high,low,tempMNInfo.srcIdent,tempMNInfo.eventId,tempMNInfo.linkFlag);

    // 如果是选举出来的处理信息的master
    if (chiefMaster && isNeedToNotice)
    {
      stampInfo.push_back(tempStampInfo);

      struct linkinforesponse tempLinkInfoResponse;
      tempLinkInfoResponse.unRecvNum=0;
      tempLinkInfoResponse.tempMNInfo=tempMNInfo;
      linkInfoResponse.push_back(tempLinkInfoResponse);

      // tempStampInfo.note="("+type+")SendMessageToNode start";
      // clock_gettime(CLOCK_MONOTONIC,&tv);
      // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
      // stampInfo.push_back(tempStampInfo);

      clock_gettime(CLOCK_MONOTONIC,&tv);
      tempStampInfo.note="("+type+")HandleMessage over";
      tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
      stampInfo.push_back(tempStampInfo);

      SendMessageToNode(high,low,tempMNInfo.srcIdent,tempMNInfo.eventId,tempMNInfo.linkFlag);

      // clock_gettime(CLOCK_MONOTONIC,&tv);
      // tempStampInfo.note="("+type+")SendMessageToNode over";
      // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
      // stampInfo.push_back(tempStampInfo);

      threadparamE *tempThreadParam=new threadparamE();
      tempThreadParam->tempGlobalRouting=this;
      tempThreadParam->tempMNInfo=tempMNInfo;

      if (pthread_create(&checkResponse_thread,0,CheckResponseThread,(void*)tempThreadParam)!=0)
      {
        // printf("Create checkResponse_thread failed.\n");
        // Logfout << GetNow() << "Create CheckResponseThread Failed[eventId:" << tempMNInfo.eventId << "]." << endl;
        exit(1);
      }

      // clock_gettime(CLOCK_MONOTONIC,&tv);
      // tempStampInfo.note="("+type+")Create CheckResponseThread over";
      // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
      // stampInfo.push_back(tempStampInfo);

      // chief需要等到所有受影响的交换机都回复了后才回复
      // 转发至其他slave master同步
      // tempMNInfo.srcIdent=myIdent;
      // for (int i=0;i<nodeMapToSock.size();i++)
      // {
      //   if (nodeMapToSock[i].nodeIdent.level==0)
      //   {
      //     tempMNInfo.destIdent=nodeMapToSock[i].nodeIdent;
      //     value=m_tcpRoute->SendMessageTo(nodeMapToSock[i].nodeSock,tempMNInfo);
      //     // Logfout << GetNow() << "Forward linkInfo to common master " << tempNodeMapToSock[i].nodeIdent.level << "." << tempNodeMapToSock[i].nodeIdent.position << "[value:" << value << "]." << endl;
      //   }
      //   else break;
      // }
      // clock_gettime(CLOCK_MONOTONIC,&tv);
      // tempStampInfo.note="("+type+")HandleMessage over";
      // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
      // stampInfo.push_back(tempStampInfo);
    }
  }
  else // node处理从master收到的链路变化信息
  {
    if (low.level==0)// 这是和master的直连信息
    {
      ModifyNodeDirConFlag(high,low,tempMNInfo.linkFlag);
    }
    else
    {
      tempStampInfo.identA=high;
      tempStampInfo.identB=low;
      tempStampInfo.linkFlag=tempMNInfo.linkFlag;
      tempStampInfo.note="("+type+")HandleMessage start";
      struct timespec tv;
      clock_gettime(CLOCK_MONOTONIC,&tv);
      tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;

      if (SameNode(myIdent,high) || SameNode(myIdent,low))// 直连链路信息
      {
        for (int i=0;i<NICInfo.size();i++)
        {
          if (SameNode(NICInfo[i].neighborIdent,high) || SameNode(NICInfo[i].neighborIdent,low))// 
          {
            if (NICInfo[i].eventId<=tempMNInfo.eventId)
            {
              stampInfo.push_back(tempStampInfo);
              NICInfo[i].eventId=tempMNInfo.eventId+1;
              // 直连链路恢复
              if (tempMNInfo.linkFlag==true) NICInfo[i].sleep=true;
              else NICInfo[i].sleep=false;

              // tempStampInfo.note="("+type+")ModifyPathEntryTable start";
              // clock_gettime(CLOCK_MONOTONIC,&tv);
              // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
              // stampInfo.push_back(tempStampInfo);

              ModifyPathEntryTable(high,low,tempMNInfo.linkFlag);

              clock_gettime(CLOCK_MONOTONIC,&tv);
              tempStampInfo.note="("+type+")HandleMessage over";
              tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
              stampInfo.push_back(tempStampInfo);

              // clock_gettime(CLOCK_MONOTONIC,&tv);
              // tempStampInfo.note="("+type+")ModifyPathEntryTable over";
              // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
              // stampInfo.push_back(tempStampInfo);
              
              // 普通Node处理链路信息后立即回复
              ident tempIdent;
              tempIdent=tempMNInfo.srcIdent;
              tempMNInfo.srcIdent=tempMNInfo.destIdent;
              tempMNInfo.destIdent=tempIdent;
              tempMNInfo.ACK=true;
              value=m_tcpRoute->SendMessageTo(GetSockByMasterIdent(tempMNInfo.destIdent),tempMNInfo);
              
              // clock_gettime(CLOCK_MONOTONIC,&tv);
              // tempStampInfo.note="("+type+")Send response over";
              // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
              // stampInfo.push_back(tempStampInfo);

              // clock_gettime(CLOCK_MONOTONIC,&tv);
              // tempStampInfo.note="("+type+")HandleMessage over";
              // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
              // stampInfo.push_back(tempStampInfo);
              
              // Logfout << GetNow() << "Response linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--"
              // << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
              // if (tempMNInfo.linkFlag==true) Logfout << " up";
              // else Logfout << " down";
              // Logfout << " to Master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[eventId:" << tempMNInfo.eventId << "][value:" << value << "]." << endl;
            }
            else
            {
              // 
            }
            break;
          }
        }
      }
      else // 非直连链路则查看node维护的链路表
      {
        if (UpdateNodeLinkTable(high,low,tempMNInfo.eventId,tempMNInfo.linkFlag,2)==true)//
        {
          stampInfo.push_back(tempStampInfo);

          // tempStampInfo.note="("+type+")ModifyPathEntryTable start";
          // clock_gettime(CLOCK_MONOTONIC,&tv);
          // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          // stampInfo.push_back(tempStampInfo);

          ModifyPathEntryTable(high,low,tempMNInfo.linkFlag);

          clock_gettime(CLOCK_MONOTONIC,&tv);
          tempStampInfo.note="("+type+")HandleMessage over";
          tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          stampInfo.push_back(tempStampInfo);

          // clock_gettime(CLOCK_MONOTONIC,&tv);
          // tempStampInfo.note="("+type+")ModifyPathEntryTable over";
          // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          // stampInfo.push_back(tempStampInfo);
           
          // 普通Node处理链路信息后立即回复
          ident tempIdent;
          tempIdent=tempMNInfo.srcIdent;
          tempMNInfo.srcIdent=tempMNInfo.destIdent;
          tempMNInfo.destIdent=tempIdent;
          tempMNInfo.ACK=true;
          value=m_tcpRoute->SendMessageTo(GetSockByMasterIdent(tempMNInfo.destIdent),tempMNInfo);

          // clock_gettime(CLOCK_MONOTONIC,&tv);
          // tempStampInfo.note="("+type+")Send response over";
          // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          // stampInfo.push_back(tempStampInfo);

          // clock_gettime(CLOCK_MONOTONIC,&tv);
          // tempStampInfo.note="("+type+")HandleMessage over";
          // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          // stampInfo.push_back(tempStampInfo);
          
          // Logfout << GetNow() << "Response linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--"
          // << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
          // if (tempMNInfo.linkFlag==true) Logfout << " up";
          // else Logfout << " down";
          // Logfout << " to Master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[eventId:" << tempMNInfo.eventId << "][value:" << value << "]." << endl;
        }
      } 
    }
  }
  // Logfout.close();
  pthread_mutex_unlock(&mutexA);
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

string 
Ipv4GlobalRouting::GetMasterAddrByIdent(ident masterIdent)
{
  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (SameNode(masterIdent,masterMapToSock[i].masterIdent)) return masterMapToSock[i].masterAddr;
  }
  return NULL;
}

bool 
Ipv4GlobalRouting::IsLegalMaster(string masterAddress)// 检查和master的连接是否合法
{
  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (!strcmp(masterAddress.c_str(),masterMapToSock[i].masterAddr.c_str()))
    {
      if (masterMapToSock[i].masterIdent.level!=-1 && masterMapToSock[i].masterIdent.position!=-1 && masterMapToSock[i].masterSock>0) return true;
      else return false;
    }
  }
  return false;
}

bool 
Ipv4GlobalRouting::IsLegalNeighbor(struct NICinfo tempNICInfo)//判断邻居是否合法，如果邻居为无效连接或者服务器，返回false
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 判断neighborIdent、isSwitch和isServer
  if (tempNICInfo.neighborIdent.level!=-1 && tempNICInfo.neighborIdent.position!=-1)
  {
    if (tempNICInfo.isSwitch && !tempNICInfo.isServer) return true;
    else return false;
  }
  else return false;
  // Logfout.close();
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
        NICInfo[i].recvND=true;
        NICInfo[i].isSwitch=true;
        NICInfo[i].neighborIdent=tempNDInfo.myIdent;// 收邻居发来的hello
        isFindNICInfo=true;
        // 判断向邻居发送hello后是否收到回复
        // if (NICInfo[i].isServer!=NICInfo[i].isSwitch)// 不是交换机就是服务器，说明已经收到hello的回复了
        if (NICInfo[i].sendND==true) FreshNeighboorList(NICInfo[i]);
      }
      break;
    }
  }
  if (isFindNICInfo==false)
  {
    struct NICinfo temp;
    temp.eventId=0;
    temp.NICName="";
    temp.isServer=false;
    temp.isSwitch=true;// 主动收到邻居的ND，肯定是switch
    temp.sendND=false;
    temp.recvND=true;// 收到了邻居发来的ND
    temp.judge=true;
    temp.sleep=false;
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
  PrintNeighborInfo();
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
      NICInfo[i].sendND=true;
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
  // pthread_mutex_lock(&mutexA);
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
    
    // tempStampInfo.identA=myIdent;
    // tempStampInfo.identB=tempNICInfo.neighborIdent;
    // tempStampInfo.linkFlag=true;
    // tempStampInfo.note="link up";
    // struct timespec tv;
    // clock_gettime(CLOCK_MONOTONIC,&tv);
    // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
    // stampInfo.push_back(tempStampInfo);

    SendLinkInfoToMaster(myIdent,tempNICInfo.neighborIdent,true);

    // tempStampInfo.note="Send link up over";
    // clock_gettime(CLOCK_MONOTONIC,&tv);
    // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
    // stampInfo.push_back(tempStampInfo);
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

      if (headPathTableEntry->next!=NULL)
      {
        struct pathtableentry *tempPathTableEntry=headPathTableEntry->next;
        while (tempPathTableEntry!=NULL)
        {
          if (IsLegalPathInfo(tempPathTableEntry->pathNodeIdent,tempNICInfo.neighborIdent))// 判断路径发送给该邻居是否合法
          {
            // 必须先初始化，否则会出现比如上一条路径有4个node，而这条只有2个node，却也会变成4个的错误
            for (int j=0;j<MAX_PATH_LEN;j++) tempPathInfo->pathNodeIdent[j]=tempNode;

            for (int j=0;j<MAX_ADDR_NUM;j++) 
            {
              tempPathInfo->addrSet[j].addr=tempAddr;
              tempPathInfo->addrSet[j].prefixLen=32;
            }

            for (int j=0,k=tempPathTableEntry->nodeCounter-1;k>=0;j++,k--)
            {
              tempPathInfo->pathNodeIdent[j]=tempPathTableEntry->pathNodeIdent[k];
            }

            tempPathInfo->nodeCounter=tempPathTableEntry->nodeCounter;
            tempPathInfo->nodeAddr.addr=tempPathTableEntry->nodeAddr.addr;
            tempPathInfo->nodeAddr.prefixLen=tempPathTableEntry->nodeAddr.prefixLen;

            tempPathInfo->nextHopAddr.addr=tempNICInfo.localAddr;
            tempPathInfo->nextHopAddr.prefixLen=NetmaskToPrefixlen(tempNICInfo.localMask);

            for (int j=0;j<MAX_ADDR_NUM;j++)
            {
              if (!strcmp(inet_ntoa(tempPathTableEntry->pathAddrSet->addrSet[j].addr.sin_addr),"255.255.255.255")) break;
              tempPathInfo->addrSet[j]=tempPathTableEntry->pathAddrSet->addrSet[j];
            }

            // Logfout << GetNow() << "The pathinfo size is " << sizeof(*tempPathInfo) << endl;
            usleep(SEND_PATH_INTERVAL);// 限速保护
            m_udpClient.SendPathInfoTo(tempNICInfo.localAddr,tempNICInfo.neighborAddr,tempPathInfo);
          }
          tempPathTableEntry=tempPathTableEntry->next;
        }
      } 
      else 
      {
        Logfout << GetNow() << "Path table is empty!" << endl;
      }
    }
  } 
  // Logfout << "FreshNeighboorList over" << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::FreshPathTable(struct pathinfo *tempPathInfo,struct sockaddr_in remote_addr)//收到了新的路径
{
  // pthread_mutex_lock(&mutexA);
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
  // Logfout << "-------FreshPathTable over" << endl;
  Logfout.close();
  // pthread_mutex_unlock(&mutexA);
}

bool 
Ipv4GlobalRouting::IsNewNeighbor(string NICName)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // Logfout << "IsNewNeighbor NICName:" << NICName << endl;

  for (int i=0;i<NICInfo.size();i++)
  {
    // Logfout << "NICName:" << NICInfo[i].NICName << endl;
    if (NICInfo[i].NICName==NICName) // 邻居信息已经存在
    {
      if (NICInfo[i].sendND==true && NICInfo[i].recvND==true) // 
      {
        Logfout.close();
        return false;
      }
      break;
    }
  }
  Logfout.close();
  return true;
}

// 上报链路信息时获取本地链路的eventid，不同于为路径表中的每一条链路维护的id，这个id是其他node来维护的
int 
Ipv4GlobalRouting::GetLocalLinkEventId(ident neighborIdent)
{
  // pthread_mutex_lock(&mutexA);
  for (int i=0;i<NICInfo.size();i++)
  {
    if (SameNode(NICInfo[i].neighborIdent,neighborIdent))
    {
      NICInfo[i].eventId++;
      if (NICInfo[i].eventId>10000) NICInfo[i].eventId=0;
      // pthread_mutex_unlock(&mutexA);
      return NICInfo[i].eventId;
    }
  }
  // pthread_mutex_unlock(&mutexA);
  return 0;
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
      if (NICInfo[i].sendND==true && NICInfo[i].recvND==true && NICInfo[i].judge==false) 
      {
        NICInfo[i].judge=true;

        tempStampInfo.identA=myIdent;
        tempStampInfo.identB=NICInfo[i].neighborIdent;
        tempStampInfo.linkFlag=true;
        tempStampInfo.note="link up";
        struct timespec tv;
        clock_gettime(CLOCK_MONOTONIC,&tv);
        tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
        stampInfo.push_back(tempStampInfo);
        
        SendLinkInfoToMaster(myIdent,NICInfo[i].neighborIdent,true);// 网卡恢复

        clock_gettime(CLOCK_MONOTONIC,&tv);
        tempStampInfo.note="Send link up over";
        tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
        stampInfo.push_back(tempStampInfo);
        
        // PrintNeighborInfo();
        return true;
      }
      else 
      {
        return false;//不是新的NIC
      }
    }
    else if (NICInfo[i].NICName=="")//比如是NDRecvHello添加的表项，要继续完善
    {
      if (!strcmp(inet_ntoa(NICInfo[i].localAddr.sin_addr),localAddr.c_str()))
      {
        NICInfo[i].NICName=tempName;
        NICInfo[i].localMask=*((struct sockaddr_in *)(ifa->ifa_netmask));
        isFindNIC=true;
        // PrintNeighborInfo();
        return true;// 因为要发送ND
      }
    }
  }
  // 10.0.80.0/24是用来控制vm的，其网口不计入
  string unexceptAddress=inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr);
  if (isFindNIC==false && unexceptAddress.substr(0,7)!="10.0.80")// 不是控制网口不存在就添加
  {
    struct NICinfo tempNICInfo;
    // if (ifa->ifa_name[0]=='e') // sonic 
    if (!strcmp(ifa->ifa_name,MGMT_INTERFACE))// vm
    {
      tempNICInfo.eventId=0;
      tempNICInfo.NICName=ifa->ifa_name;
      tempNICInfo.isServer=false;
      tempNICInfo.isSwitch=false;
      tempNICInfo.sendND=true;//
      tempNICInfo.recvND=true;//
      tempNICInfo.judge=true;
      tempNICInfo.sleep=false;
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
      tempNICInfo.eventId=0;
      tempNICInfo.NICName=ifa->ifa_name;
      tempNICInfo.isServer=false;
      tempNICInfo.isSwitch=false;
      tempNICInfo.sendND=false;
      tempNICInfo.recvND=false;
      tempNICInfo.judge=true;
      tempNICInfo.sleep=false;
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
    PrintNeighborInfo();
    return true;
  }
  PrintNeighborInfo();
  return false;
}

ident
Ipv4GlobalRouting::GetForwardIdent(ident masterIdent)
{
  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (SameNode(masterIdent,masterMapToSock[i].masterIdent))
    {
      if (masterMapToSock[i].direct==false)
      {
        return masterMapToSock[i].inDirPath->pathNodeIdent[masterMapToSock[i].inDirPath->nodeCounter-1];
      }
      else return myIdent;// 虽然找到了该master的ident，但是是直连，所以直接返回myident
    }
  }
  return myIdent;
}

int
Ipv4GlobalRouting::SendLinkInfoToMaster(ident identA,ident identB,bool linkFlag)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  int value=0;

  // 不需要或者重连完毕
  struct MNinfo tempMNInfo;
  tempMNInfo.addr.sin_family=AF_INET;// 此时addr无实际意义
  inet_aton("255.255.255.255",&(tempMNInfo.addr.sin_addr));
  tempMNInfo.addr.sin_port=htons(0);
  tempMNInfo.srcIdent=myIdent;
  tempMNInfo.forwardIdent=myIdent;
  tempMNInfo.pathNodeIdent[0]=identA;
  tempMNInfo.pathNodeIdent[1]=identB;
  for (int i=2;i<MAX_PATH_LEN;i++)
  {
    tempMNInfo.pathNodeIdent[i].level=-1;
    tempMNInfo.pathNodeIdent[i].position=-1;
  }
  tempMNInfo.eventId=GetLocalLinkEventId(identB);
  tempMNInfo.clusterMaster=false;
  tempMNInfo.chiefMaster=false;
  tempMNInfo.reachable=true;
  tempMNInfo.keepAlive=false;
  tempMNInfo.linkFlag=linkFlag;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;
  tempMNInfo.bye=false;

  // Logfout << "SendLinkInfoToMaster masterMapToSock.size()：" << masterMapToSock.size() << endl;

  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (masterMapToSock[i].chiefMaster==true)// 先给chief master发送
    {
      // 先用udp发送
      tempMNInfo.destIdent=masterMapToSock[i].masterIdent;
      // Logfout << "nodeInDirPathTable.size():" << nodeInDirPathTable.size() << endl;
      for (int j=0,k=0;k<MAX_TF_NODE_NUM && j<nodeInDirPathTable.size();j++)
      {
        if (nodeInDirPathTable[j]->linkCounter==0 && nodeInDirPathTable[j]->dirConFlag==true)
        {
          // Logfout << "try udp" << endl;
          tempMNInfo.forwardIdent=nodeInDirPathTable[j]->pathNodeIdent[nodeInDirPathTable[j]->nodeCounter-1];
          struct sockaddr_in *localAddr=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
          localAddr=NULL;
          localAddr=GetLocalAddrByNeighborIdent(nodeInDirPathTable[j]->pathNodeIdent[1]);
          if (localAddr==NULL) break;
          m_udpClient.SendLinkInfo(*localAddr,inet_ntoa(nodeInDirPathTable[j]->nodeAddr.addr.sin_addr),tempMNInfo);
          // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
          // if (linkFlag==true) Logfout << " up";
          // else Logfout << " down";
          // Logfout << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by udp [forwardNode:" << tempMNInfo.forwardIdent.level << "." << tempMNInfo.forwardIdent.position << "][eventId:" << tempMNInfo.eventId << "]." << endl;
          k++;
        }
      }
      // 再用tcp发送
      // while (masterMapToSock[i].masterSock==-1) // 临时做法
      // {
      //   usleep(100);
      // }
      tempMNInfo.destIdent=masterMapToSock[i].masterIdent;
      tempMNInfo.forwardIdent=GetForwardIdent(masterMapToSock[i].masterIdent);

      struct linkinforesponse tempLinkInfoResponse;
      tempLinkInfoResponse.unRecvNum=1;
      tempLinkInfoResponse.tempMNInfo=tempMNInfo;
      linkInfoResponse.push_back(tempLinkInfoResponse);

      value=m_tcpRoute->SendMessageTo(masterMapToSock[i].masterSock,tempMNInfo);
      // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
      // if (linkFlag==true) Logfout << " up";
      // else Logfout << " down";
      // Logfout << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by tcp [value:" << value << "][eventId:" << tempMNInfo.eventId << "][sock:" << masterMapToSock[i].masterSock << "]." << endl;
      
      if (value>0)// 发送成功
      {
        threadparamE *tempThreadParam=new threadparamE();
        tempThreadParam->tempGlobalRouting=this;
        tempThreadParam->tempMNInfo=tempMNInfo;

        if (pthread_create(&checkResponse_thread,0,CheckResponseThread,(void*)tempThreadParam)!=0)
        {
          // Logfout << GetNow() << "Create CheckResponseThread Failed[eventId:" << tempMNInfo.eventId << "]." << endl;
          exit(1);
        }
      }
      else 
      {
        pthread_mutex_lock(&mutexA);
        delayMNInfo.push_back(tempMNInfo);
        for (int i=0;i<masterMapToSock.size();i++)
        {
          if (masterMapToSock[i].masterSock>0)// 套接字大于0，但已经无法与master建立连接，则重连
          {
            masterMapToSock[i].masterSock=-1;
            m_tcpRoute->SendHelloTo(masterMapToSock[i].masterIdent,masterMapToSock[i].masterAddr);
          }
        }
        pthread_mutex_unlock(&mutexA);
        // PrintMasterMapToSock();
      }
      break;
    }
  }  

  // 上传信息给slave master by tcp
  // for (int i=0;i<masterMapToSock.size();)
  // {
  //   if (masterMapToSock[i].chiefMaster==true) 
  //   {
  //     i++;
  //     continue;
  //   }
  //   if (masterMapToSock[i].masterSock==-1) // 临时做法
  //   {
  //     usleep(100);
  //     continue;
  //   }
  //   tempMNInfo.destIdent=masterMapToSock[i].masterIdent;
  //   tempMNInfo.forwardIdent=GetForwardIdent(masterMapToSock[i].masterIdent);
  //   value=m_tcpRoute->SendMessageTo(masterMapToSock[i].masterSock,tempMNInfo);
  //   Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
  //   if (linkFlag==true) Logfout << " up";
  //   else Logfout << " down";
  //   Logfout << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by tcp [value:" << value << "][eventId:" << tempMNInfo.eventId << "]." << endl;
  //   i++;
  // }
  // PrintNeighborInfo();
  // Logfout.close();
  return value;
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
          ident tempIdent;
          tempIdent.level=-1;
          tempIdent.position=-1;

          if (!strcmp(ifa->ifa_name,MGMT_INTERFACE))
          {
            Logfout << GetNow() << "NIC " << ifa->ifa_name << "(" << inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr) << ") UP." << endl;
            
            for (int i=0;i<tempGlobalRouting->masterAddressSet.size();i++)
            {
              if (!tempGlobalRouting->IsLegalMaster(tempGlobalRouting->masterAddressSet[i].masterAddress[0])) tempTCPRoute->SendHelloTo(tempIdent,tempGlobalRouting->masterAddressSet[i].masterAddress[0]);
            }
          }
          // else if (ifa->ifa_name[0]=='E')// sonic
          else
          {
            // Logfout << GetNow() << "NIC " << ifa->ifa_name << "(" << inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr) << ") UP." << endl;
            if (tempGlobalRouting->IsNewNeighbor(ifa->ifa_name)) // 判断是否为新的邻居
            {
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
            // else
            // {
            //   // Logfout << GetNow() << "Old neighbor." << endl;
            // }
            // for (int i=0;i<tempGlobalRouting->masterAddressSet.size();i++)
            // {
            //   if (!tempGlobalRouting->IsLegalMaster(tempGlobalRouting->masterAddressSet[i].masterAddress[0])) tempTCPRoute->SendHelloTo(tempIdent,tempGlobalRouting->masterAddressSet[i].masterAddress[0]);
            // }
          }
        }
      }
      ifa=ifa->ifa_next;
    }
    freeifaddrs(ifa);

    for (auto iter=tempGlobalRouting->NICInfo.begin();iter!=tempGlobalRouting->NICInfo.end();)
    {
      // 网口或者链路失效后，邻居关系依然保存，所以以下是判断是否为正常的邻居关系转为异常
      if ((*iter).flag==false && (*iter).sendND==true && (*iter).recvND==true && (*iter).judge==true)// 网卡或者链路出故障了
      {
        // Logfout << GetNow() << "NIC " << (*iter).NICName << "(" << inet_ntoa((*iter).localAddr.sin_addr) << ") DOWN." << endl;
        if ((*iter).NICName==MGMT_INTERFACE) // 管理网口故障，则检查是否通过管理网口与master建立tcp连接
        {
          // 判断是否要和master重连
          // for (auto tempIter=tempGlobalRouting->masterMapToSock.begin();tempIter!=tempGlobalRouting->masterMapToSock.end();)
          // {
          //   if ((*iter).NICName==(*tempIter).NICName) // 本地网口故障导致master需要重连
          //   {
          //     shutdown((*tempIter).masterSock,SHUT_RDWR);
          //     (*tempIter).masterSock=-1;
          //     tempTCPRoute->SendHelloTo((*tempIter).masterIdent,(*tempIter).masterAddr);
          //   }
          //   tempIter++;
          // }
        }
        else// 非管理网口故障，要先修改路径表
        {
          tempGlobalRouting->tempStampInfo.identA=tempGlobalRouting->myIdent;
          tempGlobalRouting->tempStampInfo.identB=(*iter).neighborIdent;
          tempGlobalRouting->tempStampInfo.linkFlag=false;
          tempGlobalRouting->tempStampInfo.note="link down";
          struct timespec tv;
          clock_gettime(CLOCK_MONOTONIC,&tv);
          tempGlobalRouting->tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          tempGlobalRouting->stampInfo.push_back(tempGlobalRouting->tempStampInfo);

          if ((*iter).sleep==false)
          {
            (*iter).sleep=true;
            ident high,low;
            if (tempGlobalRouting->myIdent.level<(*iter).neighborIdent.level) 
            {
              high=(*iter).neighborIdent;
              low=tempGlobalRouting->myIdent;
            }
            else if (tempGlobalRouting->myIdent.level>(*iter).neighborIdent.level)
            {
              high=tempGlobalRouting->myIdent;
              low=(*iter).neighborIdent;
            }

            // tempGlobalRouting->tempStampInfo.note="(tcp)HandleMessage start";
            // clock_gettime(CLOCK_MONOTONIC,&tv);
            // tempGlobalRouting->tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
            // tempGlobalRouting->stampInfo.push_back(tempGlobalRouting->tempStampInfo);

            tempGlobalRouting->ModifyPathEntryTable(high,low,false);

            // clock_gettime(CLOCK_MONOTONIC,&tv);
            // tempGlobalRouting->tempStampInfo.note="(tcp)HandleMessage over";
            // tempGlobalRouting->tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
            // tempGlobalRouting->stampInfo.push_back(tempGlobalRouting->tempStampInfo);
          } 
        }
        
        // 如果是到master的直连挂了，也可以上报，但是还没有写和master有关的ND
        // if ((*iter).NICName[0]!='e')// sonic
        if (strcmp((*iter).NICName.c_str(),MGMT_INTERFACE))// 此处处理非管理网口，管理网口关闭，直连断掉需要另外处理
        {
          tempGlobalRouting->SendLinkInfoToMaster(tempGlobalRouting->myIdent,(*iter).neighborIdent,false);

          struct timespec tv;
          clock_gettime(CLOCK_MONOTONIC,&tv);
          tempGlobalRouting->tempStampInfo.identA=tempGlobalRouting->myIdent;
          tempGlobalRouting->tempStampInfo.identB=(*iter).neighborIdent;
          tempGlobalRouting->tempStampInfo.linkFlag=false;
          tempGlobalRouting->tempStampInfo.note="Send link down over";
          tempGlobalRouting->tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
          tempGlobalRouting->stampInfo.push_back(tempGlobalRouting->tempStampInfo);
        }

        (*iter).judge=false;
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
  Logfout << inet_ntoa(tempPathTableEntry->nextHopAddr.addr.sin_addr); 
  Logfout << endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintNodeLinkTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/NodeLinkTable-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  int num=0;
  struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
  tempLinkTableEntry=headNodeLinkTableEntry->next;
  // Logfout << "Num:" << headMasterLinkTableEntry->lastUpdateTime << endl;//用来统计链路数量
  Logfout << "Node--Node\tlinkFlag\tlastLinkFlag\tisStartTimer\teventId" << endl;
  while (tempLinkTableEntry!=NULL)
  {
    Logfout << tempLinkTableEntry->high.level << "." << tempLinkTableEntry->high.position << "--" << tempLinkTableEntry->low.level << "." << tempLinkTableEntry->low.position << "\t";
    Logfout << tempLinkTableEntry->linkFlag << "\t\t" << tempLinkTableEntry->lastLinkFlag << "\t\t" << tempLinkTableEntry->isStartTimer << "\t\t" << tempLinkTableEntry->eventId << endl;
    tempLinkTableEntry=tempLinkTableEntry->next;
    num++;
  }
  Logfout << "Num:" << num << endl;
  // if (myIdent.level==0 && myIdent.position==0) system("cp -f /var/log/MasterLinkTable-0.0.txt /home/guolab/MasterLinkTable.txt");
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintNeighborInfo()
{
  // pthread_mutex_lock(&mutexA);
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/NeighborInfo-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "NeighborIdent\tNICName\tLocalAddress\tNeighborAddress\tFlag\tEventId" << endl;
  for (int i=0;i<NICInfo.size();i++)
  {
    Logfout << NICInfo[i].neighborIdent.level << "." << NICInfo[i].neighborIdent.position << "\t\t" << NICInfo[i].NICName << "\t";
    Logfout << inet_ntoa(NICInfo[i].localAddr.sin_addr) << "\t" << inet_ntoa(NICInfo[i].neighborAddr.sin_addr) << "\t" << NICInfo[i].flag << "\t";
    Logfout << NICInfo[i].eventId << endl;
  }

  Logfout.close();
  // pthread_mutex_unlock(&mutexA);
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
    
    Logfout << "nextHop[" << inet_ntoa((*iter).nextHopAddr.addr.sin_addr) << "]\t";

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

struct pathtableentry *
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
          // struct sockaddr_in localAddr,remoteAddr;
          // remoteAddr=tempPathTableEntry->nodeAddr.addr;
          // localAddr=tempPathTableEntry->nodeAddr.addr;
          // remoteAddr.sin_port=htons(ND_PORT);
          // GetLocalAddrByNeighborIdent(&localAddr,tempPathTableEntry->pathNodeIdent[1]);
          // struct NDinfo tempNDInfo;
          // tempNDInfo.myIdent=srcIdent;
          // tempNDInfo.localAddr=localAddr;
          // m_udpClient.SendNDTo(localAddr,remoteAddr,tempNDInfo);
          // 首先建立tcp连接，再转发信息
          // Logfout << GetNow() << "1111111111" << endl;
          // m_tcpRoute->SendHelloTo(inet_ntoa(tempPathTableEntry->nodeAddr.addr.sin_addr));
          // sleep(3);
          // Logfout << GetNow() << "3333333333" << endl;
          return tempPathTableEntry;
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
  return NULL;
  Logfout.close();
}

void 
Ipv4GlobalRouting::TransferTo(struct MNinfo tempMNInfo)// udp，转发至chief或者node
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (myIdent.level!=0)
  {
    if (tempMNInfo.destIdent.level==0 || tempMNInfo.destIdent.level==-1)// node转发至chief或者所有master
    {
      if (GetAddrByNICName(MGMT_INTERFACE))// 检查管理网卡是否正常
      {
        struct sockaddr_in *MGMTAddr=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        MGMTAddr=NULL;
        MGMTAddr=GetAddrByNICName(MGMT_INTERFACE);

        if (MGMTAddr!=NULL)
        {
          ident chiefMasterIdent=GetChiefMasterIdent();

          for (int i=0;i<masterAddressSet.size();i++)
          {
            if (SameNode(masterAddressSet[i].masterIdent,chiefMasterIdent))
            {
              for (int j=1;j<masterAddressSet[i].masterAddress.size();j++)
              {
                m_udpClient.SendLinkInfo(*MGMTAddr,masterAddressSet[i].masterAddress[j],tempMNInfo);
                // Logfout << GetNow() << "Transfer linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
                // if (tempMNInfo.linkFlag==true) Logfout << " up";
                // else if (tempMNInfo.linkFlag==false) Logfout << " down";
                // Logfout << " from Node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
                // Logfout << " to Master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[" << masterAddressSet[i].masterAddress[j] << "] by udp." << endl;
              }
              break;
            }
          }
        }
      }
    }
    else // 转发至目的node
    {
      struct pathtableentry *tempPathTableEntry=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
      tempPathTableEntry=NULL;
      tempPathTableEntry=GetPathToNode(tempMNInfo.destIdent);
      if (tempPathTableEntry==NULL)
      {
        // Logfout << GetNow() << "Can't transfer linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
        // if (tempMNInfo.linkFlag==true) Logfout << " up";
        // else if (tempMNInfo.linkFlag==false) Logfout << " down";
        // Logfout << " from Master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
        // Logfout << " to Node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by udp." << endl;
      }
      else 
      {
        struct sockaddr_in *localAddr=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        localAddr=GetLocalAddrByNeighborIdent(tempPathTableEntry->pathNodeIdent[1]);
        if (localAddr==NULL)
        {
          // Logfout << GetNow() << "Can't transfer linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
          // if (tempMNInfo.linkFlag==true) Logfout << " up";
          // else if (tempMNInfo.linkFlag==false) Logfout << " down";
          // Logfout << " from Master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
          // Logfout << " to Node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by udp." << endl;
        }
        else
        {
          m_udpClient.SendLinkInfo(*localAddr,inet_ntoa(tempPathTableEntry->nodeAddr.addr.sin_addr),tempMNInfo);
          // Logfout << GetNow() << "Transfer linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
          // if (tempMNInfo.linkFlag==true) Logfout << " up";
          // else if (tempMNInfo.linkFlag==false) Logfout << " down";
          // Logfout << " from Master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
          // Logfout << " to Node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by udp." << endl;
        }
      }
    }
  }
  else
  {
    // Logfout << GetNow() << "Master can't transfer linkInfo " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
    // if (tempMNInfo.linkFlag==true) Logfout << " up";
    // else if (tempMNInfo.linkFlag==false) Logfout << " down";
    // Logfout << " from ";
    // if (tempMNInfo.srcIdent.level==0) Logfout << "Master ";
    // else Logfout << "Node ";
    // Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to ";
    // if (tempMNInfo.destIdent.level==0) Logfout << "Master ";
    // else Logfout << "Node ";
    // Logfout << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by udp." << endl;
  }
  
  // Logfout.close();
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
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);


  // Logfout << "AddSingleRoute:" << inet_ntoa(destAddr.sin_addr) << "/" << prefixLen << ",NICName:" << tempNextHopAndWeight.NICName << " nextHop gateway:"; 
  // Logfout << inet_ntoa(tempNextHopAndWeight.gateAddr.sin_addr) << endl;

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
  if (tempNextHopAndWeight.gateAddr.sin_addr.s_addr==destAddr.sin_addr.s_addr) req.r.rtm_scope=RT_SCOPE_LINK;//
  else req.r.rtm_scope=RT_SCOPE_UNIVERSE;//
  req.r.rtm_type=RTN_UNICAST;//
  req.r.rtm_dst_len=prefixLen;//
  
  // mtu没设置
  int bytelen=(req.r.rtm_family==AF_INET)?4:16;
  unsigned long nextgw_addr=tempNextHopAndWeight.gateAddr.sin_addr.s_addr;
  unsigned long srcgw_addr=tempNextHopAndWeight.srcAddr.sin_addr.s_addr;

  // Logfout << "Before addattr_l: " << nextgw_addr <<" " << destAddr.sin_addr.s_addr <<" " << srcgw_addr << endl;

  destAddr.sin_addr.s_addr=Convert(destAddr,prefixLen);
  addattr_l(&req.n,sizeof(req),RTA_DST,&(destAddr.sin_addr.s_addr),bytelen);//目的地址
  addattr32(&req.n,sizeof(req),RTA_PRIORITY,NL_DEFAULT_ROUTE_METRIC);//metric
  addattr_l(&req.n,sizeof(req),RTA_GATEWAY,&nextgw_addr,bytelen);//网关，单路径好像不要加上网关
  // addattr_l(&req.n,sizeof(req),RTA_PREFSRC,&srcgw_addr,bytelen);//src
  addattr_l(&req.n,sizeof(req),RTA_OIF,&if_index,bytelen);//该路由项的输出网络设备索引
  
  // Logfout << "After addattr_l: " << nextgw_addr <<" " << destAddr.sin_addr.s_addr <<" " << srcgw_addr << endl;

  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  // fprintf(stderr,"send return %d\n",status);
  close(rt_sock);
  // Logfout.close();
}

void
Ipv4GlobalRouting::AddMultiRoute(struct sockaddr_in destAddr,unsigned int prefixLen,vector<struct nexthopandweight> nextHopAndWeight)
{    
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // // test
  // Logfout << "AddMultiRoute:" << inet_ntoa(destAddr.sin_addr) << "/" << prefixLen << endl;
  // for (int i=0;i<nextHopAndWeight.size();i++)
  // {
  //   Logfout << "NICName:" << nextHopAndWeight[i].NICName << " weight:" << nextHopAndWeight[i].weight << endl;
  // }
  // // end

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
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

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
  // Logfout << "--------------DelRoute:" << inet_ntoa(destAddr.sin_addr) << "/" << prefixLen << endl;
  // Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateAddrSet(ident pathNodeIdentA,ident pathNodeIdentB,int nodeCounter,vector<struct addrset> addrSet)//A是目的结点，B是倒数第二个结点
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

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
        // 两条路径长度必须相等，最后一个结点也必须相同
        if (nodeCounter==tempPathTableEntry->nodeCounter && SameNode(pathNodeIdentA,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]))
        {
          // 且linkCounter为0，0表示故障链路数为0
          if (tempPathTableEntry->linkCounter==0)
          {
            // while (1)
            // {
            //   NICName=GetNICNameByRemoteAddr(tempPathTableEntry->nextHopAddr.addr);
            //   if (NICName=="")// 还没有探测到这个邻居
            //   {
            //     // Logfout << GetNow() << "!!!!!!!!!!!!!!" << endl;
            //     usleep(100000);//休眠100ms
            //   }
            //   else break;//否则继续进行
            // }
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
          }
          if (tempPathTableEntry->next==NULL) break;
          else tempPathTableEntry=tempPathTableEntry->next;
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
        // 两条路径长度必须相等，最后两个结点也必须相同
        if (nodeCounter==tempPathTableEntry->nodeCounter && SameNode(pathNodeIdentA,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-1]) && SameNode(pathNodeIdentB,tempPathTableEntry->pathNodeIdent[tempPathTableEntry->nodeCounter-2]))
        {
          // 且linkCounter为0，0表示故障链路数为0
          if (tempPathTableEntry->linkCounter==0)
          {
            // while (1)
            // {
            //   NICName=GetNICNameByRemoteAddr(tempPathTableEntry->nextHopAddr.addr);
            //   if (NICName=="")// 还没有探测到这个邻居
            //   {
            //     // Logfout << GetNow() << "!!!!!!!!!!!!!!" << endl;
            //     usleep(100000);//休眠100ms
            //   }
            //   else break;//否则继续进行
            // }
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
              // PrintPathEntry(tempPathTableEntry);
            }
          }
          if (tempPathTableEntry->next==NULL) break;
          else tempPathTableEntry=tempPathTableEntry->next;
        }
        else break;
      }
    }
  } 

  // Logfout << "------------------" << endl << "addrSet:";
  // for (int i=0;i<addrSet.size();i++) Logfout << inet_ntoa(addrSet[i].addr.sin_addr) << "\t";
  // Logfout << endl << "weight:" << endl;
  // for (int i=0;i<nextHopAndWeight.size();i++) Logfout << nextHopAndWeight[i].NICName << "--" << nextHopAndWeight[i].weight << "--" << inet_ntoa(nextHopAndWeight[i].gateAddr.sin_addr) << "\t";
  // Logfout << endl;
  // Logfout << "------------------" << endl;

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
  // Logfout.close();
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
  // if (newPathTableEntry->nodeCounter!=1) InsertNodeInDirPathTable(newPathTableEntry);// 避免将自己添加进去

  // 更新Node维护的链路表
  ident high,low;
  for (int i=1;i<newPathTableEntry->nodeCounter-1;i++)// 直连链路不进入该表
  {
    if (newPathTableEntry->pathNodeIdent[i].level<newPathTableEntry->pathNodeIdent[i+1].level)
    {
      high=newPathTableEntry->pathNodeIdent[i+1];
      low=newPathTableEntry->pathNodeIdent[i];
    }
    else 
    {
      high=newPathTableEntry->pathNodeIdent[i];
      low=newPathTableEntry->pathNodeIdent[i+1];
    }
    UpdateNodeLinkTable(high,low,0,true,1);
  }

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
  // Logfout << "AddNewPathTableEntry over!" << endl;
  Logfout.close();
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
  Logfout << "MasterAddr\tMasterIdent\tMasterSock\tNICName\tchiefMaster\tDirect\tMiddleAddr\tKeepAliveFaildNum\tKeepAliveFlag\tInDirPath" << endl;
  for (int i=0;i<masterMapToSock.size();i++)
  {
    Logfout << masterMapToSock[i].masterAddr << "\t" << masterMapToSock[i].masterIdent.level << "." << masterMapToSock[i].masterIdent.position << "\t\t";
    Logfout << masterMapToSock[i].masterSock << "\t\t" << masterMapToSock[i].NICName << "\t" << masterMapToSock[i].chiefMaster << "\t\t" << masterMapToSock[i].direct << "\t";
    if (masterMapToSock[i].middleAddr=="") Logfout << "\t";
    else Logfout << masterMapToSock[i].middleAddr;
    Logfout << "\t" << masterMapToSock[i].keepAliveFaildNum << "\t\t\t" << masterMapToSock[i].recvKeepAlive << "\t\t\t";
    if (masterMapToSock[i].inDirPath!=NULL)
    {
      for (int j=0;j<MAX_PATH_LEN;j++)
      {
        if (masterMapToSock[i].inDirPath->pathNodeIdent[j].level!=-1) Logfout << masterMapToSock[i].inDirPath->pathNodeIdent[j].level << "." << masterMapToSock[i].inDirPath->pathNodeIdent[j].position << "--";
        else break;
      }
    }
    Logfout << endl;
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
Ipv4GlobalRouting::CheckMasterMapToSock()// 更新完路径表中的链路或者直连标志后都要检查和master的连接的间接路径是否受到影响
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  for (int i=0;i<masterMapToSock.size();i++)
  {
    if (masterMapToSock[i].inDirPath!=NULL)
    {
      if (masterMapToSock[i].inDirPath->linkCounter!=0 || masterMapToSock[i].inDirPath->dirConFlag==false)// 与master连接的该间接路径受到影响，需要重连
      {
        shutdown(masterMapToSock[i].masterSock,SHUT_RDWR);
        masterMapToSock[i].masterSock=-1;
        Logfout << GetNow() << "I need to reconnect with Master " << masterMapToSock[i].masterIdent.level << "." << masterMapToSock[i].masterIdent.position << "[" << masterMapToSock[i].masterAddr << "]." << endl;
        m_tcpRoute->SendHelloTo(masterMapToSock[i].masterIdent,masterMapToSock[i].masterAddr);
      }
    }
  }

  Logfout.close();
}

void 
Ipv4GlobalRouting::UpdateNodeInDirPathTable()// 修改路径表或者直连标志位后，需要检查间接路径表是否需要更新
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

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
            // PrintNodeInDirPathTable();
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
            // PrintNodeInDirPathTable();
            break;
          }
          tempPathTableEntry=tempPathTableEntry->next;
        }
      }
    }
  }
  // CheckMasterMapToSock();// 更新完路径表中的链路或者直连标志后都要检查和master的连接的间接路径是否受到影响
  // PrintNodeInDirPathTable();
  // Logfout.close();
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
    // UpdateNodeInDirPathTable();
    PrintPathEntryTable();
  }
  Logfout.close();
}

void 
Ipv4GlobalRouting::ModifyPathEntryTable(ident high,ident low,bool linkFlag)// 修改路径表，修改linkCounter
{
  // pthread_mutex_lock(&mutexA);
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  vector<struct mappingtableentry> tempMappingTable;
  GetMappingTableEntry(high,low,&tempMappingTable);// 获取相关的映射表
  // Logfout << "---------------" << endl;
  // Logfout << "linkFlag:" << linkFlag << endl;
  
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
    // Logfout << GetNow() << "Didn't find mapping entry." << endl;
    return;
  }

  int diffLinkCounter=0;// 记录遍历过程中，对应位置链路不同的情况次数，用来做判断终止条件
  // Logfout << "映射表项数：" << tempMappingTable.size() << endl;
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
    int tempNodeCounter=0;// 路径长度
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
            if (tempAddrSet.size()>0) UpdateAddrSet(tempDestIdent,tempIdent,tempNodeCounter,tempAddrSet);
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
        // Logfout << "isNeedToUpdateRoute:" << isNeedToUpdateRoute << endl;
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
      if (tempAddrSet.size()>0) UpdateAddrSet(tempDestIdent,tempIdent,tempNodeCounter,tempAddrSet);
      tempAddrSet.clear();
      // 再更新到节点的地址
      tempAddrSet.push_back(tempNodeAddr);
      // Logfout << GetNow() << "***************要更新的Node地址：" << inet_ntoa(tempAddrSet[0].addr.sin_addr) << endl;
      UpdateAddrSet(tempDestIdent,tempNextIdent,tempNodeCounter,tempAddrSet);
    }
  }
  // UpdateNodeInDirPathTable();
  // PrintPathEntryTable();
  // Logfout.close();
  // pthread_mutex_unlock(&mutexA);
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

  int num=0;
  struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
  tempLinkTableEntry=headMasterLinkTableEntry->next;
  Logfout << "Node--Node\tlinkFlag\tlastLinkFlag\tisStartTimer\teventId" << endl;
  while (tempLinkTableEntry!=NULL)
  {
    Logfout << tempLinkTableEntry->high.level << "." << tempLinkTableEntry->high.position << "--" << tempLinkTableEntry->low.level << "." << tempLinkTableEntry->low.position << "\t";
    Logfout << tempLinkTableEntry->linkFlag << "\t\t" << tempLinkTableEntry->lastLinkFlag << "\t\t" << tempLinkTableEntry->isStartTimer << "\t\t" << tempLinkTableEntry->eventId << endl;
    tempLinkTableEntry=tempLinkTableEntry->next;
    num++;
  }
  Logfout << "Num:" << num << endl;
  if (myIdent.level==0 && myIdent.position==0) system("cp -f /var/log/MasterLinkTable-0.0.txt /home/guolab/MasterLinkTable.txt");
  Logfout.close();
}

void 
Ipv4GlobalRouting::PrintNodeMapToSock()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/NodeMapToSock-" << myIdent.level << "." << myIdent.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Num:" << nodeMapToSock.size() << endl;
  Logfout << "Node\tNodeAddress\tSock\tDirect" << endl;
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    Logfout << nodeMapToSock[i].nodeIdent.level << "." << nodeMapToSock[i].nodeIdent.position << "\t" << nodeMapToSock[i].nodeAddress << "\t" << nodeMapToSock[i].nodeSock << "\t" << nodeMapToSock[i].direct << endl;
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
  tempMNInfo.addr=*(tempGlobalRouting->GetAddrByNICName(MGMT_INTERFACE));
  tempMNInfo.forwardIdent=tempGlobalRouting->myIdent;
  tempMNInfo.srcIdent=tempGlobalRouting->myIdent;
  tempMNInfo.pathNodeIdent[0].level=-1;// 全部置为-1表示当前的chief Master决定放弃chief Master的地位
  tempMNInfo.pathNodeIdent[0].position=-1;
  tempMNInfo.pathNodeIdent[1].level=tempGlobalRouting->inDirNodeNum;
  tempMNInfo.pathNodeIdent[1].position=tempGlobalRouting->inDirNodeNum;
  for (int i=2;i<MAX_PATH_LEN;i++)
  {
    tempMNInfo.pathNodeIdent[i].level=-1;
    tempMNInfo.pathNodeIdent[i].position=-1;
  }
  tempMNInfo.eventId=-1;
  tempMNInfo.clusterMaster=true;
  tempMNInfo.chiefMaster=false;
  tempMNInfo.reachable=true;
  tempMNInfo.keepAlive=false;
  tempMNInfo.linkFlag=false;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;
  tempMNInfo.bye=false;
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
          // struct MNinfo stopMNInfo;
          // stopMNInfo.addr.sin_family=AF_INET;
          // inet_aton("255.255.255.255",&(stopMNInfo.addr.sin_addr));
          // stopMNInfo.addr.sin_port=htons(0);
          // stopMNInfo.addr=*(tempGlobalRouting->GetAddrByNICName(MGMT_INTERFACE));
          // stopMNInfo.destIdent.level=-1;// 此时还不知道master的ident
          // stopMNInfo.destIdent.position=-1;
          // stopMNInfo.forwardIdent=tempGlobalRouting->myIdent;
          // stopMNInfo.srcIdent=tempGlobalRouting->myIdent;
          // stopMNInfo.pathNodeIdent[0]=tempGlobalRouting->myIdent;
          // stopMNInfo.pathNodeIdent[1]=tempGlobalRouting->myIdent;
          // for (int i=2;i<MAX_PATH_LEN;i++)
          // {
          //   stopMNInfo.pathNodeIdent[i].level=-1;
          //   stopMNInfo.pathNodeIdent[i].position=-1;
          // }
          // stopMNInfo.clusterMaster=false;
          // stopMNInfo.chiefMaster=tempGlobalRouting->chiefMaster;
          // stopMNInfo.reachable=true;
          // stopMNInfo.keepAlive=false;
          // stopMNInfo.linkFlag=false;
          // stopMNInfo.hello=false;
          // stopMNInfo.ACK=false;
          // stopMNInfo.bye=false;
          // 准备发送，以后写
          system("echo \"————————————————————————————————\" >> /home/guolab/output/ATCTest-primus.log"); 
          string command;
          command="echo \"   Can not connect with node "+to_string(tempGlobalRouting->nodeMapToSock[i].nodeIdent.level)+"."+to_string(tempGlobalRouting->nodeMapToSock[i].nodeIdent.position)+".\" >> /home/guolab/output/ATCTest-primus.log";
          system(command.c_str());
          system("echo \"   Go back to BGP......\" >> /home/guolab/output/ATCTest-primus.log"); 
          system("echo \"————————————————————————————————\" >> /home/guolab/output/ATCTest-primus.log"); 
          // system("pssh ")
          // pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus/Primus
          // system("/home/guolab/script/stop.sh");
          // system("bash /home/guolab/script/stopPrimusTest.sh");     
        }
        tempGlobalRouting->nodeMapToSock[i].keepAliveFaildNum=0;
      }
    }
    sleep(tempGlobalRouting->m_defaultKeepaliveTimer);
  }
  Logfout << GetNow() << "Master " << tempGlobalRouting->myIdent.level << "." << tempGlobalRouting->myIdent.position << "'s ListenKeepAliveThread down." << endl;
  Logfout.close();
  // pthread_exit(0);
}

void* 
Ipv4GlobalRouting::MasterLinkTimer(void* threadParam)
{
  pthread_detach(pthread_self());
  Ipv4GlobalRouting *tempGlobalRouting=((threadparamD *)threadParam)->tempGlobalRouting;
  struct linktableentry *tempLinkTableEntry=((threadparamD *)threadParam)->tempLinkTableEntry;
  ident srcIdent=((threadparamD *)threadParam)->srcIdent;

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

  tempLinkTableEntry->isStartTimer=false;
  tempLinkTableEntry->lastUpdateTime=tempGlobalRouting->GetSystemTime();
  tempLinkTableEntry->linkUpdateTimer=tempGlobalRouting->m_defaultLinkTimer;

  if (tempLinkTableEntry->lastLinkFlag!=tempLinkTableEntry->linkFlag)//两个状态不同则处理
  {
    Logfout << GetNow() << "Different link flag." << endl;
    tempLinkTableEntry->linkFlag=tempLinkTableEntry->lastLinkFlag;
    tempGlobalRouting->SendMessageToNode(tempLinkTableEntry->high,tempLinkTableEntry->low,srcIdent,tempLinkTableEntry->eventId,tempLinkTableEntry->lastLinkFlag);
  }
  // tempGlobalRouting->PrintMasterLinkTable();//故障定位
  Logfout << GetNow() << "Master link timer timed out,system time:" << tempGlobalRouting->GetSystemTime() << "s." << endl;
  Logfout.close();
  pthread_exit(0);
}

bool 
Ipv4GlobalRouting::UpdateNodeLinkTable(ident high,ident low,int eventId,bool linkFlag,int cmd)// cmd=1，插入新的链路；cmd=2，查询链路
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (cmd==1)// 插入新的链路
  {
    // 此部分返回值
    // true表示添加新的链路成功，false表示存在这样的链路
    // 如果添加失败呢？交给林boss 来处理吧，哈哈哈
    struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
    tempLinkTableEntry=headNodeLinkTableEntry->next;// 获得Node维护的链路表的头节点

    while (tempLinkTableEntry!=NULL)
    {
      if (SameNode(high,tempLinkTableEntry->high) && SameNode(low,tempLinkTableEntry->low))// 存在这样的链路
      {
        // Logfout.close();
        return false;
      }
      tempLinkTableEntry=tempLinkTableEntry->next;
    }
    // 不存在则添加
    struct linktableentry *newLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
    newLinkTableEntry->high=high;
    newLinkTableEntry->low=low;
    newLinkTableEntry->eventId=0;// 只有以上三个值有效
    newLinkTableEntry->linkFlag=true;
    newLinkTableEntry->lastLinkFlag=true;
    newLinkTableEntry->lastUpdateTime=GetSystemTime();
    newLinkTableEntry->linkUpdateTimer=m_defaultLinkTimer;
    newLinkTableEntry->isStartTimer=false;
    newLinkTableEntry->next=NULL;

    struct linktableentry *currentLinkTableEntry=headNodeLinkTableEntry;
    struct linktableentry *nextLinkTableEntry=headNodeLinkTableEntry->next;
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
    headNodeLinkTableEntry->lastUpdateTime++;//用来统计链路数量
    PrintNodeLinkTable();
    // Logfout.close();
    return true;
  }
  // 查询链路，node会通过udp和tcp 收到多个内容相同的信息，但node只处理一次，并记录eventid
  // 所以此部分功能用来判断Node是否需要根据当前收到的信息更新路径表
  // node的链路表不维护直连链路
  else if (cmd==2)
  {
    // 现在用最low的办法
    // pthread_mutex_lock(&mutexA);
    struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
    tempLinkTableEntry=headNodeLinkTableEntry->next;// 获得Node维护的链路表的头节点

    while (tempLinkTableEntry!=NULL)
    {
      if (SameNode(high,tempLinkTableEntry->high) && SameNode(low,tempLinkTableEntry->low))// 存在这样的链路
      {
        // 查看eventid
        if (tempLinkTableEntry->eventId<eventId)// 当前记录的eventid小于新得信息得eventide，所以有必要更新
        {
          tempLinkTableEntry->eventId=eventId;
          // 或许还可以记录下这些链路的状态
          tempLinkTableEntry->lastLinkFlag=tempLinkTableEntry->linkFlag;
          tempLinkTableEntry->linkFlag=linkFlag;

          // Logfout.close();
          // pthread_mutex_unlock(&mutexA);
          return true;
        }
        else // 收到一个old message，无需处理
        {
          // Logfout.close();
          // pthread_mutex_unlock(&mutexA);
          return false;
        }
      }
      tempLinkTableEntry=tempLinkTableEntry->next;
    }
    // 找不到链路，不处理了
    // 有个风险，当前路径是由udp的拓扑发现生成的，如果过程中一条链路故障，但是node还没收到关于这条链路的路径，则会忽视
    // 收到路径后误认为路径是有效的，出现错误
    // Logfout.close();
    // pthread_mutex_unlock(&mutexA);
    return false;
  }
}

void 
Ipv4GlobalRouting::FakeGenerateSpinePath(ident tempIdent,int tempSpineNodes,int tempLeafNodes,int tempToRNodes,int tempPods)
{
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

  // tempPathInfo->pathNodeIdent[0]=tempIdent;
  int index=tempIdent.position/(tempSpineNodes/tempLeafNodes);
  // 生成第二层的下一跳

  printf("AddNewPathTableEntry start ...\n");
  for (int i=0;i<tempPods;i++)
  {
    tempPathInfo->pathNodeIdent[0].level=2;
    tempPathInfo->pathNodeIdent[0].position=tempLeafNodes*i+index;
    int DestIP1=i+1;
    int DestIP2=2;
    string IP="32.1."+to_string(DestIP1)+"."+to_string(DestIP2);
    // printf("DestIP %s\n",IP.c_str());
    inet_aton(IP.c_str(),&(tempAddr.sin_addr));
    tempPathInfo->nodeAddr.addr=tempAddr;
    tempPathInfo->nodeAddr.prefixLen=24;
    tempPathInfo->nextHopAddr.addr=tempAddr;
    tempPathInfo->nextHopAddr.prefixLen=24;
    tempPathInfo->nodeCounter=2;
    tempPathInfo->pathNodeIdent[1]=tempIdent;

    // sockaddr_in nextHopIP;
    // nextHopIP=tempPathInfo->nextHopAddr.addr;
    
    printf("AddNewPathTableEntry doing 3-2\n");
    for (int k=0;k<MAX_PATH_LEN;k++)
    {
      printf("%d.%d->",tempPathInfo->pathNodeIdent[k].level,tempPathInfo->pathNodeIdent[k].position);
    }
    printf("\n");

    AddNewPathTableEntry(*tempPathInfo);//Node调用此函数添加新的路径

    // 继续生成第三跳
    tempPathInfo->pathNodeIdent[2].level=1;
    for (int j=0;j<tempToRNodes;j++)
    {
      tempPathInfo->pathNodeIdent[0].level=1;
      tempPathInfo->pathNodeIdent[0].position=i*tempToRNodes+j;
      tempPathInfo->pathNodeIdent[1].level=2;
      tempPathInfo->pathNodeIdent[1].position=i*tempLeafNodes+index;
      tempPathInfo->pathNodeIdent[2]=tempIdent;
      int DestIP0=tempPathInfo->pathNodeIdent[0].position+1;
      int DestIP1=j+1;
      int DestIP2=2;
      string IP="21."+to_string(DestIP0)+"."+to_string(DestIP1)+"."+to_string(DestIP2);
      // printf("DestIP %s\n",IP.c_str());
      inet_aton(IP.c_str(),&(tempAddr.sin_addr));
      tempPathInfo->nodeAddr.addr=tempAddr;
      tempPathInfo->nodeAddr.prefixLen=24;

      // tempPathInfo->nextHopAddr.addr=nextHopIP;
      // tempPathInfo->nextHopAddr.prefixLen=24;
      tempPathInfo->nodeCounter=3;
      printf("AddNewPathTableEntry doing 2-1\n");
      for (int k=0;k<MAX_PATH_LEN;k++)
      {
        printf("%d.%d->",tempPathInfo->pathNodeIdent[k].level,tempPathInfo->pathNodeIdent[k].position);
      }
      printf("\n");
      AddNewPathTableEntry(*tempPathInfo);//Node调用此函数添加新的路径
    }
  }
  printf("AddNewPathTableEntry done!\n");
  PrintPathEntryTable();
  PrintMappingTable();
  PrintNodeLinkTable();
}

void 
Ipv4GlobalRouting::FakeGenerateLink(int tempSpineNodes,int tempLeafNodes,int tempToRNodes,int tempPods)
{
  ident high,low;
  high.level=3;
  low.level=2;
  for (int i=0;i<tempSpineNodes;i++)
  {
    high.position=i;
    for (int j=0;j<tempPods;j++)
    {
      low.position=j*tempLeafNodes+i/(tempSpineNodes/tempLeafNodes);
      UpdateMasterLinkTable(high,low,high,1,true);
    }
  }
  high.level=2;
  low.level=1;
  for (int i=0;i<tempPods*tempLeafNodes;i++)
  {
    high.position=i;
    for (int j=0;j<tempToRNodes;j++)
    {
      low.position=i/tempLeafNodes*tempToRNodes+j;
      UpdateMasterLinkTable(high,low,high,1,true);
    }
  }
  PrintMasterLinkTable();
}

bool 
Ipv4GlobalRouting::UpdateMasterLinkTable(ident high,ident low,ident srcIdent,int eventId,bool linkFlag)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  bool isFindMasterLink=false;// 不存在，如果linkFlag为true，则添加，否则出错
  // 遍历整个链路表，此处可优化
  // 还必须考虑链路更新信息的上报节点，通常一条链路的两端都会上报信息
  // 暂时不写吧
  struct linktableentry *tempLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
  tempLinkTableEntry=headMasterLinkTableEntry->next;
  while (tempLinkTableEntry!=NULL)
  {
    if (SameNode(high,tempLinkTableEntry->high) && SameNode(low,tempLinkTableEntry->low))// 存在这样的链路
    {
      isFindMasterLink=true;
      // 先查看事件id
      // pthread_mutex_lock(&mutexA);
      if (tempLinkTableEntry->eventId>=eventId)
      {
        // pthread_mutex_unlock(&mutexA);
        return false;// master维护的链路表的id大于收到信息的id，不处理
      } 
      tempLinkTableEntry->eventId=eventId;// 更新id
      // pthread_mutex_unlock(&mutexA);
      
      if (tempLinkTableEntry->linkFlag==true)//master的链路表中该条链路此时正常工作
      {
        // 可能是链路另一端上报的信息
        if (linkFlag==true) return false;
        else if (linkFlag==false)// 表示该条链路故障，两次链路变化的时间间隔不管是否大于定时器，都要广播该信息
        {
          tempLinkTableEntry->linkFlag=false;
          tempLinkTableEntry->lastLinkFlag==false;
          tempLinkTableEntry->lastUpdateTime=GetSystemTime(); 
          // PrintMasterLinkTable();
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
          if (chiefMaster==true)
          {
            // threadparamD *threadParam=new threadparamD();
            // threadParam->tempGlobalRouting=this;
            // threadParam->tempLinkTableEntry=tempLinkTableEntry;
            // threadParam->srcIdent=srcIdent;
            // if (pthread_create(&masterLink_thread,NULL,MasterLinkTimer,(void *)threadParam)<0)
            // {
            //   // Logfout << GetNow() << "Master could not start timer" << endl;
            //   exit(0);
            // }
            // tempLinkTableEntry->isStartTimer=true; 
            //尝试快速恢复
            if (linkFlag==true) 
            {
              // PrintMasterLinkTable();
              return true;
            }
            else return false;
          }
          else return false;        
        }
        else// 如果已经开启了定时器，则不向node转发信息
        {
          tempLinkTableEntry->linkUpdateTimer+=m_defaultLinkTimer;//等待时间累加
          return false;
        }
      }
      // PrintMasterLinkTable();
      break;
    }
    tempLinkTableEntry=tempLinkTableEntry->next;
  }
  //如果没有找到，就要添加一条这样的路径
  if (isFindMasterLink==false)
  {
    struct linktableentry *newLinkTableEntry=(struct linktableentry *)malloc(sizeof(struct linktableentry));
    newLinkTableEntry->high=high;
    newLinkTableEntry->low=low;
    newLinkTableEntry->eventId=eventId;
    newLinkTableEntry->linkFlag=linkFlag;
    newLinkTableEntry->lastLinkFlag=linkFlag;
    newLinkTableEntry->lastUpdateTime=GetSystemTime();
    newLinkTableEntry->linkUpdateTimer=m_defaultLinkTimer;
    newLinkTableEntry->isStartTimer=false;
    newLinkTableEntry->next=NULL;
    // 应该考虑开启定时器保护
    struct linktableentry *currentLinkTableEntry=headMasterLinkTableEntry;
    struct linktableentry *nextLinkTableEntry=headMasterLinkTableEntry->next;
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
    headMasterLinkTableEntry->lastUpdateTime++;//用来统计链路数量

    ident tempIdent;
    tempIdent.level=-1;
    tempIdent.position=-1;

    struct effectnode tempEffectNode;
    tempEffectNode.high=high;
    tempEffectNode.low=low;
    for (int i=0;i<MAX_EFFECT_NODE;i++)
    {
      tempEffectNode.effectNode[i]=tempIdent;
    }
    effectNodeRange.push_back(tempEffectNode);

    GenerateLinkEffectRange();
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
    // sleep(5);// 等待新的chiefmaster做好准备
    for (int i=0;i<clusterMasterInfo.size();i++)
    {
      if (SameNode(clusterMasterInfo[i].masterIdent,chiefMasterIdent))
      {
        m_tcpRoute->SendHelloTo(chiefMasterIdent,inet_ntoa(clusterMasterInfo[i].masterAddr.sin_addr));
        break;
      }
    }
    
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
  else // common master选出新的chief master，可能现在的chief master本身就是最优的，不需要重新选举
  {
    struct sockaddr_in newTempMasterAddr;
    newTempMasterAddr.sin_family=AF_INET;
    inet_aton("255.255.255.255",&(newTempMasterAddr.sin_addr));
    newTempMasterAddr.sin_port=htons(0);
    newTempMasterAddr=*(GetAddrByNICName(MGMT_INTERFACE));
    ident newTempChiefMasterIdent=myIdent;
    int newTempInDirNodeNum=inDirNodeNum;

    // 尝试找出一个新的最优master
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
    // 新选出的Chief Master是原来的
    if (SameNode(newTempChiefMasterIdent,chiefMasterIdent)) 
    {
      Logfout << GetNow() << "Master " << chiefMasterIdent.level << "." << chiefMasterIdent.position << " is still the chiefMaster." << endl;
      Logfout.close();
      return;
    }

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
      tempMNInfo.addr=*(GetAddrByNICName(MGMT_INTERFACE));
      tempMNInfo.forwardIdent=myIdent;
      tempMNInfo.srcIdent=myIdent;
      tempMNInfo.eventId=-1;
      tempMNInfo.clusterMaster=true;
      tempMNInfo.chiefMaster=true;
      tempMNInfo.reachable=true;
      tempMNInfo.keepAlive=false;
      tempMNInfo.linkFlag=false;
      tempMNInfo.hello=false;  
      tempMNInfo.ACK=false; 
      tempMNInfo.bye=false;  

      // 先通知old chief master
      tempMNInfo.pathNodeIdent[0].level=-1;
      tempMNInfo.pathNodeIdent[0].position=-1;
      tempMNInfo.pathNodeIdent[1].level=inDirNodeNum;
      tempMNInfo.pathNodeIdent[1].position=inDirNodeNum;
      for (int i=2;i<MAX_PATH_LEN;i++)
      {
        tempMNInfo.pathNodeIdent[i].level=-1;
        tempMNInfo.pathNodeIdent[i].position=-1;
      }
      tempMNInfo.destIdent=chiefMasterIdent;
      m_tcpRoute->SendMessageTo(oldChiefMasterSock,tempMNInfo);

      // 通知所有的node，chiefmaster已经发生变化，即发送一个ACK包
      tempMNInfo.pathNodeIdent[0]=myIdent;
      tempMNInfo.pathNodeIdent[1]=myIdent; 
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
      m_tcpRoute->SendHelloTo(newTempChiefMasterIdent,inet_ntoa(newTempMasterAddr.sin_addr));
    }
  }
  Logfout.close();
}

void
Ipv4GlobalRouting::UpdateInDirPath(ident nodeIdent,ident pathNodeIdent[MAX_PATH_LEN],int nodeSock)
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
      // 直接添加
      for (int j=0;j<MAX_PATH_LEN;j++) masterInDirPathTable[i].pathNodeIdent[j]=pathNodeIdent[j];
      masterInDirPathTable[i].nodeSock=nodeSock;
      // 检查是否有积压的未发送的MNInfo
      struct accumulationMNinfo *currentAccumulationMNInfo=masterInDirPathTable[i].headAccumulationMNInfo;
      while (currentAccumulationMNInfo->next!=NULL) 
      {
        currentAccumulationMNInfo=currentAccumulationMNInfo->next;
        m_tcpRoute->SendMessageTo(masterInDirPathTable[i].nodeSock,*(currentAccumulationMNInfo->tempMNInfo));
      }
      masterInDirPathTable[i].headAccumulationMNInfo->next=NULL;
      break;
    }
  }

  if (isFind==false) 
  {
    struct indirpathtableentry *tempInDirPathTableEntry=(struct indirpathtableentry *)malloc(sizeof(struct indirpathtableentry));
    for (int i=0;i<MAX_PATH_LEN;i++) tempInDirPathTableEntry->pathNodeIdent[i]=pathNodeIdent[i];
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
Ipv4GlobalRouting::UpdateNodeMapToSock(ident nodeIdent,string nodeAddress,int sock,bool direct)// master和起转发的node会使用
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
          for (auto iter=masterInDirPathTable.begin();iter!=masterInDirPathTable.end();)
          {
            if (SameNode(iter->pathNodeIdent[0],nodeIdent))// 间接路径表里要删除相关表项
            {
              iter=masterInDirPathTable.erase(iter);// 删除该项           
              // if (iter==masterInDirPathTable.end()) break;
              PrintMasterInDirPathTable();
              break;
            }
            iter++;
          }
          nodeMapToSock[i].nodeAddress=nodeAddress;
          nodeMapToSock[i].direct=direct;
          nodeMapToSock[i].nodeSock=sock;
          nodeMapToSock[i].keepAliveFaildNum=0;
          nodeMapToSock[i].recvKeepAlive=false;
          
          if (nodeIdent.level!=0 && chiefMaster==true) SendMessageToNode(nodeIdent,myIdent,nodeIdent,-1,direct);
          inDirNodeNum--;

          struct clustermasterinfo tempClusterMasterInfo;
          tempClusterMasterInfo.chiefMaster=chiefMaster;// 
          tempClusterMasterInfo.masterAddr.sin_family=AF_INET;
          inet_aton("255.255.255.255",&(tempClusterMasterInfo.masterAddr.sin_addr));
          tempClusterMasterInfo.masterAddr.sin_port=htons(0);
          tempClusterMasterInfo.masterAddr=*(GetAddrByNICName(MGMT_INTERFACE));// common master的地址
          tempClusterMasterInfo.masterIdent=myIdent;// common master的ident
          tempClusterMasterInfo.inDirNodeNum=inDirNodeNum;
          // 主动发出indirnodenum
          SendInDirNodeNumToCommon(tempClusterMasterInfo);
        }
        else if (nodeMapToSock[i].direct==true && direct==false) 
        {
          shutdown(nodeMapToSock[i].nodeSock,SHUT_RDWR);
          nodeMapToSock[i].direct=direct;
          nodeMapToSock[i].nodeSock=sock;
          nodeMapToSock[i].keepAliveFaildNum=0;
          nodeMapToSock[i].recvKeepAlive=false;

          if (nodeIdent.level!=0 && chiefMaster==true) SendMessageToNode(nodeIdent,myIdent,nodeIdent,-1,direct);
          inDirNodeNum++;
          // test
          // if (chiefMaster) inDirNodeNum+=3;
          // end

          struct clustermasterinfo tempClusterMasterInfo;
          tempClusterMasterInfo.chiefMaster=chiefMaster;//
          tempClusterMasterInfo.masterAddr.sin_family=AF_INET;
          inet_aton("255.255.255.255",&(tempClusterMasterInfo.masterAddr.sin_addr));
          tempClusterMasterInfo.masterAddr.sin_port=htons(0); 
          tempClusterMasterInfo.masterAddr=*(GetAddrByNICName(MGMT_INTERFACE));// common master的地址
          tempClusterMasterInfo.masterIdent=myIdent;// common master的ident
          tempClusterMasterInfo.inDirNodeNum=inDirNodeNum;
          // 主动发出indirnodenum
          SendInDirNodeNumToCommon(tempClusterMasterInfo);
        }
        else if (nodeMapToSock[i].direct==false && direct==false)
        {
          nodeMapToSock[i].direct=direct;
          nodeMapToSock[i].nodeSock=sock;
          nodeMapToSock[i].keepAliveFaildNum=0;
          nodeMapToSock[i].recvKeepAlive=false;
        }
        PrintNodeMapToSock();
        Logfout << GetNow() << "My inDirNodeNum:" << inDirNodeNum << "." << endl;

        // 判断是否为最优的Chief Master
        if (inDirNodeNum>MAX_INDIR_NUM && chiefMaster==true && isStartMasterElection==false)
        {
          Logfout << GetNow() << "I may be not the chiefMaster!" << endl;

          isStartMasterElection=true;
          chiefMasterStatusChange=true;
            
          // threadparamB *threadParam=new threadparamB();
          // threadParam->tempGlobalRouting=this;
          // threadParam->tempUdpClient=NULL;
          // threadParam->tempTCPRoute=m_tcpRoute;

          // if (pthread_create(&oldChiefFindNewOne_thread,NULL,OldChiefFindNewOneThread,(void *)threadParam)<0)
          // {
          //   Logfout << GetNow() << "Create OldChiefFindNewOneThread failed!" << endl;
          //   exit(0);
          // }
        }
        else chiefMasterStatusChange=false;// 标志位，chief master判断出自己不是最优后，会等待3s再询问其他master，在这期间如果chief master的状态发生变化，则还要等待3s
      }
      Logfout.close();
      return;
    }
  }

  struct nodemaptosock tempNodeMapToSock;
  tempNodeMapToSock.nodeIdent=nodeIdent;
  tempNodeMapToSock.nodeAddress=nodeAddress;
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
    tempMNInfo.forwardIdent=myIdent;
    tempMNInfo.srcIdent=myIdent;
    tempMNInfo.pathNodeIdent[0]=nodeIdent;
    tempMNInfo.pathNodeIdent[1]=myIdent;
    for (int i=2;i<MAX_PATH_LEN;i++)
    {
      tempMNInfo.pathNodeIdent[i].level=-1;
      tempMNInfo.pathNodeIdent[i].position=-1;
    }
    // 暂时没有针对直连链路维护eventid
    tempMNInfo.eventId=-1;// master获取某条链路
    tempMNInfo.clusterMaster=false;
    tempMNInfo.chiefMaster=false;
    tempMNInfo.reachable=true;
    tempMNInfo.keepAlive=false;
    tempMNInfo.linkFlag=direct;
    tempMNInfo.hello=false;
    tempMNInfo.ACK=false;
    tempMNInfo.bye=false;

    for (int i=0;i<nodeMapToSock.size();i++)
    {
      if (nodeMapToSock[i].nodeIdent.level!=0) 
      {
        // 先向已经建立连接了的Node转发这个新的连接
        tempMNInfo.destIdent=nodeMapToSock[i].nodeIdent;
        tempMNInfo.pathNodeIdent[0]=nodeIdent;
        m_tcpRoute->SendMessageTo(GetSockByIdent(nodeMapToSock[i].nodeIdent),tempMNInfo);
        // 然后再向新建立连接的Node转发之前已经和Master建立了的连接
        tempMNInfo.destIdent=nodeIdent;
        tempMNInfo.pathNodeIdent[0]=nodeMapToSock[i].nodeIdent;
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
  tempMNInfo.forwardIdent=myIdent;
  tempMNInfo.srcIdent=myIdent;
  tempMNInfo.pathNodeIdent[0]=tempClusterMasterInfo.masterIdent;// 
  tempMNInfo.pathNodeIdent[1].level=tempClusterMasterInfo.inDirNodeNum;// pathNodeIdentB有效，值为indirnodenum
  tempMNInfo.pathNodeIdent[1].position=tempClusterMasterInfo.inDirNodeNum;
  for (int i=2;i<MAX_PATH_LEN;i++)
  {
    tempMNInfo.pathNodeIdent[i].level=-1;
    tempMNInfo.pathNodeIdent[i].position=-1;
  }
  tempMNInfo.eventId=-1;
  tempMNInfo.clusterMaster=true;
  tempMNInfo.chiefMaster=tempClusterMasterInfo.chiefMaster;
  tempMNInfo.keepAlive=false;
  tempMNInfo.reachable=true;
  tempMNInfo.linkFlag=false;
  tempMNInfo.hello=false;
  tempMNInfo.ACK=false;
  tempMNInfo.bye=false;

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
      tempMNInfo.forwardIdent=myIdent;
      tempMNInfo.srcIdent=myIdent;
      tempMNInfo.eventId=-1;
      tempMNInfo.clusterMaster=true;
      tempMNInfo.reachable=true;
      tempMNInfo.keepAlive=false;
      tempMNInfo.linkFlag=false;
      tempMNInfo.hello=false;
      tempMNInfo.ACK=false;
      tempMNInfo.bye=false;

      int tempSock=GetSockByIdent(tempClusterMasterInfo.masterIdent);
      for (int i=0;i<clusterMasterInfo.size();i++)
      {
        tempMNInfo.addr=clusterMasterInfo[i].masterAddr;
        tempMNInfo.pathNodeIdent[0]=clusterMasterInfo[i].masterIdent;// 
        tempMNInfo.pathNodeIdent[1].level=clusterMasterInfo[i].inDirNodeNum;// pathNodeIdentB有效，值为indirnodenum
        tempMNInfo.pathNodeIdent[1].position=clusterMasterInfo[i].inDirNodeNum;
        for (int j=2;j<MAX_PATH_LEN;j++)
        {
          tempMNInfo.pathNodeIdent[j].level=-1;
          tempMNInfo.pathNodeIdent[j].position=-1;
        }
        tempMNInfo.chiefMaster=clusterMasterInfo[i].chiefMaster;
        m_tcpRoute->SendMessageTo(tempSock,tempMNInfo);
      }
      // 最后就是chief自己的indirnodenum
      tempMNInfo.addr=*(GetAddrByNICName(MGMT_INTERFACE));
      tempMNInfo.pathNodeIdent[0]=myIdent;// 
      tempMNInfo.pathNodeIdent[1].level=inDirNodeNum;// pathNodeIdentB有效，值为indirnodenum
      tempMNInfo.pathNodeIdent[1].position=inDirNodeNum;
      for (int i=2;i<MAX_PATH_LEN;i++)
      {
        tempMNInfo.pathNodeIdent[i].level=-1;
        tempMNInfo.pathNodeIdent[i].position=-1;
      }
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
Ipv4GlobalRouting::IsUnreachableInDirNode(ident tempNode,vector<ident> effectInDirNode,struct MNinfo tempMNInfo)// 判断Node是否为不可达的间接node
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
          // // 某个Node不可达，就记录那些需要发送给他的信息，等连接成功后再发送
          // // 但是应该对这些信息进行甄别，当node重连上后，只需要把当前的网络状态发给它即可，而不是将失联期间的整个历史纪录都下发
          // struct accumulationMNinfo *currentAccumulationMNInfo=masterInDirPathTable[i].headAccumulationMNInfo;
          // // 应该在此处修改
          // while (currentAccumulationMNInfo->next!=NULL) currentAccumulationMNInfo=currentAccumulationMNInfo->next;
          // struct accumulationMNinfo *newAccumulationMNInfo=(struct accumulationMNinfo *)malloc(sizeof(struct accumulationMNinfo));
          // newAccumulationMNInfo->tempMNInfo=new MNinfo();
          // *(newAccumulationMNInfo->tempMNInfo)=tempMNInfo;
          // newAccumulationMNInfo->next=NULL;
          // currentAccumulationMNInfo->next=newAccumulationMNInfo;
          Logfout.close();
          return true;
        }
      }
    }
  }
  Logfout.close();
  return false;
}

void 
Ipv4GlobalRouting::AssistSendTo(struct MNinfo tempMNInfo)// 通过udp来协助下发
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  srand((unsigned)time(NULL));
  int randIndex=0;

  struct sockaddr_in localAddr;
  for (int i=0;i<masterAddressSet.size();i++)
  {
    if (SameNode(masterAddressSet[i].masterIdent,myIdent))
    {
      randIndex=(rand())%(masterAddressSet[i].masterAddress.size()-1)+1;
      memset(&localAddr,0,sizeof(localAddr)); //数据初始化--清零
      localAddr.sin_family=AF_INET; //设置为IP通信
      localAddr.sin_addr.s_addr=inet_addr((masterAddressSet[i].masterAddress[randIndex]).c_str());
      localAddr.sin_port=htons(0);
      break;
    }
  }
  // fprintf(stderr, "AssistSendTo Pick a NIC!\n");

  for (int i=0,j=0;i<MAX_TF_NODE_NUM && j<nodeMapToSock.size();j++)// 最多尝试nodeMapToSock.size()次
  {
    randIndex=(rand())%(nodeMapToSock.size());
    if (nodeMapToSock[randIndex].nodeIdent.level!=0 && nodeMapToSock[randIndex].direct==true)
    {
      tempMNInfo.forwardIdent=nodeMapToSock[randIndex].nodeIdent;
      // fprintf(stderr, "AssistSendTo try send to %d.%d through %d.%d!\n",tempMNInfo.destIdent.level,tempMNInfo.destIdent.position,tempMNInfo.forwardIdent.level,tempMNInfo.forwardIdent.position);
      m_udpClient.SendLinkInfo(localAddr,nodeMapToSock[randIndex].nodeAddress,tempMNInfo);
      // fprintf(stderr, "AssistSendTo succeed send to %d.%d through %d.%d!\n",tempMNInfo.destIdent.level,tempMNInfo.destIdent.position,tempMNInfo.forwardIdent.level,tempMNInfo.forwardIdent.position);
      // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
      // if (tempMNInfo.linkFlag==true) Logfout << " up";
      // else Logfout << " down";
      // Logfout << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << " by udp [forwardNode:" << nodeMapToSock[randIndex].nodeIdent.level << "." << nodeMapToSock[randIndex].nodeIdent.position << "]." << endl;
      i++;
    }
  }
  // Logfout.close();
}

void // 求出tempNode内node的上行或者下行链路的另一端结点的ident，存放于tempEffectNode中
Ipv4GlobalRouting::GetEffectNode(vector<ident> effectInDirNode,vector<ident> tempNode,ident srcIdent,string type,vector<ident> *tempEffectNode,struct MNinfo tempMNInfo)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 求tempNode中Node的下一跳
  for (int i=0;i<tempNode.size();i++)
  {
    // fprintf(stderr, "GetEffectNode Calculate %d.%d\n", tempNode[i].level, tempNode[i].position);
    struct linktableentry *nextLinkTableEntry=headMasterLinkTableEntry;
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

  for (int i=0;i<tempNode.size();i++)// 先给一部分Node下发
  {
    if (type=="DOWN" || tempEffectNode->size()==0)//
    {
      // fprintf(stderr, "GetEffectNode try Send %d.%d\n", tempNode[i].level, tempNode[i].position);
      tempMNInfo.destIdent=tempNode[i];
      if (SameNode(tempMNInfo.destIdent,srcIdent));// 此Node不需要通知
      else if (!IsUnreachableInDirNode(tempMNInfo.destIdent,effectInDirNode,tempMNInfo))// 不是受影响的node，直接发送
      {
        // fprintf(stderr, "GetEffectNode real Send %d.%d\n", tempNode[i].level, tempNode[i].position);
        UpdateResponseRecord(tempMNInfo.eventId,tempMNInfo.pathNodeIdent[0],tempMNInfo.pathNodeIdent[1],1);
        AssistSendTo(tempMNInfo);
        m_tcpRoute->SendMessageTo(GetSockByIdent(tempNode[i]),tempMNInfo);
        // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position << " ";
        // if (tempMNInfo.linkFlag==true) Logfout << "up";
        // else Logfout << "down";
        // Logfout << " to node " << tempNode[i].level << "." << tempNode[i].position << " by tcp." << endl;
        // fprintf(stderr, "GetEffectNode succeed Send %d.%d\n", tempNode[i].level, tempNode[i].position);
      }
      else 
      {
        // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position << " ";
        // if (tempMNInfo.linkFlag==true) Logfout << "up";
        // else Logfout << "down";
        // Logfout << " to node " << tempNode[i].level << "." << tempNode[i].position << " later." << endl;      
      }
    }
  }

  if (tempEffectNode->size()==0) return;// 不存在相关的下一跳节点了，直接退出

  tempNode.clear();// 清空
  // 继续求下一跳
  for (int i=0;i<tempEffectNode->size();i++) tempNode.push_back((*tempEffectNode)[i]);
  
  if (type=="UP" && tempNode[0].level!=3)// 还未到Spine
  {
    tempEffectNode->clear();
    GetEffectNode(effectInDirNode,tempNode,srcIdent,"UP",tempEffectNode,tempMNInfo);
  }
  else if (type=="UP" && tempNode[0].level==3)// 到SpineNode后可以开始求下行
  {
    tempEffectNode->clear();
    GetEffectNode(effectInDirNode,tempNode,srcIdent,"DOWN",tempEffectNode,tempMNInfo);
  }
  else if (type=="DOWN" && tempNode[0].level!=1)// 还未到最底层，继续求下行
  {
    tempEffectNode->clear();
    GetEffectNode(effectInDirNode,tempNode,srcIdent,"DOWN",tempEffectNode,tempMNInfo);
  }
  else if (type=="DOWN" && tempNode[0].level==1)// 此处需要考虑，如果是100个leaf，求10000个tor，
  {
    for (int i=0;i<tempEffectNode->size();i++)
    {
      // fprintf(stderr, "GetEffectNode try Send %d.%d\n", (*tempEffectNode)[i].level, (*tempEffectNode)[i].position);
      tempMNInfo.destIdent=(*tempEffectNode)[i];
      if (SameNode(tempMNInfo.destIdent,srcIdent));// 此Node不需要通知
      else if (!IsUnreachableInDirNode(tempMNInfo.destIdent,effectInDirNode,tempMNInfo))// 不是受影响的node，直接发送
      {
        // fprintf(stderr, "GetEffectNode real Send %d.%d\n", (*tempEffectNode)[i].level, (*tempEffectNode)[i].position);
        UpdateResponseRecord(tempMNInfo.eventId,tempMNInfo.pathNodeIdent[0],tempMNInfo.pathNodeIdent[1],1);
        // fprintf(stderr, "GetEffectNode succeed UpdateResponseRecord Send %d.%d\n", (*tempEffectNode)[i].level, (*tempEffectNode)[i].position);
        AssistSendTo(tempMNInfo);
        // fprintf(stderr, "GetEffectNode succeed AssistSendTo Send %d.%d\n", (*tempEffectNode)[i].level, (*tempEffectNode)[i].position);
        m_tcpRoute->SendMessageTo(GetSockByIdent((*tempEffectNode)[i]),tempMNInfo);
        // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position << " ";
        // if (tempMNInfo.linkFlag==true) Logfout << "up";
        // else Logfout << "down";
        // Logfout << " to node " << (*tempEffectNode)[i].level << "." << (*tempEffectNode)[i].position << " by tcp." << endl;
        // fprintf(stderr, "GetEffectNode succeed Send %d.%d\n", (*tempEffectNode)[i].level, (*tempEffectNode)[i].position);
      }
      else 
      {
        // Logfout << GetNow() << "Send " << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position << " ";
        // if (tempMNInfo.linkFlag==true) Logfout << "up";
        // else Logfout << "down";
        // Logfout << " to node " << (*tempEffectNode)[i].level << "." << (*tempEffectNode)[i].position << " later." << endl;
      }
    }
    tempEffectNode->clear();
    return;
  }
  // fprintf(stderr, "GetEffectNode ERROR!\n");
  // Logfout.close();
}

void 
Ipv4GlobalRouting::ChooseNodeToInformInDirNode(ident destIdent,ident lastNode,struct MNinfo tempMNInfo)
{
  // pthread_mutex_lock(&mutexA);
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (GetSockByIdent(destIdent)>0) 
  {
    Logfout << GetNow() << "Node " << destIdent.level << "." << destIdent.position << " has connected with chiefMaster "
    << myIdent.level << "." << myIdent.position << endl;
  }
  else
  {
    // 随机选几个直连的Node尝试通知该间接Node重新和master建立连接
    // srand((int)time(NULL));
    int randIndex=0;
    while (1)
    {
      randIndex=((int)GetSystemTime())%(nodeMapToSock.size());
      // Logfout << "randIndex:" << randIndex << endl;
      if (!SameNode(lastNode,nodeMapToSock[randIndex].nodeIdent) && nodeMapToSock[randIndex].nodeIdent.level!=0 && nodeMapToSock[randIndex].direct==true)
      {
        tempMNInfo.destIdent=destIdent;
        tempMNInfo.forwardIdent=nodeMapToSock[randIndex].nodeIdent;
        Logfout << GetNow() << "Choose node " << nodeMapToSock[randIndex].nodeIdent.level << "." << nodeMapToSock[randIndex].nodeIdent.position;
        m_tcpRoute->SendMessageTo(nodeMapToSock[randIndex].nodeSock,tempMNInfo);
        Logfout << " to inform node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
        break;
      }
    }
  }
  Logfout.close();
  // pthread_mutex_unlock(&mutexA);
}

void
Ipv4GlobalRouting::SendMessageToNode(ident high,ident low,ident srcIdent,int eventId,bool linkFlag)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 确定影响范围，这代码写得太垃圾了
  struct MNinfo *tempMNInfo=(struct MNinfo *)malloc(sizeof(struct MNinfo));
  tempMNInfo->addr.sin_family=AF_INET;// addr此时无实际意义
  inet_aton("255.255.255.255",&(tempMNInfo->addr.sin_addr));
  tempMNInfo->addr.sin_port=htons(0);
  tempMNInfo->destIdent=high;// 不可省略，未初始化不能使用
  tempMNInfo->forwardIdent=high;
  tempMNInfo->srcIdent=myIdent;
  tempMNInfo->pathNodeIdent[0]=high;
  tempMNInfo->pathNodeIdent[1]=low;
  for (int i=2;i<MAX_PATH_LEN;i++)
  {
    tempMNInfo->pathNodeIdent[i].level=-1;
    tempMNInfo->pathNodeIdent[i].position=-1;
  }
  tempMNInfo->eventId=eventId;
  tempMNInfo->clusterMaster=false;
  tempMNInfo->chiefMaster=false;
  tempMNInfo->reachable=true;
  tempMNInfo->keepAlive=false;
  tempMNInfo->linkFlag=linkFlag;
  tempMNInfo->hello=false;
  tempMNInfo->ACK=false;
  tempMNInfo->bye=false;

  // Logfout << GetNow() << "Master start to send message:" << endl;

  for (int i=0;i<effectNodeRange.size();i++)
  {
    if (SameNode(effectNodeRange[i].high,high) && SameNode(effectNodeRange[i].low,low))// 找到影响范围
    {
      for (int j=0;j<MAX_EFFECT_NODE;j++)
      {
        if (effectNodeRange[i].effectNode[j].level!=-1)
        {
          if(linkFlag==true || (linkFlag==false && !SameNode(effectNodeRange[i].effectNode[j],srcIdent)))
          {
            tempMNInfo->destIdent=effectNodeRange[i].effectNode[j];
            UpdateResponseRecord(tempMNInfo->eventId,high,low,1);
            AssistSendTo(*tempMNInfo);
            m_tcpRoute->SendMessageTo(GetSockByIdent(tempMNInfo->destIdent),*tempMNInfo);
         
            // Logfout << GetNow() << "Send " << tempMNInfo->pathNodeIdent[0].level << "." << tempMNInfo->pathNodeIdent[0].position << "--" << tempMNInfo->pathNodeIdent[1].level << "." << tempMNInfo->pathNodeIdent[1].position << " ";
            // if (tempMNInfo->linkFlag==true) Logfout << "up";
            // else Logfout << "down";
            // Logfout << " to node " << effectNodeRange[i].effectNode[j].level << "." << effectNodeRange[i].effectNode[j].position << " by TCP & UDP." << endl;
          }
        }
      }
      break;
    }
  }

  

  // vector<ident> tempNode,tempEffectNode;
  // tempNode.push_back(high);

  // if (high.level==1) GetEffectNode(effectInDirNode,tempNode,srcIdent,"UP",&tempEffectNode,*tempMNInfo);
  // else if (high.level==2) GetEffectNode(effectInDirNode,tempNode,srcIdent,"UP",&tempEffectNode,*tempMNInfo);
  // else if (high.level==3) GetEffectNode(effectInDirNode,tempNode,srcIdent,"DOWN",&tempEffectNode,*tempMNInfo);

  // for (int i=0;i<effectInDirNode.size();i++) 
  // {
  //   ChooseNodeToInformInDirNode(effectInDirNode[i],lastNode,*tempMNInfo);
  // }
}

void
Ipv4GlobalRouting::SendMessageToNode1(ident high,ident low,ident srcIdent,int eventId,bool linkFlag)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 确定影响范围，这代码写得太垃圾了
  struct MNinfo *tempMNInfo=(struct MNinfo *)malloc(sizeof(struct MNinfo));
  tempMNInfo->addr.sin_family=AF_INET;// addr此时无实际意义
  inet_aton("255.255.255.255",&(tempMNInfo->addr.sin_addr));
  tempMNInfo->addr.sin_port=htons(0);
  tempMNInfo->destIdent=high;// 不可省略，未初始化不能使用
  tempMNInfo->forwardIdent=high;
  tempMNInfo->srcIdent=myIdent;
  tempMNInfo->pathNodeIdent[0]=high;
  tempMNInfo->pathNodeIdent[1]=low;
  for (int i=2;i<MAX_PATH_LEN;i++)
  {
    tempMNInfo->pathNodeIdent[i].level=-1;
    tempMNInfo->pathNodeIdent[i].position=-1;
  }
  tempMNInfo->eventId=eventId;
  tempMNInfo->clusterMaster=false;
  tempMNInfo->chiefMaster=false;
  tempMNInfo->reachable=true;
  tempMNInfo->keepAlive=false;
  tempMNInfo->linkFlag=linkFlag;
  tempMNInfo->hello=false;
  tempMNInfo->ACK=false;
  tempMNInfo->bye=false;

  // 某些间接连接的Node可能会因为这次链路变化而导致master的信息无法传达，所以需要记录
  vector<ident> effectInDirNode;
  ident lastNode;
  lastNode.level=-1;
  lastNode.position=-1;

  if (linkFlag==false)// 链路故障，避免将信息发给发送信息的源节点
  {
    if (low.level==0)// 直连链路故障
    {
      for (int i=0;i<masterInDirPathTable.size();i++)
      {
        if (SameNode(high,masterInDirPathTable[i].pathNodeIdent[0])) continue;// 不需要通知，直连失联的Node会自动重连
        for (int j=MAX_PATH_LEN-1;j>=0;j--)
        {
          if (masterInDirPathTable[i].pathNodeIdent[j].level!=-1) // 间接路径是通过该masterInDirPathTable[i].pathNodeIdent[j]的直接连接与Master通信
          {
            if (SameNode(high,masterInDirPathTable[i].pathNodeIdent[j]))
            {
              effectInDirNode.push_back(masterInDirPathTable[i].pathNodeIdent[0]);
            }
            break;
          }
        }
      }
    }
    else// 拓扑中的某条链路故障
    {
      for (int i=0;i<masterInDirPathTable.size();i++)
      {
        for (int j=0;j<MAX_PATH_LEN-1;j++)
        {
          if ((SameNode(high,masterInDirPathTable[i].pathNodeIdent[j]) && SameNode(low,masterInDirPathTable[i].pathNodeIdent[j+1])) || (SameNode(low,masterInDirPathTable[i].pathNodeIdent[j]) && SameNode(high,masterInDirPathTable[i].pathNodeIdent[j+1])))
          {
            effectInDirNode.push_back(masterInDirPathTable[i].pathNodeIdent[0]);
            break;
          }
        }
      }
    }
  }
  else // 链路恢复
  {
    if (low.level!=0)// 数据平面链路恢复，需要将信息下发给上报结点
    {
      srcIdent.level=-1;
      srcIdent.position=-1;
    }
  }

  // Logfout << GetNow() << "Master start to send message:" << endl;

  vector<ident> tempNode,tempEffectNode;
  tempNode.push_back(high);

  if (high.level==1) GetEffectNode(effectInDirNode,tempNode,srcIdent,"UP",&tempEffectNode,*tempMNInfo);
  else if (high.level==2) GetEffectNode(effectInDirNode,tempNode,srcIdent,"UP",&tempEffectNode,*tempMNInfo);
  else if (high.level==3) GetEffectNode(effectInDirNode,tempNode,srcIdent,"DOWN",&tempEffectNode,*tempMNInfo);

  for (int i=0;i<effectInDirNode.size();i++) 
  {
    ChooseNodeToInformInDirNode(effectInDirNode[i],lastNode,*tempMNInfo);
  }
}

void
Ipv4GlobalRouting::PrintLinkEffectRange()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/LinkEffectRange-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Link\tEffectNode" << endl;
  for (int i=0;i<effectNodeRange.size();i++)
  {
    Logfout << effectNodeRange[i].high.level << "." << effectNodeRange[i].high.position << "--" << effectNodeRange[i].low.level << "." << effectNodeRange[i].low.position << "\t";
    for (int j=0;j<MAX_EFFECT_NODE;j++)
    {
      if (effectNodeRange[i].effectNode[j].level==-1) break;
      Logfout << effectNodeRange[i].effectNode[j].level << "." << effectNodeRange[i].effectNode[j].position << "\t";
    }
    Logfout << endl;
  }

  Logfout.close();
}

// void
// Ipv4GlobalRouting::GetLinkEffectNode(ident* effectNode,vector<ident> tempNode,string type)
// {
//   vector<ident> tempEffectNode;
//   tempEffectNode.clear();
//   for (int i=0;i<tempNode.size();i++)
//   {
//     struct linktableentry *nextLinkTableEntry=headMasterLinkTableEntry;
//     if (type=="UP")// 求上行链路
//     {
//       do
//       {
//         bool isFindSameNode=false;
//         nextLinkTableEntry=nextLinkTableEntry->next;
//         if (nextLinkTableEntry!=NULL)// 终止条件还可以进行判断
//         {
//           if (SameNode(tempNode[i],nextLinkTableEntry->low)) 
//           {
//             for (int j=0;j<MAX_EFFECT_NODE;j++)
//             {
//               if (SameNode(effectNode[j],nextLinkTableEntry->high))// 存在相同的则不再添加，这些地方都必须优化
//               {
//                 isFindSameNode=true;
//                 break;
//               }
//             }
//             if (!isFindSameNode) 
//             {
//               tempEffectNode.push_back(nextLinkTableEntry->high);
//             }
//           }
//         }
//         else break;
//       }while(nextLinkTableEntry!=NULL);
//     }
//     else if (type=="DOWN")// 求下行链路
//     {
//       do
//       {
//         bool isFindSameNode=false;
//         nextLinkTableEntry=nextLinkTableEntry->next;
//         if (nextLinkTableEntry!=NULL)// 终止条件还可以进行判断
//         {
//           if (SameNode(tempNode[i],nextLinkTableEntry->high)) 
//           {
//             for (int j=0;j<MAX_EFFECT_NODE;j++)
//             {
//               if (SameNode(effectNode[j],nextLinkTableEntry->low))// 存在相同的则不再添加，这些地方都必须优化
//               {
//                 isFindSameNode=true;
//                 break;
//               }
//             }
//             if (!isFindSameNode) 
//             {
//               tempEffectNode.push_back(nextLinkTableEntry->low);
//             }
//           }
//         }
//       }while(nextLinkTableEntry!=NULL);
//     }
//   }
  

//   if (type=="DOWN" || tempEffectNode.size()==0)//
//   {
//     for (int i=0;i<tempNode.size();i++)// 先给一部分Node下发
//     {
//       for (int j=0;j<MAX_EFFECT_NODE;j++)
//       {
//         if (!SameNode(effectNode[j],tempNode[i]) && effectNode[j].level==-1) effectNode[j]=tempNode[i];
//       }
//     }
//   }

//   if (tempEffectNode.size()==0) return;// 不存在相关的下一跳节点了，直接退出

//   tempNode.clear();
//   // 继续求下一跳
//   for (int i=0;i<tempEffectNode.size();i++) tempNode.push_back(tempEffectNode[i]);

//   tempEffectNode.clear();

//   if (type=="UP" && tempNode[0].level!=3)// 还未到Spine
//   {
//     GetLinkEffectNode(effectNode,tempNode,"UP");
//   }
//   else if (type=="UP" && tempNode[0].level==3)// 到SpineNode后可以开始求下行
//   {
//     GetLinkEffectNode(effectNode,tempNode,"DOWN");
//   }
//   else if (type=="DOWN" && tempNode[0].level!=1)// 还未到最底层，继续求下行
//   {
//     GetLinkEffectNode(effectNode,tempNode,"DOWN");
//   }
//   else if (type=="DOWN" && tempNode[0].level==1)// 此处需要考虑，如果是100个leaf，求10000个tor，
//   {
//     for (int i=0;i<tempEffectNode.size();i++)// 先给一部分Node下发
//     {
//       for (int j=0;j<MAX_EFFECT_NODE;j++)
//       {
//         if (!SameNode(effectNode[j],tempNode[i]) && effectNode[j].level==-1) effectNode[j]=tempEffectNode[i];
//       }
//     }
//     return;
//   }
//   // return;
// }

void
Ipv4GlobalRouting::AddEffectRange(ident high,ident low,vector<ident> tempNode)
{
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/LinkEffectRange-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // Logfout << "AddEffectRange "<< high.level <<"."<< high.position << "-->" << low.level <<"."<< low.position<< endl;
  // Logfout.close();
  // PrintLinkEffectRange();
  for (int i=0;i<effectNodeRange.size();i++)
  {
    if (SameNode(effectNodeRange[i].high,high) && SameNode(effectNodeRange[i].low,low))
    {
      for (int k=0;k<tempNode.size();k++)
      {
        for (int j=0;j<MAX_EFFECT_NODE;j++)
        {
          if (!SameNode(effectNodeRange[i].effectNode[j],tempNode[k]) && effectNodeRange[i].effectNode[j].level==-1) 
          {
            effectNodeRange[i].effectNode[j]=tempNode[k];
            break;
          }
        }
      }
    }
  }

  // Logfout.open(logFoutPath.str().c_str(),ios::app);
  // Logfout << "AddEffectRange done" << endl;
  // Logfout.close();
  // PrintLinkEffectRange();
}

void
Ipv4GlobalRouting::GetLinkEffectNode(ident high,ident low,vector<ident> tempNode,vector<ident> *tempEffectNode,string type)
{
  for (int i=0;i<tempNode.size();i++)
  {
    // fprintf(stderr, "GetEffectNode Calculate %d.%d\n", tempNode[i].level, tempNode[i].position);
    struct linktableentry *nextLinkTableEntry=headMasterLinkTableEntry;
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

  // for (int i=0;i<tempNode.size();i++)// 先给一部分Node下发
  // {
  //   if (type=="DOWN" || tempEffectNode->size()==0)//
  //   {
  //     AddEffectRange(high,low,tempNode);
  //   }
  // }
  if (type=="DOWN" || tempEffectNode->size()==0) AddEffectRange(high,low,tempNode);

  if (tempEffectNode->size()==0) return;// 不存在相关的下一跳节点了，直接退出

  tempNode.clear();// 清空
  // 继续求下一跳
  for (int i=0;i<tempEffectNode->size();i++) tempNode.push_back((*tempEffectNode)[i]);
  
  if (type=="UP" && tempNode[0].level!=3)// 还未到Spine
  {
    tempEffectNode->clear();
    GetLinkEffectNode(high,low,tempNode,tempEffectNode,"UP");
  }
  else if (type=="UP" && tempNode[0].level==3)// 到SpineNode后可以开始求下行
  {
    tempEffectNode->clear();
    GetLinkEffectNode(high,low,tempNode,tempEffectNode,"DOWN");
  }
  else if (type=="DOWN" && tempNode[0].level!=1)// 还未到最底层，继续求下行
  {
    tempEffectNode->clear();
    GetLinkEffectNode(high,low,tempNode,tempEffectNode,"DOWN");
  }
  else if (type=="DOWN" && tempNode[0].level==1)// 此处需要考虑，如果是100个leaf，求10000个tor，
  {
    // for (int i=0;i<tempEffectNode->size();i++)
    // {
    //   AddEffectRange(high,low,(*tempEffectNode));
    // }
    AddEffectRange(high,low,(*tempEffectNode));
    tempEffectNode->clear();
    return;
  }
}

void
Ipv4GlobalRouting::GenerateLinkEffectRange()
{
  // pthread_mutex_lock(&mutexA);
  // printf("GenerateLinkEffectRange\n");
  ident tempIdent;
  tempIdent.level=-1;
  tempIdent.position=-1;

  for (int i=0;i<effectNodeRange.size();i++)
  {
    for (int j=0;j<MAX_EFFECT_NODE;j++)
    {
      effectNodeRange[i].effectNode[j]=tempIdent;//全部清空
    }
    // 重新计算影响范围
    vector<ident> tempNode,tempEffectNode;
    tempNode.clear();
    tempNode.push_back(effectNodeRange[i].high);

    if (effectNodeRange[i].high.level==1) {
      fprintf(stderr,"ERROR: should not hit level==1!!!!\n");
      GetLinkEffectNode(effectNodeRange[i].high,effectNodeRange[i].low,tempNode,&tempEffectNode,"UP");
    }
    else if (effectNodeRange[i].high.level==2) GetLinkEffectNode(effectNodeRange[i].high,effectNodeRange[i].low,tempNode,&tempEffectNode,"UP");
    else if (effectNodeRange[i].high.level==3) GetLinkEffectNode(effectNodeRange[i].high,effectNodeRange[i].low,tempNode,&tempEffectNode,"DOWN");
    else
      fprintf(stderr,"effectNodeRange[%d].high.level=%d\n",i,effectNodeRange[i].high.level);
  }
  PrintLinkEffectRange();
  // pthread_mutex_unlock(&mutexA);
}

/**************************Master**************************/