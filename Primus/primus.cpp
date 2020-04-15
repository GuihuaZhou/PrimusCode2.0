#include "primus.h"

Primus::Primus(
    int level,
    int position,
    int toRNodes,
    int leafNodes,
    int spineNodes,
    int nPods,
    int defaultLinkTimer,
    int defaultKeepaliveTimer)
{
    m_Ident.level=level; 
    m_Ident.position=position;
    if (m_Ident.level!=0) m_Role=1;
    else if (m_Ident.position==0) m_Role=2;//master
    else m_Role=3;//slave
    m_ToRNodes=toRNodes;
    m_LeafNodes=leafNodes;
    m_SpineNodes=spineNodes;
    m_nPods=nPods;
    if (m_Ident.level==3) m_Pod=-1;// spinenode 不属于任何pod
    else if (m_Ident.level==2) m_Pod=m_Ident.position/m_LeafNodes;
    else if (m_Ident.level==1) m_Pod=m_Ident.position/m_ToRNodes;
    m_DefaultLinkTimer=defaultLinkTimer*1000;// master链路定时器，配置文件写成了ms，所以要乘以1000，换算成us
    m_DefaultKeepaliveTimer=defaultKeepaliveTimer;

    affectedNodeNumA=1+m_nPods*(m_ToRNodes+1);
    affectedNodeNumB=(m_SpineNodes/m_LeafNodes)+m_nPods*(m_ToRNodes+1);

    m_EpollFd=-1;

    tempIdent.level=-1;
    tempIdent.position=-1;

    tempAddr.sin_family=AF_INET;
    tempAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    tempAddr.sin_port=htons(0);
}

Primus::~Primus()
{
    // 
}

int
Primus::rta_addattr_l(struct rtattr *rta, size_t maxlen, int type, void *data, size_t alen)
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
Primus::addattr_l(struct nlmsghdr *n, size_t maxlen, int type, void *data, size_t alen)
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
Primus::addattr32 (struct nlmsghdr *n, size_t maxlen, int type, int data)
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
Primus::OpenNetlink()
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
Primus::GetNow()
{
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime(&tt);
  stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

unsigned long 
Primus::Convert(struct sockaddr_in dstAddr,unsigned int prefixLen)
{
  unsigned long netmask=0;
  unsigned long netaddress=0;
  netmask = (~0) << (32-prefixLen);
  netmask = htonl(netmask);
  netaddress = dstAddr.sin_addr.s_addr & netmask;
  
  return netaddress;
}

void
Primus::AddSingleRoute(struct sockaddr_in dstAddr,unsigned int prefixLen,struct nexthopandweight tempNextHopAndWeight)
{  
  // cout << "AddSingleRoute:" << inet_ntoa(dstAddr.sin_addr) << "/" << prefixLen << ",NICName:" << tempNextHopAndWeight.NICName;
  // // // cout << ",srcAddr:" << inet_ntoa(tempNextHopAndWeight.srcAddr.sin_addr);
  // // // cout << ",gateAddr:" << inet_ntoa(tempNextHopAndWeight.gateAddr.sin_addr);
  // cout << endl;

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
  if (gateAddr==inet_ntoa(dstAddr.sin_addr)) req.r.rtm_scope=RT_SCOPE_LINK;//
  else req.r.rtm_scope=RT_SCOPE_UNIVERSE;//
  req.r.rtm_type=RTN_UNICAST;//
  req.r.rtm_dst_len=prefixLen;//
  
  // mtu没设置
  int bytelen=(req.r.rtm_family==AF_INET)?4:16;

  dstAddr.sin_addr.s_addr=Convert(dstAddr,prefixLen);
  addattr_l(&req.n,sizeof(req),RTA_DST,&(dstAddr.sin_addr.s_addr),bytelen);//目的地址
  addattr32(&req.n,sizeof(req),RTA_PRIORITY,NL_DEFAULT_ROUTE_METRIC);//metric
  addattr_l(&req.n,sizeof(req),RTA_GATEWAY,&(tempNextHopAndWeight.gateAddr.sin_addr.s_addr),bytelen);//网关，单路径好像不要加上网关
  addattr_l(&req.n,sizeof(req),RTA_PREFSRC,&(tempNextHopAndWeight.srcAddr.sin_addr.s_addr),bytelen);//src
  addattr_l(&req.n,sizeof(req),RTA_OIF,&if_index,bytelen);//该路由项的输出网络设备索引
     
  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  close(rt_sock);
}

void
Primus::AddMultiRoute(struct sockaddr_in dstAddr,unsigned int prefixLen,vector<struct nexthopandweight> nextHopAndWeight)
{    
  // cout << "AddMultiRoute:" << inet_ntoa(dstAddr.sin_addr) << "/" << prefixLen << ",";
  // for (int i=0;i<nextHopAndWeight.size();i++)
  // {
  //   cout << endl << "\t\t\tNICName:" << nextHopAndWeight[i].NICName << " weight:" << nextHopAndWeight[i].weight;
  //   // cout << " srcAddr:" << inet_ntoa(nextHopAndWeight[i].srcAddr.sin_addr);
  //   // cout << " gateAddr:" << inet_ntoa(nextHopAndWeight[i].gateAddr.sin_addr);
  // }
  // cout << endl;

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
  dstAddr.sin_addr.s_addr=Convert(dstAddr,prefixLen);

  int bytelen=(req.r.rtm_family==AF_INET)?4:16;
  addattr_l(&req.n,sizeof(req),RTA_DST,&(dstAddr.sin_addr.s_addr),bytelen);
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
}

void 
Primus::DelRoute(struct sockaddr_in dstAddr,unsigned int prefixLen)
{
  // cout << "DelRoute " << inet_ntoa(dstAddr.sin_addr) << "/" << prefixLen << endl;
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

  dstAddr.sin_addr.s_addr=Convert(dstAddr,prefixLen);
  addattr_l(&req.n,sizeof(req),RTA_DST,&(dstAddr.sin_addr.s_addr),4);
  
  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  close(rt_sock);
}

void 
Primus::PrintMessage(struct message tempMessage)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "PrimusLog-" << m_Ident.level << "." << m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  Logfout << GetNow();
  if (SameNode(tempMessage.srcIdent,m_Ident)) Logfout << "Send";
  else if (SameNode(tempMessage.dstIdent,m_Ident) || ((m_Role==2 || m_Role==3)) && (SameNode(tempMessage.dstIdent,tempIdent))) Logfout << "Recv";
  else Logfout << "Forw";

  Logfout << " message[" ;
  if (tempMessage.transportType==1) Logfout << "TCP][";
  else Logfout << "UDP][";
  if (SameNode(tempMessage.fowIdent,tempIdent)) Logfout << "Direct][";
  else Logfout << "InDirect][";
  if (tempMessage.messageType==1) Logfout << "HL";
  else if (tempMessage.messageType==2) Logfout << "LS";
  else if (tempMessage.messageType==3) Logfout << "KA";
  else if (tempMessage.messageType==4) Logfout << "RE";

  if (tempMessage.ack==true) Logfout << ":RS";
  else Logfout << ":RP";
  Logfout << ":" << tempMessage.linkInfo.eventId << "][src:" << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position;
  if (!SameNode(tempMessage.fowIdent,tempIdent)) Logfout << ",fow:" << tempMessage.fowIdent.level << "." << tempMessage.fowIdent.position;
  Logfout << ",dst:" << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << "]";

  if (tempMessage.messageType==2)
  {
    Logfout << "[" << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
    << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position;
    if (tempMessage.linkInfo.linkStatus==true) Logfout << "/UP]";
    else Logfout << "/DOWN]";
  }
  Logfout << "." << endl;

  Logfout.close();
}

void 
Primus::PrintLinkTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "LinkTable-" << m_Ident.level << "." << m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Link\t\tLinkFlag\tIsStartTimer\tEventId\t\tAddr" << endl;

  for (int i=0;i<linkNum;i++)
  {
    Logfout << linkTable[i].linkInfo.identA.level << "." << linkTable[i].linkInfo.identA.position 
    << "--" << linkTable[i].linkInfo.identB.level << "." << linkTable[i].linkInfo.identB.position
    << "\t" << linkTable[i].linkInfo.linkStatus
    << "\t\t" << linkTable[i].isStartTimer 
    << "\t\t" << linkTable[i].linkInfo.eventId
    << "\t\t" << inet_ntoa(linkTable[i].linkInfo.addrA.sin_addr);
    Logfout << "\t" << inet_ntoa(linkTable[i].linkInfo.addrB.sin_addr)
    << endl;
  }
  Logfout.close();
}

void 
Primus::PrintNeighborTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "NeighborTable-" << m_Ident.level << "." << m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "NeighborIdent\tLocalNICName\tSrcAddr\tNeighborAddr\tNDType\tNICFlag" << endl;

  for (int i=0;i<neighborTable.size();i++)
  {
    Logfout << neighborTable[i].neighborIdent.level << "." << neighborTable[i].neighborIdent.position
    << "\t\t" << neighborTable[i].localNICName 
    << "\t\t" << inet_ntoa(neighborTable[i].srcAddr.sin_addr);
    Logfout << "\t\t" << inet_ntoa(neighborTable[i].neighborAddr.sin_addr)
    << "\t\t" << neighborTable[i].NDType
    << "\t\t" << neighborTable[i].NICFlag << endl;
  }
  Logfout.close();
}

void
Primus::PrintPathTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "PathTable-" << m_Ident.level << "." << m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "Path\t\t\t\t\t\tFaultLinkCounter\tDirConFlag\tWeights" << endl;

  for (int i=0;i<pathNum;i++)
  {
    for (int j=0;j<MAX_PATH_LEN;j++)
    {
      if (pathTable[i].pathNodeIdent[j].level!=-1 && pathTable[i].pathNodeIdent[j].position!=-1)
        Logfout << pathTable[i].pathNodeIdent[j].level << "." << pathTable[i].pathNodeIdent[j].position;
      if (j<MAX_PATH_LEN-1 && pathTable[i].pathNodeIdent[j+1].level!=-1 && pathTable[i].pathNodeIdent[j+1].position!=-1)
        Logfout << "--";
      else 
        Logfout << "\t";
    }
    // Logfout << pathTable[i].faultLinkCounter;
    Logfout << "\t" << pathTable[i].faultLinkCounter << "\t\t" << pathTable[i].dirConMasterFlag << "\t";
    if (pathTable[i].numOfVaildPathPerNextHop!=NULL)
    {
      int nextHopNum=0;
      if (m_Ident.level==1) nextHopNum=m_LeafNodes;
      else if (m_Ident.level==2) nextHopNum=m_SpineNodes/m_LeafNodes;
      for (int j=0;j<nextHopNum;j++)
      {
        Logfout << "[" << pathTable[i].numOfVaildPathPerNextHop[j].nextHopIdent.level << "."
        << pathTable[i].numOfVaildPathPerNextHop[j].nextHopIdent.position << ":"
        << pathTable[i].numOfVaildPathPerNextHop[j].vaildPathNum << "]";
        if (j<nextHopNum-1) Logfout << ":";
      }
    }
    for (int j=0;j<MAX_ADDR_NUM;j++)
    {
      if (pathTable[i].addrSet[j].sin_addr.s_addr!=tempAddr.sin_addr.s_addr) 
        Logfout << " " << inet_ntoa(pathTable[i].addrSet[j].sin_addr);
      else break;
    }
    Logfout << endl;
  }
  Logfout.close();
}

void
Primus::PrintNodeSockTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "NodeSockTable-" << m_Ident.level << "." << m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "NodeIdent\tNodeSock" << endl;

  for (int i=0;i<nodeSockNum;i++)
  {
    Logfout << nodeSockTable[i].nodeIdent.level << "." << nodeSockTable[i].nodeIdent.position << "\t" << nodeSockTable[i].nodeSock << endl;
  }

  Logfout.close();
}

void
Primus::PrintControllerSockTable()
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "ControllerSockTable-" << m_Ident.level << "." << m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc);

  Logfout << "ControllerIdent\tControllerSock\tControllerRole" << endl;

  for (int i=0;i<MAX_CTRL_NUM;i++)
  {
    if (SameNode(controllerSockTable[i].controllerIdent,tempIdent)) break;
    Logfout << controllerSockTable[i].controllerIdent.level << "." << controllerSockTable[i].controllerIdent.position
    << "\t" << controllerSockTable[i].controllerSock 
    << "\t" << controllerSockTable[i].controllerRole << endl;
  }

  Logfout.close();
}

bool 
Primus::AddSocketToEpoll(int sock)
{
  struct epoll_event tempEvent;
  tempEvent.events = EPOLLIN;
  tempEvent.data.fd = sock;

  if (epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, sock, &tempEvent) == -1)
  {
    perror ("epoll_ctl add failed");
    exit(1);
  }

  return true;
}

bool 
Primus::DelSocket(int sock)
{
  struct epoll_event tempEvent;
  tempEvent.events = EPOLLIN | EPOLLET;
  tempEvent.data.fd = sock;

  if (epoll_ctl(m_EpollFd, EPOLL_CTL_DEL, sock, &tempEvent) == -1)
  {
    perror ("epoll_ctl del failed");
    exit(1);
  }

  // close(sock);
  shutdown(sock,SHUT_RDWR);

  for (int i=0;i<nodeSockNum;i++)
  {
    if (nodeSockTable[i].nodeSock==sock) nodeSockTable[i].nodeSock=-1;
  }

  for (int i=0;i<MAX_CTRL_NUM;i++)
  {
    if (controllerSockTable[i].controllerSock==sock)
      controllerSockTable[i].controllerSock=-1;
  }

  return true;
}

void
Primus::InitiatePathTable()
{
  // 临时做法，没有去探测服务器地址
  sockaddr_in tempAddrA,tempAddrB;
  tempAddrA=tempAddr;
  tempAddrB=tempAddr;
  tempAddrA.sin_addr.s_addr=inet_addr("192.168.1.1");
  tempAddrB.sin_addr.s_addr=inet_addr("192.168.3.1");

  int pathTableIndex=0;
  switch (m_Ident.level)
  {
  case 1:// tor=
    {// the number of path
      // 5 hop: m_SpineNodes*(m_nPods-1)*m_ToRNodes
      // 4 hop: m_SpineNodes*(m_nPods-1)
      // 3 hop: m_LeafNodes*(m_ToRNodes-1)+m_SpineNodes
      // 2 hop: m_LeafNodes
      
      pathNum=m_LeafNodes*m_ToRNodes+m_SpineNodes*((m_nPods-1)*(m_ToRNodes+1)+1);
      pathTable=(pathtableentry *)malloc(sizeof(pathtableentry)*(pathNum));
      
      // 5 hop
      for (int i=0;i<m_nPods*m_ToRNodes;i++)// tor
      {
        if (i/m_ToRNodes==m_Pod) continue;
        else
        {
          nexthopandvaildpathnum *numOfVaildPathPerNextHop;
          numOfVaildPathPerNextHop=(nexthopandvaildpathnum *)malloc(m_LeafNodes*sizeof(nexthopandvaildpathnum));

          for (int j=m_Pod*m_LeafNodes;j<(m_Pod+1)*m_LeafNodes;j++) 
            numOfVaildPathPerNextHop[j%m_LeafNodes]={{2,j},0};//m_SpineNodes/m_LeafNodes

          for (int j=m_Pod*m_LeafNodes;j<(m_Pod+1)*m_LeafNodes;j++)// leafnode，nexthop
          {
            for (int k=(j%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);k<(j%m_LeafNodes+1)*(m_SpineNodes/m_LeafNodes);k++)// spinenode
            {
              pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
              pathTable[pathTableIndex].pathNodeIdent[1]={2,j};
              pathTable[pathTableIndex].pathNodeIdent[2]={3,k};
              pathTable[pathTableIndex].pathNodeIdent[3]={2,(i/m_ToRNodes)*m_LeafNodes+(j%m_LeafNodes)};
              pathTable[pathTableIndex].pathNodeIdent[4]={1,i};
              for (int l=5;l<MAX_PATH_LEN;l++) pathTable[pathTableIndex].pathNodeIdent[l]=tempIdent;
              for (int l=0;l<MAX_ADDR_NUM;l++) pathTable[pathTableIndex].addrSet[l]=tempAddr;
              // 临时做法
              if (i==0) pathTable[pathTableIndex].addrSet[0]=tempAddrA;
              else if (i==1) pathTable[pathTableIndex].addrSet[0]=tempAddrB;
              pathTable[pathTableIndex].faultLinkCounter=4;// 初始化时默认所有的链路都是坏的
              pathTable[pathTableIndex].dirConMasterFlag=false;
              pathTable[pathTableIndex].numOfVaildPathPerNextHop=numOfVaildPathPerNextHop;
              pathTableIndex++;
            }
          }
        }
      }
      // 4 hop
      for (int i=0;i<m_LeafNodes;i++)// nexthop
      {
        for (int j=0;j<m_nPods;j++)// pod
        {
          if (j==m_Pod) continue;
          else 
          {
            for (int k=0;k<(m_SpineNodes/m_LeafNodes);k++)// spinenode
            {
              pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
              pathTable[pathTableIndex].pathNodeIdent[1]={2,m_Pod*m_LeafNodes+i};
              pathTable[pathTableIndex].pathNodeIdent[2]={3,i*(m_SpineNodes/m_LeafNodes)+k};
              pathTable[pathTableIndex].pathNodeIdent[3]={2,j*m_LeafNodes+i};
              for (int k=4;k<MAX_PATH_LEN;k++) pathTable[pathTableIndex].pathNodeIdent[k]=tempIdent;
              for (int k=0;k<MAX_ADDR_NUM;k++) pathTable[pathTableIndex].addrSet[k]=tempAddr;
              pathTable[pathTableIndex].faultLinkCounter=3;// 初始化时默认所有的链路都是坏的
              pathTable[pathTableIndex].dirConMasterFlag=false;
              pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
              pathTableIndex++;
            }
          }
        }
      }
      // 3 hop
      // des hop is tor
      for (int i=0;i<m_ToRNodes;i++)
      {
        if ((m_Pod*m_ToRNodes+i)==m_Ident.position)// itself
          continue;
        else 
        {
          nexthopandvaildpathnum *numOfVaildPathPerNextHop;
          numOfVaildPathPerNextHop=(nexthopandvaildpathnum *)malloc(m_LeafNodes*sizeof(nexthopandvaildpathnum));

          for (int j=0;j<m_LeafNodes;j++) 
            numOfVaildPathPerNextHop[j%m_LeafNodes]={{2,m_Pod*m_LeafNodes+j},0};//1

          for (int j=0;j<m_LeafNodes;j++)
          {
            pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
            pathTable[pathTableIndex].pathNodeIdent[1]={2,m_Pod*m_LeafNodes+j};
            pathTable[pathTableIndex].pathNodeIdent[2]={1,m_Pod*m_ToRNodes+i};
            for (int k=3;k<MAX_PATH_LEN;k++) pathTable[pathTableIndex].pathNodeIdent[k]=tempIdent;
            for (int k=0;k<MAX_ADDR_NUM;k++) pathTable[pathTableIndex].addrSet[k]=tempAddr;
            pathTable[pathTableIndex].faultLinkCounter=2;// 初始化时默认所有的链路都是坏的
            pathTable[pathTableIndex].dirConMasterFlag=false;
            pathTable[pathTableIndex].numOfVaildPathPerNextHop=numOfVaildPathPerNextHop;
            pathTableIndex++;
          }
        }
      }
      // des hop is spine
      for (int i=0;i<m_LeafNodes;i++)
      {
        for (int j=i*(m_SpineNodes/m_LeafNodes);j<(i+1)*(m_SpineNodes/m_LeafNodes);j++)
        {
          pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
          pathTable[pathTableIndex].pathNodeIdent[1]={2,m_Pod*m_LeafNodes+i};
          pathTable[pathTableIndex].pathNodeIdent[2]={3,j};
          for (int k=3;k<MAX_PATH_LEN;k++) pathTable[pathTableIndex].pathNodeIdent[k]=tempIdent;
          for (int k=0;k<MAX_ADDR_NUM;k++) pathTable[pathTableIndex].addrSet[k]=tempAddr;
          pathTable[pathTableIndex].faultLinkCounter=2;// 初始化时默认所有的链路都是坏的
          pathTable[pathTableIndex].dirConMasterFlag=false;
          pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
          pathTableIndex++;
        }
      }
      // 2 hop
      for (int i=0;i<m_LeafNodes;i++)
      {
        pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
        pathTable[pathTableIndex].pathNodeIdent[1]={2,m_Pod*m_LeafNodes+i};
        for (int j=2;j<MAX_PATH_LEN;j++) pathTable[pathTableIndex].pathNodeIdent[j]=tempIdent;
        for (int j=0;j<MAX_ADDR_NUM;j++) pathTable[pathTableIndex].addrSet[j]=tempAddr;
        pathTable[pathTableIndex].faultLinkCounter=1;// 初始化时默认所有的链路都是坏的
        pathTable[pathTableIndex].dirConMasterFlag=false;
        pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
        pathTableIndex++;
      }
      if (pathTableIndex!=pathNum) printf("pathtableindex is wrong!\n");
      break;
    }
  case 2://leafnode
    // 4 hop: (m_nPods-1)*m_ToRNodes*(m_SpineNodes/m_LeafNodes)
    // 3 hop: (m_nPods-1)*(m_SpineNodes/m_LeafNodes)
    // 2 hop: m_ToRNodes+m_SpineNodes/m_LeafNodes
    pathNum=m_ToRNodes+(m_SpineNodes/m_LeafNodes)*((m_nPods-1)*(m_ToRNodes+1)+1);
    pathTable=(pathtableentry *)malloc(sizeof(pathtableentry)*(pathNum));

    // 4 hop
    for (int i=0;i<m_nPods*m_ToRNodes;i++)// tor
    {
      if (i/m_ToRNodes==m_Pod) continue;
      nexthopandvaildpathnum *numOfVaildPathPerNextHop;
      numOfVaildPathPerNextHop=(nexthopandvaildpathnum *)malloc((m_SpineNodes/m_LeafNodes)*sizeof(nexthopandvaildpathnum));

      for (int j=(m_Ident.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);j<(m_Ident.position%m_LeafNodes+1)*(m_SpineNodes/m_LeafNodes);j++) 
        numOfVaildPathPerNextHop[j%(m_SpineNodes/m_LeafNodes)]={{3,j},0};//1;

      for (int j=(m_Ident.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);j<(m_Ident.position%m_LeafNodes+1)*(m_SpineNodes/m_LeafNodes);j++)// spinenode
      {
        pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
        pathTable[pathTableIndex].pathNodeIdent[1]={3,j};
        pathTable[pathTableIndex].pathNodeIdent[2]={2,(i/m_ToRNodes)*m_LeafNodes+m_Ident.position%m_LeafNodes};
        pathTable[pathTableIndex].pathNodeIdent[3]={1,i};
        for (int k=4;k<MAX_PATH_LEN;k++) pathTable[pathTableIndex].pathNodeIdent[k]=tempIdent;
        for (int k=0;k<MAX_ADDR_NUM;k++) pathTable[pathTableIndex].addrSet[k]=tempAddr;
        // 临时做法
        if (i==0) pathTable[pathTableIndex].addrSet[0]=tempAddrA;
        else if (i==1) pathTable[pathTableIndex].addrSet[0]=tempAddrB;
        pathTable[pathTableIndex].faultLinkCounter=3;// 初始化时默认所有的链路都是坏的
        pathTable[pathTableIndex].dirConMasterFlag=false;
        pathTable[pathTableIndex].numOfVaildPathPerNextHop=numOfVaildPathPerNextHop;
        pathTableIndex++;
      }
    }
    // 3 hop
    for (int i=m_Ident.position%m_LeafNodes;i<m_nPods*m_LeafNodes;i+=m_LeafNodes)// leafnode
    {
      if (i==m_Ident.position) continue;
      nexthopandvaildpathnum *numOfVaildPathPerNextHop;
      numOfVaildPathPerNextHop=(nexthopandvaildpathnum *)malloc((m_SpineNodes/m_LeafNodes)*sizeof(nexthopandvaildpathnum));

      for (int j=(m_Ident.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);j<(m_Ident.position%m_LeafNodes+1)*(m_SpineNodes/m_LeafNodes);j++) 
        numOfVaildPathPerNextHop[j%(m_SpineNodes/m_LeafNodes)]={{3,j},0};//1

      for (int j=(m_Ident.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);j<(m_Ident.position%m_LeafNodes+1)*(m_SpineNodes/m_LeafNodes);j++)// spinenode
      {
        pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
        pathTable[pathTableIndex].pathNodeIdent[1]={3,j};
        pathTable[pathTableIndex].pathNodeIdent[2]={2,i};
        for (int k=3;k<MAX_PATH_LEN;k++) pathTable[pathTableIndex].pathNodeIdent[k]=tempIdent;
        for (int k=0;k<MAX_ADDR_NUM;k++) pathTable[pathTableIndex].addrSet[k]=tempAddr;
        pathTable[pathTableIndex].faultLinkCounter=2;// 初始化时默认所有的链路都是坏的
        pathTable[pathTableIndex].dirConMasterFlag=false;
        pathTable[pathTableIndex].numOfVaildPathPerNextHop=numOfVaildPathPerNextHop;
        pathTableIndex++;
      }
    }
    // 2 hop
    // des is tor
    for (int i=m_Pod*m_ToRNodes;i<(m_Pod+1)*m_ToRNodes;i++)
    {
      pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
      pathTable[pathTableIndex].pathNodeIdent[1]={1,i};
      for (int j=2;j<MAX_PATH_LEN;j++) pathTable[pathTableIndex].pathNodeIdent[j]=tempIdent;
      for (int j=0;j<MAX_ADDR_NUM;j++) pathTable[pathTableIndex].addrSet[j]=tempAddr;
      // 临时做法
      if (i==0) pathTable[pathTableIndex].addrSet[0]=tempAddrA;
      else if (i==1) pathTable[pathTableIndex].addrSet[0]=tempAddrB;
      pathTable[pathTableIndex].faultLinkCounter=1;// 初始化时默认所有的链路都是坏的
      pathTable[pathTableIndex].dirConMasterFlag=false;
      pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
      pathTableIndex++;
    }
    // des is spine
    for (int i=(m_Ident.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);i<(m_Ident.position%m_LeafNodes+1)*(m_SpineNodes/m_LeafNodes);i++)
    {
      pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
      pathTable[pathTableIndex].pathNodeIdent[1]={3,i};
      for (int j=2;j<MAX_PATH_LEN;j++) pathTable[pathTableIndex].pathNodeIdent[j]=tempIdent;
      for (int j=0;j<MAX_ADDR_NUM;j++) pathTable[pathTableIndex].addrSet[j]=tempAddr;
      pathTable[pathTableIndex].faultLinkCounter=1;// 初始化时默认所有的链路都是坏的
      pathTable[pathTableIndex].dirConMasterFlag=false;
      pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
      pathTableIndex++;
    }
    break;
  case 3:// spinenode
    // 3 hop: m_nPods*m_ToRNodes
    // 2 hop: m_nPods
    
    pathNum=m_nPods*(m_ToRNodes+1);
    pathTable=(pathtableentry *)malloc(sizeof(pathtableentry)*pathNum);

    // 3 hop
    for (int i=0;i<m_nPods;i++)
    {
      for (int j=i*m_ToRNodes;j<(i+1)*m_ToRNodes;j++)
      {
        pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
        pathTable[pathTableIndex].pathNodeIdent[1]={2,(m_Ident.position/(m_SpineNodes/m_LeafNodes))+i*m_LeafNodes};
        pathTable[pathTableIndex].pathNodeIdent[2]={1,j};
        for (int k=3;k<MAX_PATH_LEN;k++) pathTable[pathTableIndex].pathNodeIdent[k]=tempIdent;
        for (int k=0;k<MAX_ADDR_NUM;k++) pathTable[pathTableIndex].addrSet[k]=tempAddr;
        // 临时做法
        if (j==0) pathTable[pathTableIndex].addrSet[0]=tempAddrA;
        else if (j==1) pathTable[pathTableIndex].addrSet[0]=tempAddrB;
        pathTable[pathTableIndex].faultLinkCounter=2;// 初始化时默认所有的链路都是坏的
        pathTable[pathTableIndex].dirConMasterFlag=false;
        pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
        pathTableIndex++;
      }
    }
    // 2 hop
    for (int i=0;i<m_nPods;i++)
    {
      pathTable[pathTableIndex].pathNodeIdent[0]=m_Ident;
      pathTable[pathTableIndex].pathNodeIdent[1]={2,(m_Ident.position/(m_SpineNodes/m_LeafNodes))+i*m_LeafNodes};
      for (int j=2;j<MAX_PATH_LEN;j++) pathTable[pathTableIndex].pathNodeIdent[j]=tempIdent;
      for (int j=0;j<MAX_ADDR_NUM;j++) pathTable[pathTableIndex].addrSet[j]=tempAddr;
      pathTable[pathTableIndex].faultLinkCounter=1;// 初始化时默认所有的链路都是坏的
      pathTable[pathTableIndex].dirConMasterFlag=false;
      pathTable[pathTableIndex].numOfVaildPathPerNextHop=NULL;
      pathTableIndex++;
    }
    break;
  default:
    break;
  }
}

void 
Primus::InitiateNodeSockTable()
{
  nodeSockNum=m_SpineNodes+(m_LeafNodes+m_ToRNodes)*m_nPods+3;// 3是控制器的数量
  nodeSockTable=(nodesocktableentry *)malloc(sizeof(nodesocktableentry)*nodeSockNum);
  int nodeSockIndex=0;
  
  for (int i=0;i<m_nPods*m_ToRNodes;i++) nodeSockTable[nodeSockIndex++]={{1,i},-1,0};
  for (int i=0;i<m_nPods*m_LeafNodes;i++) nodeSockTable[nodeSockIndex++]={{2,i},-1,0};
  for (int i=0;i<m_SpineNodes;i++) nodeSockTable[nodeSockIndex++]={{3,i},-1,0};
  for (int i=0;i<3;i++) nodeSockTable[nodeSockIndex++]={{0,i},-1,0};
  if (nodeSockIndex!=nodeSockNum)
    fprintf(stderr,
      "InitiateNodeSockTable error!(nodeSockIndex[%d]!=nodeSockNum[%d])\n",
      nodeSockIndex,
      nodeSockNum);
}

void 
Primus::InitiateControllerTable()
{
  for (int i=0;i<MAX_CTRL_NUM;i++)
  {
    controllerSockTable[i]={{-1,-1},-1,-1,0};
    // controllerSockTable[i].controllerRole//1:node;2:master;3:slave。-1 is nothing
  }
}

void
Primus::InitiateLinkTable()
{
  linkNum=m_SpineNodes*m_nPods+m_LeafNodes*m_nPods*m_ToRNodes;
  linkTable=(linktableentry *)malloc(sizeof(linktableentry)*linkNum);

  struct message tempLastMessage={{tempIdent,tempIdent,tempAddr,tempAddr,0,false},tempIdent,tempIdent,tempIdent,false,1,1,0,1};

  for (int i=0;i<m_SpineNodes;i++)// spine--leaf
  {
    for (int j=0;j<m_nPods;j++)
    {
      linkTable[i*m_nPods+j]=
      {{{3,i},{2,j*m_LeafNodes+i/(m_SpineNodes/m_LeafNodes)},tempAddr,tempAddr,0,false},false,1,tempLastMessage,-1};
    }
  }
  for (int i=0;i<m_nPods;i++)// leaf--tor
  {
    for (int j=0;j<m_LeafNodes;j++)
    {
      for (int k=0;k<m_ToRNodes;k++)
      {
        linkTable[m_SpineNodes*m_nPods+i*m_LeafNodes*m_ToRNodes+j*m_ToRNodes+k]=
        {{{2,i*m_LeafNodes+j},{1,i*m_ToRNodes+k},tempAddr,tempAddr,0,false},false,1,tempLastMessage,-1};
      }
    }
  }
}

int 
Primus::GetLinkIndex(ident identA,ident identB)
{
  ident high,low;
  if (identA.level>identB.level)
  {
    high=identA;
    low=identB;
  }
  else
  {
    high=identB;
    low=identA;
  }

  int linkIndex=-1;
  if (high.level==3) linkIndex=high.position*m_nPods+low.position/m_LeafNodes;
  else if (high.level==2) linkIndex=m_SpineNodes*m_nPods+high.position*m_ToRNodes+low.position%m_ToRNodes;
  return linkIndex;
}

bool
Primus::SameNode(ident nodeIdentA,ident nodeIdentB)
{
  if (nodeIdentA.level==nodeIdentB.level && nodeIdentA.position==nodeIdentB.position) return true;
  else return false;
}

int 
Primus::GetNodeSock(ident nodeIdent)
{
  int nodeSockIndex=0;
  if (nodeIdent.level==1) nodeSockIndex=nodeIdent.position;
  else if (nodeIdent.level==2) nodeSockIndex=m_nPods*m_ToRNodes+nodeIdent.position;
  else if (nodeIdent.level==3) nodeSockIndex=m_nPods*(m_ToRNodes+m_LeafNodes)+nodeIdent.position;

  if (SameNode(nodeSockTable[nodeSockIndex].nodeIdent,nodeIdent))
  {
    return nodeSockTable[nodeSockIndex].nodeSock;
  }
  else
  {
    fprintf(stderr,
      "GetNodeSock error!(nodeSockTable[%d]:%d.%d,nodeIdent:%d.%d).\n",
      nodeSockIndex,
      nodeSockTable[nodeSockIndex].nodeIdent.level,
      nodeSockTable[nodeSockIndex].nodeIdent.position,
      nodeIdent.level,
      nodeIdent.position);
    return -1;
  }
}

void 
Primus::UpdateControllerSockTable(int controllerSock,ident controllerIdent,int controllerRole)
{
  // cout << endl << "UpdateControllerSockTable\n[controllerSock:" << controllerSock << ",controllerIdent:" << controllerIdent.level << "." << controllerIdent.position
  // << ",controllerRole:" << controllerRole << endl << endl;
  for (int i=0;i<MAX_CTRL_NUM;i++)
  {
    // cout << "i:" << i << ",[" << controllerSockTable[i].controllerIdent.level << "." << controllerSockTable[i].controllerIdent.position << endl;
    if (SameNode(controllerSockTable[i].controllerIdent,tempIdent) // 遍历完表未找到直接添加
    || SameNode(controllerSockTable[i].controllerIdent,controllerIdent))
    {
      controllerSockTable[i].controllerIdent=controllerIdent;
      controllerSockTable[i].controllerSock=controllerSock;
      controllerSockTable[i].controllerRole=controllerRole;
      controllerSockTable[i].unRecvNum=0;
      break;
    }
  }
  // cout << endl;
  // PrintControllerSockTable();
}

int 
Primus::SendMessageByTCP(int sock,struct message tempMessage)
{
  tempMessage.transportType=1;
  char sendBuf[MESSAGE_BUF_SIZE];
  memcpy(sendBuf,&tempMessage,sizeof(struct message));
  int ret=send(sock,sendBuf,sizeof(struct message),0);

  if (tempMessage.messageType!=3) PrintMessage(tempMessage);
  return ret;
}

int 
Primus::SendMessageByUDP(struct sockaddr_in localAddr,struct sockaddr_in remoteAddr,struct message tempMessage)
{
  int nodeSock=0;
  int ret=0;

  remoteAddr.sin_family=AF_INET;
  remoteAddr.sin_port=htons(PRIMUS_LISTEN_PORT);

  if ((nodeSock=socket(PF_INET,SOCK_DGRAM,0))<0)
  {
    cout << "SendPathInfoTo sock failed" << endl;
    exit(1);
  }

  int value=1;
  if (setsockopt(nodeSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    cout << "SendPathInfoTo set SO_REUSEPORT error" << endl;
    exit(1);
  }

  if (setsockopt(nodeSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    cout << "SendPathInfoTo set SO_REUSEADDR error" << endl;
    exit(0);
  }

  if ((bind(nodeSock,(struct sockaddr*)&(localAddr),sizeof(localAddr))<0))
  {
    cout << "SendPathInfoTo bind failed"<<std::endl;
    exit(1);
  }

  tempMessage.transportType=2;
  char sendBuf[MESSAGE_BUF_SIZE];
  memcpy(sendBuf,&tempMessage,sizeof(struct message));

  ret=sendto(nodeSock,sendBuf,sizeof(struct message),0,(struct sockaddr *)&remoteAddr,sizeof(remoteAddr));
  close(nodeSock);
  if (tempMessage.messageType!=3) PrintMessage(tempMessage);
  return ret;
}

void* 
Primus::SrcWaitRSThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparamf *)tempThreadParam)->tempPrimus;
  int linkIndex=((threadparamf *)tempThreadParam)->linkIndex;

  for (int i=1;i<=3;i++)
  {
    usleep(WAIT_INTERVAL*i);

    pthread_mutex_lock(&tempPrimus->LinkTableMutex);
    if (tempPrimus->linkTable[linkIndex].startStamp.tv_sec==0 && tempPrimus->linkTable[linkIndex].startStamp.tv_usec==0)// 收到
    {
      pthread_mutex_unlock(&tempPrimus->LinkTableMutex);
      break;
    }
    pthread_mutex_unlock(&tempPrimus->LinkTableMutex);

    if (i==3)// 超时3次未收到
    {
      cout << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position << " din't recv RS["
      << tempPrimus->linkTable[linkIndex].linkInfo.identA.level << "." << tempPrimus->linkTable[linkIndex].linkInfo.identA.position << "--"
      << tempPrimus->linkTable[linkIndex].linkInfo.identB.level << "." << tempPrimus->linkTable[linkIndex].linkInfo.identB.position << "]."
      << endl;
      struct link tempLink={tempPrimus->tempIdent,tempPrimus->tempIdent,tempPrimus->tempAddr,tempPrimus->tempAddr,-1,false};
      struct message tempMessage={tempLink,tempPrimus->m_Ident,tempPrimus->tempIdent,tempPrimus->tempIdent,false,4,tempPrimus->m_Role,-1,1};
      for (int j=0;j<MAX_CTRL_NUM;j++)
      {
        if (tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempPrimus->tempIdent)) break;
        if (tempPrimus->controllerSockTable[j].controllerSock!=-1 && tempPrimus->controllerSockTable[j].controllerRole==3)// 发送给所有的slave
        {
          tempMessage.dstIdent=tempPrimus->controllerSockTable[j].controllerIdent;
          if ((tempPrimus->SendMessageByTCP(tempPrimus->controllerSockTable[j].controllerSock,tempMessage))==MESSAGE_BUF_SIZE)
            cout << "Send to slave " << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << "." << endl;
        }
      }
      usleep(WAIT_INTERVAL*8);
      for (int j=0;j<MAX_CTRL_NUM;j++)
      {
        if (tempPrimus->controllerSockTable[j].controllerSock!=-1 && tempPrimus->controllerSockTable[j].controllerRole==2)// 发送给新的master
        {
          tempMessage=((threadparamf *)tempThreadParam)->tempMessage;
          tempMessage.dstIdent=tempPrimus->controllerSockTable[j].controllerIdent;
          tempPrimus->SendMessageByTCP(tempPrimus->controllerSockTable[j].controllerSock,tempMessage);
          break;
        }
      }
      i=1;
      continue;
    }
  }
}

sockaddr_in 
Primus::GetLocalAddrByNeighborIdent(ident neighborIdent)
{
  for (int i=0;i<neighborTable.size();i++)
  {
    if (SameNode(neighborTable[i].neighborIdent,neighborIdent)) return neighborTable[i].srcAddr;
  }
  return tempAddr;
}

sockaddr_in 
Primus::GetGateAddrByNeighborIdent(ident neighborIdent)
{
  for (int i=0;i<neighborTable.size();i++)
  {
    if (SameNode(neighborTable[i].neighborIdent,neighborIdent)) return neighborTable[i].neighborAddr;
  }
  return tempAddr;
}

string 
Primus::GetLocalNICNameByNeighborIdent(ident neighborIdent)
{
  for (int i=0;i<neighborTable.size();i++)
  {
    if (SameNode(neighborTable[i].neighborIdent,neighborIdent)) return neighborTable[i].localNICName;
  }
  return "";
}

void 
Primus::SendLSToController(struct link tempLink,int linkIndex,ident faultNextHop)
{
  pthread_t SrcWaitRSThreadID;
  threadparamf *tempThreadParam=new threadparamf();
  tempThreadParam->tempPrimus=this;
  tempThreadParam->linkIndex=linkIndex;

  gettimeofday(&linkTable[linkIndex].startStamp,NULL);
  for (int i=0;i<MAX_CTRL_NUM;i++)
  {
    if (controllerSockTable[i].controllerSock==-1 || SameNode(controllerSockTable[i].controllerIdent,tempIdent)) 
      break;

    struct message tempMessage={tempLink,m_Ident,tempIdent,controllerSockTable[i].controllerIdent,false,2,m_Role,-1,1};
    SendMessageByTCP(controllerSockTable[i].controllerSock,tempMessage);

    if (controllerSockTable[i].controllerRole==2) 
    {
      tempThreadParam->tempMessage=tempMessage;
      if ((pthread_create(&SrcWaitRSThreadID,NULL,SrcWaitRSThread,(void *)tempThreadParam))!=0)
      {
        cout << "Create SrcWaitRSThread failed." << endl;
      }
      srand((unsigned)time(NULL));
      for (int j=0;j<MAX_FOWNODE_NUM;j++)
      {
        int pathIndex=rand()%pathNum;
        if ((pathTable[pathIndex].faultLinkCounter==0)
          && pathTable[pathIndex].addrSet[0].sin_addr.s_addr!=tempAddr.sin_addr.s_addr
          && !SameNode(pathTable[pathIndex].pathNodeIdent[1],faultNextHop))// 路径有效
        {
          SendMessageByUDP(GetLocalAddrByNeighborIdent(pathTable[pathIndex].pathNodeIdent[1]),
                          pathTable[pathIndex].addrSet[0],
                          tempMessage);
        }
      }
    }
  }
}

void* 
Primus::LinkTimerThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparamc *)tempThreadParam)->tempPrimus;
  int tempLinkIndex=((threadparamc *)tempThreadParam)->linkIndex;

  int sleepTimes=0;

  while (1)
  {
    usleep(tempPrimus->m_DefaultLinkTimer);
    sleepTimes++;
    if (sleepTimes>=tempPrimus->linkTable[tempLinkIndex].linkSleepTimes) break;
  }
  
  if (!tempPrimus->SameNode(tempPrimus->linkTable[tempLinkIndex].lastMessage.srcIdent,tempPrimus->tempIdent)
    && tempPrimus->linkTable[tempLinkIndex].lastMessage.linkInfo.linkStatus!=tempPrimus->linkTable[tempLinkIndex].linkInfo.linkStatus)
  {
    tempPrimus->linkTable[tempLinkIndex].linkInfo.linkStatus=tempPrimus->linkTable[tempLinkIndex].lastMessage.linkInfo.linkStatus;
    tempPrimus->EnqueueMessageIntoEventQueue(tempPrimus->linkTable[tempLinkIndex].lastMessage);
  }
  tempPrimus->linkTable[tempLinkIndex].isStartTimer=false;
  tempPrimus->linkTable[tempLinkIndex].linkSleepTimes=1;
  // tempPrimus->PrintLinkTable();
}

bool 
Primus::AddNeighborEntry(string localNICName,struct sockaddr_in localAddr,struct sockaddr_in neighborAddr)// 发出NDHello
{
  for (int i=0;i<neighborTable.size();i++)
  {
    // 如果先收到NDHello
    if (neighborTable[i].neighborAddr.sin_addr.s_addr==neighborAddr.sin_addr.s_addr)// 存在
    {
      neighborTable[i].localNICName=localNICName;
      neighborTable[i].srcAddr=localAddr;
      return true;
    }
  }
  // 不存在
  struct neighborinfo tempNeighborInfo={tempIdent,localNICName,neighborAddr,localAddr,1,1};
  neighborTable.push_back(tempNeighborInfo);
  return true;
}

void*
Primus::SendNDHelloThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparame *)tempThreadParam)->tempPrimus;
  struct sockaddr_in localAddr=((threadparame *)tempThreadParam)->tempLocalAddr;
  struct sockaddr_in neighborAddr=((threadparame *)tempThreadParam)->tempNeighborAddr;
  string tempLocalNICName=((threadparame *)tempThreadParam)->tempLocalNICName;
  
  struct sockaddr_in fromAddr;//服务端端地址
  bzero(&fromAddr,sizeof(struct sockaddr_in));

  int sock = -1;
  int so_broadcast = 1;
  int from_len = sizeof(struct sockaddr_in);
  struct timeval timeout;
  timeout.tv_sec=3;
  timeout.tv_usec=0;
  
  //set dgram for client
  if ((sock=socket(AF_INET, SOCK_DGRAM, 0))<0)
  {
    cout << "NDHelloThread Create Socket Failed." << std::endl;
    exit(1);
  }

  int value=1;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    cout << "NDHelloThread set SO_REUSEPORT error" << endl;
    exit(1);
  }

  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    cout << "NDHelloThread set SO_REUSEADDR error" << endl;
    exit(1);
  }

  if ((bind(sock,(struct sockaddr*)&(localAddr),sizeof(localAddr))<0))
  {
    cout << "NDHelloThread Bind interface Failed." << endl;
    exit(1);
  }

  //the default socket doesn't support broadcast,so we should set socket discriptor to do it.
  if ((setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast))) < 0)
  {
    cout << "NDHelloThread SetSocketOpt Failed." << endl;
    exit(1);
  }

  // int time_live=1;
  int timeoutNum=0;
  // setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));

  char sendBuf[ND_BUF_SIZE],recvBuf[ND_BUF_SIZE];
  NDmessage tempNDMessage={1,tempPrimus->m_Ident,tempPrimus->tempIdent,localAddr};
  memcpy(sendBuf,&tempNDMessage,sizeof(NDmessage));

  while (1)
  {
    if ((sendto(sock,sendBuf,sizeof(NDmessage),0,(struct sockaddr *)&neighborAddr,sizeof(neighborAddr)))<0)
    {
      fprintf(stderr, "Send NDHello to neighbor[%s] failed.\n", inet_ntoa(neighborAddr.sin_addr));
    }

    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));

    memset(recvBuf,'\0',ND_BUF_SIZE);// 还要用来接收
    if((value=recvfrom(sock,recvBuf,ND_BUF_SIZE,0,(struct sockaddr*)&fromAddr,(socklen_t*)&from_len))<0)
    {
      timeoutNum++;
      if (timeoutNum>=3)// 超时3次，直接挂掉
      {
        cout << "Recv NDReply from[" << inet_ntoa(neighborAddr.sin_addr) << "][error:" << strerror(errno) << "(errno:" << errno <<  ")]." << endl;
        break;
      }
      else continue;
    }
    
    memcpy(&tempNDMessage,recvBuf,ND_BUF_SIZE);
    if (tempNDMessage.messageType==2)
    {
      // cout << "Recv NDReply from[" << inet_ntoa(fromAddr.sin_addr) << "]." << endl;
      tempPrimus->RecvNDReply(tempNDMessage,fromAddr);
    }
    else
    {
      cout << "Recv invaild NDMessage." << endl;
    }
    break;
  }
}

void* 
Primus::ListenNICThread(void* tempThreadParam)
{
  // cout << "Listening NIC......" << endl;
  Primus *tempPrimus=((threadparama*)tempThreadParam)->tempPrimus;

  struct ifaddrs *ifa;

  while (1)
  {
    if (0!=getifaddrs(&ifa))
    {
      cout << "Getifaddrs error." << endl;
      break;
    }
    for (;ifa!=NULL;)
    {
      if (ifa->ifa_flags==69699 
        && ifa->ifa_name
        && ifa->ifa_addr
        && ifa->ifa_netmask
        && (*ifa).ifa_ifu.ifu_dstaddr
        && ifa->ifa_name[0]=='e')
      {
        bool isFind=false;
        for (int i=0;i<tempPrimus->neighborTable.size();i++)
        {
          if (!strcmp(tempPrimus->neighborTable[i].localNICName.c_str(),ifa->ifa_name))// 已经存在
          {
            // cout << "Old NIC " << ifa->ifa_name << endl;
            // 正在进行邻居发现或者已经完成
            if (tempPrimus->neighborTable[i].NICFlag==0) tempPrimus->neighborTable[i].NICFlag=1;// 正常检查
            if (tempPrimus->neighborTable[i].NDType>0) isFind=true;
            if (tempPrimus->neighborTable[i].NICFlag==-1)// 故障链路恢复
            {
              tempPrimus->neighborTable[i].NICFlag=1;
              // 向master发送link up
              int linkIndex=tempPrimus->GetLinkIndex(tempPrimus->m_Ident,tempPrimus->neighborTable[i].neighborIdent);
              struct link tempLink={tempPrimus->m_Ident,
                                    tempPrimus->neighborTable[i].neighborIdent,
                                    tempPrimus->neighborTable[i].srcAddr,
                                    tempPrimus->neighborTable[i].neighborAddr,
                                    tempPrimus->linkTable[linkIndex].linkInfo.eventId+1,
                                    true};
              tempPrimus->SendLSToController(tempLink,linkIndex,tempPrimus->tempIdent);
            }
            break;
          }
        }
        if (isFind==false && strcmp(ifa->ifa_name,MGMT_INTERFACE))// new up nic
        {
          string tempAddress=inet_ntoa(((sockaddr_in *)ifa->ifa_addr)->sin_addr);

          if ((tempAddress[0]=='3' && tempAddress[1]=='2') || (tempAddress[0]=='2' && tempAddress[1]=='1'))
          {
            // cout << "New NIC " << ifa->ifa_name << endl;
            struct sockaddr_in neighborAddr;
            bzero(&neighborAddr,sizeof(struct sockaddr_in));
            neighborAddr.sin_family=AF_INET;
            string address=inet_ntoa(((sockaddr_in *)ifa->ifa_addr)->sin_addr);
            address[address.size()-1]=(address[address.size()-1]=='1')? '2':'1';
            neighborAddr.sin_addr.s_addr=inet_addr((char*)address.c_str());
            neighborAddr.sin_port=htons(ND_LISTEN_PORT);

            threadparame *tempThreadParam=new threadparame();
            tempThreadParam->tempPrimus=tempPrimus;
            tempThreadParam->tempLocalAddr=*((sockaddr_in *)ifa->ifa_addr);
            tempThreadParam->tempNeighborAddr=neighborAddr;
            tempThreadParam->tempLocalNICName=ifa->ifa_name;

            tempPrimus->AddNeighborEntry(tempThreadParam->tempLocalNICName,
                                         tempThreadParam->tempLocalAddr,
                                         tempThreadParam->tempNeighborAddr);

            pthread_t NDThreadID;
            if (pthread_create(&NDThreadID,NULL,SendNDHelloThread,(void *)tempThreadParam)!=0)
            {
              cout << "Create SendNDHelloThread failed." << endl;
            }
          }
        }
      }
      ifa=ifa->ifa_next;
    }

    for (int i=0;i<tempPrimus->neighborTable.size();i++)
    {
      if (tempPrimus->neighborTable[i].NICFlag==1)// 正常 
      {
        tempPrimus->neighborTable[i].NICFlag=0;
      }
      else if(tempPrimus->neighborTable[i].NICFlag==0)// 故障
      {
        tempPrimus->neighborTable[i].NICFlag=-1;
        // 向master发送链路故障信息
        int linkIndex=tempPrimus->GetLinkIndex(tempPrimus->m_Ident,tempPrimus->neighborTable[i].neighborIdent); 
        struct link tempLink={tempPrimus->m_Ident,
                              tempPrimus->neighborTable[i].neighborIdent,
                              tempPrimus->neighborTable[i].srcAddr,
                              tempPrimus->neighborTable[i].neighborAddr,
                              tempPrimus->linkTable[linkIndex].linkInfo.eventId+1,
                              false};
        tempPrimus->SendLSToController(tempLink,linkIndex,tempPrimus->neighborTable[i].neighborIdent);
      }
    }
    usleep(NIC_CHECK_INTERVAL);
  }
}

bool 
Primus::RecvNDReply(struct NDmessage tempNDMessage,struct sockaddr_in neighborAddr)
{
  for (int i=0;i<neighborTable.size();i++)
  {
    if (neighborTable[i].srcAddr.sin_addr.s_addr==tempNDMessage.srcAddr.sin_addr.s_addr)
    {
      neighborTable[i].neighborIdent=tempNDMessage.neighborIdent;
      neighborTable[i].neighborAddr=neighborAddr;
      neighborTable[i].NDType++;
      if (neighborTable[i].NDType==3)
      {
        struct link tempLink={m_Ident,
                              neighborTable[i].neighborIdent,
                              neighborTable[i].srcAddr,
                              neighborTable[i].neighborAddr,
                              1,
                              true};
        int linkIndex=GetLinkIndex(m_Ident,neighborTable[i].neighborIdent);
        SendLSToController(tempLink,linkIndex,tempIdent);
        // PrintNeighborTable();
      }
      return true;
    }
  }
  return true;
}

bool 
Primus::RecvNDHello(ident neighborIdent,struct sockaddr_in neighborAddr)// 可能还不存在相关表项
{
  for (int i=0;i<neighborTable.size();i++)
  {
    if (neighborTable[i].neighborAddr.sin_addr.s_addr==neighborAddr.sin_addr.s_addr)// 已经存在
    {
      neighborTable[i].neighborIdent=neighborIdent;
      neighborTable[i].NDType++;
      if (neighborTable[i].NDType==3)
      {
        struct link tempLink={m_Ident,
                              neighborTable[i].neighborIdent,
                              neighborTable[i].srcAddr,
                              neighborTable[i].neighborAddr,
                              1,
                              true};
        int linkIndex=GetLinkIndex(m_Ident,neighborTable[i].neighborIdent);
        SendLSToController(tempLink,linkIndex,tempIdent);
        // PrintNeighborTable();
      }
      return true;
    }
  }
  // 不存在
  struct neighborinfo tempNeighborInfo={neighborIdent," ",neighborAddr,tempAddr,1,1};
  neighborTable.push_back(tempNeighborInfo);
  return true;
}

void* 
Primus::RecvNDMessageThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparama *)tempThreadParam)->tempPrimus;
  int value = -1;
  int len = sizeof(struct sockaddr_in);
  int sock = -1;
  struct sockaddr_in serverAddr;//服务端地址
  struct sockaddr_in clientAddr;//客户端地址
  
  bzero(&serverAddr,sizeof(struct sockaddr_in));
  serverAddr.sin_family = AF_INET;//Set as IP communication
  serverAddr.sin_addr.s_addr = htons(INADDR_ANY);//server IP address:allowed to connect any local address
  serverAddr.sin_port = htons(ND_LISTEN_PORT);//server port
    
  //广播地址
  bzero(&clientAddr,sizeof(struct sockaddr_in));
  clientAddr.sin_family = AF_INET;//Set as IP communication
  clientAddr.sin_addr.s_addr = htons(INADDR_ANY);//server IP address:allowed to connect any local address
  clientAddr.sin_port = htons(ND_LISTEN_PORT);//server port

  if((sock=socket(AF_INET, SOCK_DGRAM, 0))<0)
  {
    cout << "RecvNDMessageThread Create Socket Failed." << endl;
    exit(1);
  }

  value=1;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    cout << "RecvNDMessageThread set SO_REUSEPORT error" << endl;
    exit(1);
  }

  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    cout << "RecvNDMessageThread set SO_REUSEADDR error" << endl;
    exit(1);
  }

  //bind 
  if((bind(sock,(struct sockaddr*) &serverAddr,sizeof(serverAddr))) < 0)
  {
    cout << "RecvNDMessageThread Bind Socket Failed(" << inet_ntoa(serverAddr.sin_addr) << ")" << endl;
    exit(1);
  }
  
  // cout << "Waiting for ND......" << endl;

  char recvBuf[ND_BUF_SIZE],sendBuf[ND_BUF_SIZE];
  
  while(1)
  {
    memset(recvBuf,'\0',ND_BUF_SIZE);
    if((value=recvfrom(sock,recvBuf,ND_BUF_SIZE,0,(struct sockaddr *)&clientAddr,(socklen_t*)&len)) < 0)
    {
      cout << "RecvNDMessageThread Socket recvfrom Failed." << endl;
      break;
    }
    //排除本地广播
    if(strstr((char *)inet_ntoa(clientAddr.sin_addr), "127.0.0.1"))
    {
      continue;
    }

    // 先处理，再回复
    struct NDmessage tempNDMessage;
    memcpy(&tempNDMessage,recvBuf,sizeof(struct NDmessage));

    // cout << "Recv NDmessage from[" << inet_ntoa(clientAddr.sin_addr) << "][" << clientAddr.sin_port << "]." << endl;

    if (tempNDMessage.messageType==1)// hello
    {
      tempPrimus->RecvNDHello(tempNDMessage.myIdent,tempNDMessage.srcAddr);

      tempNDMessage.messageType=2;
      tempNDMessage.neighborIdent.level=tempPrimus->m_Ident.level;
      tempNDMessage.neighborIdent.position=tempPrimus->m_Ident.position;

      memcpy(sendBuf,&tempNDMessage,ND_BUF_SIZE);

      if (sendto(sock,sendBuf,ND_BUF_SIZE,0,(struct sockaddr *)&clientAddr,len)<0)
        cout << "RecvNDMessageThread Send ND reply error." << endl;
      // else cout << "Send NDReply to[" << inet_ntoa(clientAddr.sin_addr) << "]." << endl;
    }
  }
}

bool 
Primus::UpdateNeighbor(ident neighborIdent,struct sockaddr_in neighborAddr)
{
  for (int i=0;i<neighborTable.size();i++)
  {
    if (neighborTable[i].neighborAddr.sin_addr.s_addr==neighborAddr.sin_addr.s_addr)
    {
      neighborTable[i].neighborIdent=neighborIdent;
      // PrintNeighborTable();
      return true;
    }
  }
  return true;
}

bool 
Primus::UpdateLinkTable(struct message tempMessage)// link status change or ip change
{
  struct link tempLink=tempMessage.linkInfo;
  ident high,low;
  if (tempLink.identA.level>tempLink.identB.level)
  {
    high=tempLink.identA;
    low=tempLink.identB;
  }
  else if (tempLink.identA.level<tempLink.identB.level)
  {
    high=tempLink.identB;
    low=tempLink.identA;
  }

  int linkIndex=0;
  if (high.level==3) linkIndex=high.position*m_nPods+low.position/m_LeafNodes;
  else if (high.level==2) linkIndex=m_SpineNodes*m_nPods+high.position*m_ToRNodes+low.position%m_ToRNodes;
  else 
  {
    cout << "Update invaild link " << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
    << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
    if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
    else cout << "DONW";
    cout << " from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position << endl;
  }

  // cout << "linkIndex:" << linkIndex << endl;

  if (tempLink.eventId>linkTable[linkIndex].linkInfo.eventId)// status change
  {
    if (tempLink.eventId==1 && linkTable[linkIndex].linkInfo.eventId==0)// 初始化
    {
      if (SameNode(tempMessage.linkInfo.identA,linkTable[linkIndex].linkInfo.identA))
      {
        memcpy(&linkTable[linkIndex].linkInfo.addrA,&tempMessage.linkInfo.addrA,sizeof(sockaddr_in));
        memcpy(&linkTable[linkIndex].linkInfo.addrB,&tempMessage.linkInfo.addrB,sizeof(sockaddr_in));
      }
      else
      {
        memcpy(&linkTable[linkIndex].linkInfo.addrA,&tempMessage.linkInfo.addrB,sizeof(sockaddr_in));
        memcpy(&linkTable[linkIndex].linkInfo.addrB,&tempMessage.linkInfo.addrA,sizeof(sockaddr_in));
      }
      // 添加，临时做法
      if (SameNode(m_Ident,tempLink.identA)) UpdateNeighbor(tempLink.identB,tempLink.addrB);
      else if (SameNode(m_Ident,tempLink.identB)) UpdateNeighbor(tempLink.identA,tempLink.addrA);
      
      int startIndex=0;
      if (m_Ident.level==1) startIndex=m_SpineNodes*(m_nPods-1)*m_ToRNodes;
      else if (m_Ident.level==2) startIndex=(m_nPods-1)*m_ToRNodes*(m_SpineNodes/m_LeafNodes);
      else if (m_Ident.level==3) startIndex=m_nPods*m_ToRNodes;

      sockaddr_in tempDstAddr;
      ident tempDstIdent=tempIdent;
      vector<nexthopandweight> nextHopAndWeight;

      for (int i=startIndex;i<pathNum;i++)
      {
        // 找到目的节点
        int dstNodeIndex=0;
        while (!SameNode(pathTable[i].pathNodeIdent[dstNodeIndex],tempIdent)) dstNodeIndex++;
        dstNodeIndex--;

        if (!SameNode(pathTable[i].pathNodeIdent[dstNodeIndex],tempDstIdent))
        {
          tempDstIdent=pathTable[i].pathNodeIdent[dstNodeIndex];
          if (nextHopAndWeight.size()>1)//multi
            AddMultiRoute(tempDstAddr,24,nextHopAndWeight);
          else if (nextHopAndWeight.size()==1)//single
            AddSingleRoute(tempDstAddr,24,nextHopAndWeight[0]);
          nextHopAndWeight.clear();
        }

        if (SameNode(tempLink.identA,pathTable[i].pathNodeIdent[dstNodeIndex])) 
        {
          if ((tempLink.identA.level==3 && !SameNode(tempLink.identB,pathTable[i].pathNodeIdent[dstNodeIndex-1])) 
            || (tempLink.identA.level!=3 && tempLink.identB.level!=pathTable[i].pathNodeIdent[dstNodeIndex-1].level))
          {
            for (int k=0;k<MAX_ADDR_NUM;k++)
            {
              if (pathTable[i].addrSet[k].sin_addr.s_addr==tempAddr.sin_addr.s_addr)
              {
                pathTable[i].addrSet[k]=tempLink.addrA;
                tempDstAddr=tempLink.addrA;
                if (pathTable[i].faultLinkCounter==0)
                {
                  string tempLocalNICName=GetLocalNICNameByNeighborIdent(pathTable[i].pathNodeIdent[1]);
                  bool isFind=false;
                  for (int l=0;l<nextHopAndWeight.size();l++)
                  {
                    if (nextHopAndWeight[l].NICName==tempLocalNICName)
                    {
                      nextHopAndWeight[l].weight++;
                      isFind=true;
                      break;
                    }
                  }
                  if (isFind==false)
                  {
                    struct nexthopandweight tempNextHopAndWeight={tempLocalNICName,
                                                                  GetGateAddrByNeighborIdent(pathTable[i].pathNodeIdent[1]),
                                                                  GetLocalAddrByNeighborIdent(pathTable[i].pathNodeIdent[1]),
                                                                  1};
                    nextHopAndWeight.push_back(tempNextHopAndWeight);
                  }
                }
                break;
              }
            }
          }
        }
        else if (SameNode(tempLink.identB,pathTable[i].pathNodeIdent[dstNodeIndex]))
        {
          if ((tempLink.identB.level==3 && !SameNode(tempLink.identA,pathTable[i].pathNodeIdent[dstNodeIndex-1])) 
            || (tempLink.identB.level!=3 && tempLink.identA.level!=pathTable[i].pathNodeIdent[dstNodeIndex-1].level))
          {
            for (int k=0;k<MAX_ADDR_NUM;k++)
            {
              if (pathTable[i].addrSet[k].sin_addr.s_addr==tempAddr.sin_addr.s_addr)
              {
                pathTable[i].addrSet[k]=tempLink.addrB;
                tempDstAddr=tempLink.addrB;
                if (pathTable[i].faultLinkCounter==0)
                {
                  string tempLocalNICName=GetLocalNICNameByNeighborIdent(pathTable[i].pathNodeIdent[1]);
                  bool isFind=false;
                  for (int l=0;l<nextHopAndWeight.size();l++)
                  {
                    if (nextHopAndWeight[l].NICName==tempLocalNICName)
                    {
                      nextHopAndWeight[l].weight++;
                      isFind=true;
                      break;
                    }
                  }
                  if (isFind==false)
                  {
                    struct nexthopandweight tempNextHopAndWeight={tempLocalNICName,
                                                                  GetGateAddrByNeighborIdent(pathTable[i].pathNodeIdent[1]),
                                                                  GetLocalAddrByNeighborIdent(pathTable[i].pathNodeIdent[1]),
                                                                  1};
                    nextHopAndWeight.push_back(tempNextHopAndWeight);
                  }
                }
                break;
              }
            }
          }
        }
      }
      if (nextHopAndWeight.size()>1)//multi
        AddMultiRoute(tempDstAddr,24,nextHopAndWeight);
      else if (nextHopAndWeight.size()==1)//single
        AddSingleRoute(tempDstAddr,24,nextHopAndWeight[0]);
      nextHopAndWeight.clear();
    }

    linkTable[linkIndex].linkInfo.eventId=tempLink.eventId;

    if (m_Role==1)// node
    {
      if (tempLink.linkStatus!=linkTable[linkIndex].linkInfo.linkStatus)
      {
        linkTable[linkIndex].linkInfo.linkStatus=tempLink.linkStatus;
        // PrintLinkTable();
        return true;
      }
    }
    else if (m_Role==2 || m_Role==3)
    {
      if (linkTable[linkIndex].linkInfo.linkStatus==true && tempLink.linkStatus==false)// 收到链路故障信息立即处理
      {
        linkTable[linkIndex].linkInfo.linkStatus=tempLink.linkStatus;
        // PrintLinkTable();
        if (tempMessage.transportType==1) recvTcpNum++;
        else if (tempMessage.transportType==2) recvUdpNum++;
        return true;
      }
      else if (linkTable[linkIndex].linkInfo.linkStatus==false && tempLink.linkStatus==true)
      {
        if (linkTable[linkIndex].isStartTimer==false)// 快速恢复
        {
          linkTable[linkIndex].linkInfo.linkStatus=tempLink.linkStatus;
          linkTable[linkIndex].isStartTimer=true;

          pthread_t timerThreadId;
          threadparamc *tempThreadParam=new threadparamc();
          tempThreadParam->tempPrimus=this;
          tempThreadParam->linkIndex=linkIndex;

          if (pthread_create(&timerThreadId,NULL,LinkTimerThread,(void*)tempThreadParam)!=0)
          {
            fprintf(stderr, "ERROR! Thread create failed!\n");
            exit(1);
          }
          // PrintLinkTable();
          if (tempMessage.transportType==1) recvTcpNum++;
          else if (tempMessage.transportType==2) recvUdpNum++;
          return true;
        }
      }
      // cout << "Waiting......" << endl;
      linkTable[linkIndex].linkSleepTimes+=1;
      linkTable[linkIndex].lastMessage=tempMessage;
    }
  }
  else //ip change
  {
    // 
  }
  // PrintLinkTable();
  return false;
}

bool // 更新一部分path，startIndex,endIndex是起始位置，numOfPathPerDstNode是每个node的path数量
Primus::UpdateMultiPathBlock(int startIndex,int affectedNextHopIndex,int numOfModifyPathEntryPerDstNode,int numOfDstNode,int numOfJumpPath,bool linkStatus)
{
  int numOfNormalNextHop=0;
  int numOfNormalPathPerNextHop=0;
  if (m_Ident.level==1) 
  {
    numOfNormalNextHop=m_LeafNodes;
    numOfNormalPathPerNextHop=m_SpineNodes/m_LeafNodes;
  }
  else if (m_Ident.level==2) 
  {
    numOfNormalNextHop=m_SpineNodes/m_LeafNodes;
    numOfNormalPathPerNextHop=1;
  }

  for (int i=0;i<numOfDstNode;i++)// dst数量
  {
    int pathIndex=startIndex+i*numOfJumpPath;// 开始位置 
    bool isNeedToUpdateRoute=false;
    int numOfOldVaildNextHop=0;//修改前有效的下一跳数量
    bool affectedMultiNexthop=false;
    if (affectedNextHopIndex==-1)// 影响多个下一跳
      affectedMultiNexthop=true;
    
    for (int j=0;j<numOfNormalNextHop;j++)
    {
      if (pathTable[pathIndex].numOfVaildPathPerNextHop[j].vaildPathNum>0) numOfOldVaildNextHop++;
    }

    for (int j=0;j<numOfModifyPathEntryPerDstNode;j++)// 需要修改的路径数
    {
      if (affectedMultiNexthop==true)
        affectedNextHopIndex=pathTable[pathIndex].pathNodeIdent[1].position%(m_SpineNodes/m_LeafNodes);// 此处的m_SpineNodes/m_LeafNodes是leafnode的下一跳数
      if (linkStatus==false) pathTable[pathIndex].faultLinkCounter++;
      else pathTable[pathIndex].faultLinkCounter--;
      if (linkStatus==true && pathTable[pathIndex].faultLinkCounter==0)
      {
        isNeedToUpdateRoute=true;
        pathTable[pathIndex].numOfVaildPathPerNextHop[affectedNextHopIndex].vaildPathNum++;
      } 
      else if (linkStatus==false && pathTable[pathIndex].faultLinkCounter==1) 
      {
        isNeedToUpdateRoute=true;
        pathTable[pathIndex].numOfVaildPathPerNextHop[affectedNextHopIndex].vaildPathNum--;
      }
      pathIndex++;
    }
    // // 处理路由
    if (isNeedToUpdateRoute==true)
    {
      vector<struct nexthopandweight> nextHopAndWeight;// 所有的路由的下一跳都是相同的，只有weight不同
      int isNeedToAddDefaultRoute=0; 
      for (int j=0;j<numOfNormalNextHop;j++)
      {
        if (pathTable[pathIndex-1].numOfVaildPathPerNextHop[j].vaildPathNum==0) continue;
        if (pathTable[pathIndex-1].numOfVaildPathPerNextHop[j].vaildPathNum==numOfNormalPathPerNextHop)
          isNeedToAddDefaultRoute++;

        struct nexthopandweight tempNextHopAndWeight;
        for (int k=0;k<neighborTable.size();k++)
        {
          if (SameNode(neighborTable[k].neighborIdent,
            pathTable[pathIndex-1].numOfVaildPathPerNextHop[j].nextHopIdent))
          {
            tempNextHopAndWeight.NICName=neighborTable[k].localNICName;
            tempNextHopAndWeight.gateAddr=neighborTable[k].neighborAddr;
            tempNextHopAndWeight.srcAddr=neighborTable[k].srcAddr;
            tempNextHopAndWeight.weight=pathTable[pathIndex-1].numOfVaildPathPerNextHop[j].vaildPathNum;
            nextHopAndWeight.push_back(tempNextHopAndWeight);
            break;
          }
        }
      }
      for (int j=0;j<MAX_ADDR_NUM;j++)
      {
        if (pathTable[pathIndex-1].addrSet[j].sin_addr.s_addr!=tempAddr.sin_addr.s_addr)// 有效地址
        {
          if (isNeedToAddDefaultRoute==numOfNormalNextHop)// 所有路径都是正常的
          {
            DelRoute(pathTable[pathIndex-1].addrSet[j],24);
            AddMultiRoute(pathTable[pathIndex-1].addrSet[j],16,nextHopAndWeight);
          }
          else
            if (numOfOldVaildNextHop==0 && nextHopAndWeight.size()==1)//从无到添加单路径
              AddSingleRoute(pathTable[pathIndex-1].addrSet[j],24,nextHopAndWeight[0]);
            else if (numOfOldVaildNextHop>1 && nextHopAndWeight.size()==1)//从多路径变为单路径
            {
              if (numOfOldVaildNextHop==numOfNormalNextHop)
                DelRoute(pathTable[pathIndex-1].addrSet[j],16);
              else 
                DelRoute(pathTable[pathIndex-1].addrSet[j],24);
              AddSingleRoute(pathTable[pathIndex-1].addrSet[j],24,nextHopAndWeight[0]);
            }
            // 从无到添加多路径或者修改多路径
            else if ((numOfOldVaildNextHop==0 || numOfOldVaildNextHop>1) 
              && (nextHopAndWeight.size()>1 && numOfOldVaildNextHop!=nextHopAndWeight.size()))
              AddMultiRoute(pathTable[pathIndex-1].addrSet[j],24,nextHopAndWeight);
            else if (numOfOldVaildNextHop==1 && nextHopAndWeight.size()>1)//从单路径变为多路径
            {
              DelRoute(pathTable[pathIndex-1].addrSet[j],24);
              AddMultiRoute(pathTable[pathIndex-1].addrSet[j],24,nextHopAndWeight);
            }
        }
        else break;
      }
    }
  }
  // PrintPathTable();
  return true;
}

bool 
Primus::UpdateSinglePathBlock(int startIndex,int numOfModifyPathEntryPerDstNode,int numOfDstNode,int numOfJumpPath,bool linkStatus)
{
  for (int i=0;i<numOfDstNode;i++)
  {
    int pathIndex=startIndex+i*numOfJumpPath;
    struct nexthopandweight tempNextHopAndWeight;
    for (int j=0;j<neighborTable.size();j++)
    {
      if (SameNode(neighborTable[j].neighborIdent,tempIdent)) return false;
      if (SameNode(pathTable[pathIndex].pathNodeIdent[1],neighborTable[j].neighborIdent))
      {
        tempNextHopAndWeight.NICName=neighborTable[j].localNICName;// 记得修改
        tempNextHopAndWeight.gateAddr=neighborTable[j].neighborAddr;
        tempNextHopAndWeight.srcAddr=neighborTable[j].srcAddr;
        tempNextHopAndWeight.weight=1;// 但路径不要权重
        break;
      }
    }

    int numOfOldVaildPath=0;
    int numofNewVaildPath=0;

    for (int j=0;j<numOfModifyPathEntryPerDstNode;j++)
    {
      if (pathTable[pathIndex].faultLinkCounter==0) numOfOldVaildPath++;
      if (linkStatus==false) pathTable[pathIndex].faultLinkCounter++;
      else pathTable[pathIndex].faultLinkCounter--;
      if (pathTable[pathIndex].faultLinkCounter==0) numofNewVaildPath++;
      if (pathTable[pathIndex].faultLinkCounter<0) fprintf(stderr,"(%d.%d) pathTable[%d].faultLinkCounter:%d.\n",m_Ident.level,m_Ident.position,pathIndex,pathTable[pathIndex].faultLinkCounter);
      pathIndex++;
    }
    // 修改路由
    if (numOfOldVaildPath>0 && numofNewVaildPath==0)
    {
      for (int j=0;j<MAX_ADDR_NUM;j++)
      {
        if (pathTable[pathIndex-1].addrSet[j].sin_addr.s_addr!=tempAddr.sin_addr.s_addr)// 有效地址
        {
          DelRoute(pathTable[pathIndex-1].addrSet[j],24);
        }
        else break;
      }
    }
    else if (numOfOldVaildPath==0 && numofNewVaildPath>0)
    {
      for (int j=0;j<MAX_ADDR_NUM;j++)
      {
        if (pathTable[pathIndex-1].addrSet[j].sin_addr.s_addr!=tempAddr.sin_addr.s_addr)// 有效地址
        {
          AddSingleRoute(pathTable[pathIndex-1].addrSet[j],24,tempNextHopAndWeight);
        }
        else break;
      }
    }
  }
  // PrintPathTable();
  return true;
}

bool 
Primus::UpdatePathTable(struct link tempLink)
{
  ident high,low;
  if (tempLink.identA.level>tempLink.identB.level)
  {
    high=tempLink.identA;
    low=tempLink.identB;
  }
  else if (tempLink.identA.level<tempLink.identB.level)
  {
    high=tempLink.identB;
    low=tempLink.identA;
  }

  int affectedNextHopIndex=0;
  int startIndex=0;

  switch (m_Ident.level)
  {
  case 1:// tor
    // the number of path
    // 5 hop: m_SpineNodes*(m_nPods-1)*m_ToRNodes
    // 4 hop: m_SpineNodes*(m_nPods-1)
    // 3 hop: m_LeafNodes*(m_ToRNodes-1)+m_SpineNodes
    // 2 hop: m_LeafNodes
    if (SameNode(low,m_Ident))// itself--leafnode
    {
      affectedNextHopIndex=high.position%m_LeafNodes;

      // update 5 hop (tornode-leafnode-spinenode-leafnode-tornode)
      startIndex=affectedNextHopIndex*(m_SpineNodes/m_LeafNodes);
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,m_SpineNodes/m_LeafNodes,(m_nPods-1)*m_ToRNodes,m_SpineNodes,tempLink.linkStatus);
      
      // update 4 hop (tornode-leafnode-spinenode-leafnode)
      startIndex=m_SpineNodes*(m_nPods-1)*m_ToRNodes+affectedNextHopIndex*((m_SpineNodes/m_LeafNodes)*(m_nPods-1));
      UpdateSinglePathBlock(startIndex,m_SpineNodes/m_LeafNodes,(m_nPods-1),m_SpineNodes/m_LeafNodes,tempLink.linkStatus);
      
      // update 3 hop
      // 1) tornode--leafnode--tornode
      startIndex=m_SpineNodes*(m_nPods-1)*(m_ToRNodes+1)+affectedNextHopIndex;
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,m_ToRNodes-1,m_LeafNodes,tempLink.linkStatus);
      // 2)tornode--leafnode--spinenode
      startIndex=m_SpineNodes*(m_nPods-1)*(m_ToRNodes+1)+m_LeafNodes*(m_ToRNodes-1)+affectedNextHopIndex*(m_SpineNodes/m_LeafNodes);
      UpdateSinglePathBlock(startIndex,1,m_SpineNodes/m_LeafNodes,1,tempLink.linkStatus);
      
      // update 2 hop
      startIndex=m_SpineNodes*(m_nPods-1)*(m_ToRNodes+1)+m_LeafNodes*(m_ToRNodes-1)+m_SpineNodes+affectedNextHopIndex;
      UpdateSinglePathBlock(startIndex,1,1,1,tempLink.linkStatus);
    }
    else if (high.level==2 && high.position/m_LeafNodes==m_Pod)// leafnode--tornode(in the same pod)
    {
      // 3 hop
      affectedNextHopIndex=high.position%m_LeafNodes;

      if (low.position<m_Ident.position)
        startIndex=m_SpineNodes*(m_nPods-1)*(m_ToRNodes+1)+(low.position%m_ToRNodes)*m_LeafNodes+affectedNextHopIndex;
      else
        startIndex=m_SpineNodes*(m_nPods-1)*(m_ToRNodes+1)+(low.position%m_ToRNodes-1)*m_LeafNodes+affectedNextHopIndex;

      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,1,m_LeafNodes,tempLink.linkStatus);
    }
    else if (high.level==2 && high.position/m_LeafNodes!=m_Pod)// leafnode--tornode(不在同一个pod内)
    {
      // 5 hop
      affectedNextHopIndex=high.position%m_LeafNodes;

      if (low.position<m_Ident.position)
        startIndex=low.position*m_SpineNodes+(high.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);
      else
        startIndex=(low.position-m_ToRNodes)*m_SpineNodes+(high.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes);

      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,m_SpineNodes/m_LeafNodes,1,m_SpineNodes,tempLink.linkStatus);
    }
    else if (high.level==3 && low.position/m_LeafNodes==m_Pod)// leafnode--spinenode(leafnode与m_ident在同一个pod内)
    {
      // 5 hop
      affectedNextHopIndex=low.position%m_LeafNodes;
      startIndex=high.position;
      
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,m_ToRNodes*(m_nPods-1),m_SpineNodes,tempLink.linkStatus);

      // 4 hop
      startIndex=m_SpineNodes*(m_nPods-1)*m_ToRNodes+affectedNextHopIndex*((m_SpineNodes/m_LeafNodes)*(m_nPods-1))+high.position%(m_SpineNodes/m_LeafNodes);
      UpdateSinglePathBlock(startIndex,1,m_nPods-1,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);

      // 3 hop(tor--leaf--spine)
      startIndex=m_SpineNodes*(m_nPods-1)*(m_ToRNodes+1)+(m_ToRNodes-1)*m_LeafNodes+high.position;
      UpdateSinglePathBlock(startIndex,1,1,1,tempLink.linkStatus);
    }
    else if (high.level==3 && low.position/m_LeafNodes!=m_Pod)// leafnode--spinenode(leafnode与m_ident不在同一个pod内)
    {
      // 5 hop
      affectedNextHopIndex=low.position%m_LeafNodes;
      if (low.position/m_LeafNodes<m_Pod)
        startIndex=(low.position/m_LeafNodes)*m_ToRNodes*m_SpineNodes+(low.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes)+high.position%(m_SpineNodes/m_LeafNodes);
      else
        startIndex=(low.position/m_LeafNodes-1)*m_ToRNodes*m_SpineNodes+(low.position%m_LeafNodes)*(m_SpineNodes/m_LeafNodes)+high.position%(m_SpineNodes/m_LeafNodes);

      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,m_ToRNodes,m_SpineNodes,tempLink.linkStatus);

      // 4 hop
      if (low.position/m_LeafNodes<m_Pod)
        startIndex=m_SpineNodes*(m_nPods-1)*m_ToRNodes+(low.position%m_LeafNodes)*(m_nPods-1)*(m_SpineNodes/m_LeafNodes)+(low.position/m_LeafNodes)*(m_SpineNodes/m_LeafNodes)+high.position%(m_SpineNodes/m_LeafNodes);
      else 
        startIndex=m_SpineNodes*(m_nPods-1)*m_ToRNodes+(low.position%m_LeafNodes)*(m_nPods-1)*(m_SpineNodes/m_LeafNodes)+(low.position/m_LeafNodes-1)*(m_SpineNodes/m_LeafNodes)+high.position%(m_SpineNodes/m_LeafNodes);
      UpdateSinglePathBlock(startIndex,1,1,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);
    }
    else fprintf(stderr,"Error affected type!\n");
    break;
  case 2:// leafnode
    // 4 hop: (m_nPods-1)*m_ToRNodes*(m_SpineNodes/m_LeafNodes)
    // 3 hop: (m_nPods-1)*(m_SpineNodes/m_LeafNodes)
    // 2 hop: m_ToRNodes+m_SpineNodes/m_LeafNodes
    if (SameNode(high,m_Ident))// itself--tornode
    {
      // 2 hop
      startIndex=(m_SpineNodes/m_LeafNodes)*(m_nPods-1)*(m_ToRNodes+1)+low.position%m_ToRNodes;
      UpdateSinglePathBlock(startIndex,1,1,1,tempLink.linkStatus);
    }
    else if (SameNode(low,m_Ident))// itself--spinenode
    {
      // 4 hop
      affectedNextHopIndex=high.position%(m_SpineNodes/m_LeafNodes);
      startIndex=high.position%(m_SpineNodes/m_LeafNodes);
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,(m_nPods-1)*m_ToRNodes,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);

      // 3 hop
      startIndex+=(m_nPods-1)*m_ToRNodes*(m_SpineNodes/m_LeafNodes);
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,m_nPods-1,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);

      // 2 hop
      startIndex=startIndex+(m_nPods-1)*(m_SpineNodes/m_LeafNodes)+m_ToRNodes;
      UpdateSinglePathBlock(startIndex,1,1,1,tempLink.linkStatus);
    }
    else if (high.level==3) // spinenode--leafnode
    {
      // 4 hop
      affectedNextHopIndex=high.position%(m_SpineNodes/m_LeafNodes);
      if (low.position/m_LeafNodes<m_Pod)
        startIndex=(low.position/m_LeafNodes)*(m_SpineNodes/m_LeafNodes)*m_ToRNodes+high.position%(m_SpineNodes/m_LeafNodes);
      else 
        startIndex=(low.position/m_LeafNodes-1)*(m_SpineNodes/m_LeafNodes)*m_ToRNodes+high.position%(m_SpineNodes/m_LeafNodes);
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,m_ToRNodes,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);

      // 3 hop
      if (low.position/m_LeafNodes<m_Pod)
        startIndex=(m_nPods-1)*m_ToRNodes*(m_SpineNodes/m_LeafNodes)+(low.position/m_LeafNodes)*(m_SpineNodes/m_LeafNodes)+high.position%(m_SpineNodes/m_LeafNodes);
      else 
        startIndex=(m_nPods-1)*m_ToRNodes*(m_SpineNodes/m_LeafNodes)+(low.position/m_LeafNodes-1)*(m_SpineNodes/m_LeafNodes)+high.position%(m_SpineNodes/m_LeafNodes);
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,1,1,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);
    }
    else if (high.level==2)// leafnode--tornode
    {
      // 4 hop
      affectedNextHopIndex=-1;
      if (low.position/m_ToRNodes<m_Pod)
        startIndex=low.position*(m_SpineNodes/m_LeafNodes);
      else 
        startIndex=(low.position-m_ToRNodes)*(m_SpineNodes/m_LeafNodes);
      UpdateMultiPathBlock(startIndex,affectedNextHopIndex,m_SpineNodes/m_LeafNodes,1,m_SpineNodes/m_LeafNodes,tempLink.linkStatus);
    }
    break;
  case 3:// spinenode
    // 3 hop: m_nPods*m_ToRNodes
    // 2 hop: m_nPods
    if (SameNode(high,m_Ident))//itself--leafnode
    {
      // 3 hop
      startIndex=(low.position/m_LeafNodes)*m_ToRNodes;
      UpdateSinglePathBlock(startIndex,1,m_ToRNodes,1,tempLink.linkStatus);

      // 2 hop
      startIndex=m_ToRNodes*m_nPods+low.position/m_LeafNodes;
      UpdateSinglePathBlock(startIndex,1,1,1,tempLink.linkStatus);
    }
    else if (high.position%m_LeafNodes==m_Ident.position/(m_SpineNodes/m_LeafNodes))// leafnode--tor
    {
      startIndex=low.position;
      UpdateSinglePathBlock(startIndex,1,1,1,tempLink.linkStatus);
    }
    
    break;
  default:
    break;
  }
  // PrintPathTable();
  return true;
}

bool 
Primus::RecvRS(struct link tempLink)
{
  int linkIndex=GetLinkIndex(tempLink.identA,tempLink.identB);

  pthread_mutex_lock(&LinkTableMutex);
  if (linkIndex!=-1 && linkTable[linkIndex].startStamp.tv_sec!=0
    && linkTable[linkIndex].startStamp.tv_usec!=0)
  {
    struct timeval endStamp;
    struct timeval startStamp;
    gettimeofday(&endStamp,NULL);
    startStamp=linkTable[linkIndex].startStamp;
    
    linkTable[linkIndex].startStamp.tv_sec=0;
    linkTable[linkIndex].startStamp.tv_usec=0;
    pthread_mutex_unlock(&LinkTableMutex);
    
    if (PRINT_NODE_RECV_RS_TIME)
    {
      stringstream logFoutPath;
      logFoutPath.str("");
      logFoutPath << COMMON_PATH << "CostTime.txt";
      ofstream Logfout(logFoutPath.str().c_str(),ios::app);
      
      // Logfout << "[" << tempLink.identA.level << "." << tempLink.identA.position << "--"
      // << tempLink.identB.level << "." << tempLink.identB.position << "/";
      // if (tempLink.linkStatus==true) Logfout << "UP";
      // else Logfout << "DOWN";
      // Logfout << "][eventID:" << tempLink.eventId << "][cost:" 
      // << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << " ms]." << endl;
      Logfout << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << endl;
      Logfout.close();
    }
  }
  else pthread_mutex_unlock(&LinkTableMutex);
}

bool 
Primus::UpdateNodeKeepAlive(ident nodeIdent)
{
  int nodeSockIndex=0;
  if (nodeIdent.level==1) nodeSockIndex=nodeIdent.position;
  else if (nodeIdent.level==2) nodeSockIndex=m_nPods*m_ToRNodes+nodeIdent.position;
  else if (nodeIdent.level==3) nodeSockIndex=m_nPods*(m_ToRNodes+m_LeafNodes)+nodeIdent.position;
  else if (nodeIdent.level==0) nodeSockIndex=m_nPods*(m_ToRNodes+m_LeafNodes)+m_SpineNodes+nodeIdent.position;

  if (SameNode(nodeSockTable[nodeSockIndex].nodeIdent,nodeIdent)
    && nodeSockTable[nodeSockIndex].unRecvNum>0)
  {
    nodeSockTable[nodeSockIndex].unRecvNum--;
    return true;
  }
  cout << "error UpdateNodeKeepAlive! nodeIdent[" << nodeIdent.level << "." << nodeIdent.position << "]." << endl;
  return false;
}

void* 
Primus::SlaveWaitRSThread(void* tempThreadParam)
{
  struct timeval startStamp,endStamp;
  gettimeofday(&startStamp,NULL);

  Primus *tempPrimus=((threadparama *)tempThreadParam)->tempPrimus;
  bool timeout=true;

  for (int i=1;i<=3;i++)
  {
    usleep(WAIT_INTERVAL*i);
    if (tempPrimus->m_RecvReElectReplyFromMaster==true)// 收到master的回复，没搞了
    {
      timeout=false;
      tempPrimus->m_RecvReElectReplyFromMaster=false;
      break;
    }
  }
  if (timeout==true && tempPrimus->m_Role==3 && tempPrimus->m_Ident.position==1)// 强行让0.1做master
  {
    cout << "I am the new master!" << endl;
    tempPrimus->m_Role=2;
    struct link tempLink={tempPrimus->tempIdent,tempPrimus->tempIdent,tempPrimus->tempAddr,tempPrimus->tempAddr,-1,false};
    struct message tempMessage={tempLink,tempPrimus->m_Ident,tempPrimus->tempIdent,tempPrimus->tempIdent,false,4,tempPrimus->m_Role,-1,1};
    for (int i=0;i<tempPrimus->nodeSockNum;i++)
    {
      if (tempPrimus->nodeSockTable[i].nodeSock!=-1 && !tempPrimus->SameNode(tempPrimus->nodeSockTable[i].nodeIdent,tempPrimus->tempIdent))
      {
        tempMessage.dstIdent=tempPrimus->nodeSockTable[i].nodeIdent;
        tempPrimus->SendMessageByTCP(tempPrimus->nodeSockTable[i].nodeSock,tempMessage);
      }
    }
    gettimeofday(&endStamp,NULL);
    cout << "Slave wait time is " << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << " ms.\n";
  }
  tempPrimus->m_RecvReElectFromNode=false;
}

void* 
Primus::RecvMessageThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparama *)tempThreadParam)->tempPrimus;
  int tempEpollFd=((threadparama *)tempThreadParam)->tempEpollFd;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << COMMON_PATH << "PacketType-" << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position << ".txt";
  ofstream Logfout(logFoutPath.str().c_str(),ios::trunc); 

  int ret=0;
  int sock=0;
  int recvSockNum=0;
  char recvBuf[MESSAGE_BUF_SIZE];
  struct timeval startStamp;
  struct timeval endStamp;

  while (1)
  {
    struct epoll_event events[MAX_SOCKNUM_PER_THREAD];
    recvSockNum=epoll_wait(tempEpollFd,events,MAX_SOCKNUM_PER_THREAD,-1);
    for (int i=0;i<recvSockNum;i++)
    {
      sock=events[i].data.fd;//The socks having coming data
      if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
      {
        fprintf(stderr,"Epoll event: Recv sock(%d) closed... Stop the recv thread.\n",sock);
        tempPrimus->DelSocket(sock);
        continue;
      }

      memset(recvBuf,'\0',sizeof(recvBuf));

      if ((ret=recv(sock,recvBuf,MESSAGE_BUF_SIZE,0))<=0)
      {
        cout << "RecvMessageThread ret(" << ret << ") sock(" << sock << ") recv error:" << strerror(errno) << "(errno:" << errno << ")" << endl;
        tempPrimus->DelSocket(sock);
        continue;
        // exit(1);
      }

      struct message tempMessage;
      memcpy(&tempMessage,recvBuf,sizeof(struct message));

      // if (tempMessage.messageType!=3) tempPrimus->PrintMessage(tempMessage);

      // dstIdent为本Node
      if (tempPrimus->SameNode(tempPrimus->m_Ident,tempMessage.dstIdent)
        || (tempPrimus->m_Ident.level==0 && tempPrimus->SameNode(tempPrimus->tempIdent,tempMessage.dstIdent)))
      {
        if (tempMessage.ack==true)// ack or response
        {
          switch (tempMessage.messageType)
          {
          case 1:// hello ack
            // cout << "Recv hello ack from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position << endl;
            tempPrimus->UpdateControllerSockTable(sock,tempMessage.srcIdent,tempMessage.srcIdentRole);
            tempPrimus->PrintMessage(tempMessage);
            break;
          case 2:// link status response
            if (tempPrimus->m_Role==2)// master
            {
              int messageEventQueueIndex=tempMessage.messageEventQueueIndex;

              pthread_mutex_lock(&(tempPrimus->MsgQueueEventMutex[messageEventQueueIndex]));
              tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].numOfSwitchesResponsed++;
              pthread_mutex_unlock(&(tempPrimus->MsgQueueEventMutex[messageEventQueueIndex]));
              // cout << "Master recv RS[" << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
              // << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
              // if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
              // else cout << "DOWN";
              // cout << " from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position
              // << "][recvRS:" << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].numOfSwitchesResponsed
              // << ",shouldNotify:" << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].numOfSwitchesShouldNotify
              // << "]." << endl;

              if (tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].numOfSwitchesResponsed==tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].numOfSwitchesShouldNotify)
              {
                if (PRINT_MASTER_RECV_ALLRS_TIME)
                {
                  // cout << "Master recv all RS[" << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
                  // << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
                  // if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
                  // else cout << "DOWN";
                  // cout << "]." << endl;
                  startStamp=tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].startStamp;
                }

                // tempPrimus->DequeueMessageFromEventQueue();

                tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.dstIdent=tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.srcIdent;
                tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.srcIdent=tempPrimus->m_Ident;
                tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.ack=true;

                int dstNodeSock=tempPrimus->GetNodeSock(tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.dstIdent);
                if (dstNodeSock>0 &&
                  tempPrimus->SendMessageByTCP(dstNodeSock,tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo));
                else 
                  cout << "Invaild nodeSock.";
                
                if (PRINT_MASTER_RECV_ALLRS_TIME)
                {
                  gettimeofday(&endStamp,NULL);

                  stringstream logFoutPath;
                  logFoutPath.str("");
                  logFoutPath << COMMON_PATH << "CostTime-" << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position << ".txt";
                  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

                  // Logfout << "[" << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.linkInfo.identA.level 
                  // << "." << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.linkInfo.identA.position << "--"
                  // << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.linkInfo.identB.level << "." 
                  // << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.linkInfo.identB.position << "/";
                  // if (tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.linkInfo.linkStatus==true) Logfout << "UP";
                  // else Logfout << "DOWN";
                  // Logfout << "][eventID:" << tempPrimus->messageEventQueue.eventQueue[messageEventQueueIndex].messageInfo.linkInfo.eventId 
                  // << "][cost:" << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << " ms]." << endl;
                  Logfout << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << endl;

                  Logfout.close();
                }
              } 
            }
            else if (tempPrimus->m_Role==1)
            {
              tempPrimus->RecvRS(tempMessage.linkInfo);// 收到response，更新链路表，计算时间开销
              // cout << "Recv RS " << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
              // << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
              // if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
              // else cout << "DOWN";
              // cout << endl;
              tempPrimus->PrintMessage(tempMessage);
            }
            break;
          case 3:// keepalive ack,node
            for (int j=0;j<MAX_CTRL_NUM;j++)
            {
              if (!tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempMessage.dstIdent)
                && tempPrimus->controllerSockTable[j].unRecvNum>0)
              {
                tempPrimus->controllerSockTable[j].unRecvNum--;
                // cout << "Recv keepalive ack from master " << tempMessage.srcIdent.level << "."
                // << tempMessage.srcIdent.position << endl;
              }
            }
            break;
          default:
            break;
          }
        }
        else // report
        {
          switch (tempMessage.messageType)
          {
          case 1:// hello report
            tempPrimus->AddNodeSock(tempMessage.srcIdent,sock);

            tempMessage.dstIdent=tempMessage.srcIdent;
            tempMessage.srcIdent=tempPrimus->m_Ident;
            tempMessage.ack=true;
            tempMessage.srcIdentRole=tempPrimus->m_Role;

            if ((ret=tempPrimus->SendMessageByTCP(sock,tempMessage))<=0) 
            {
              fprintf(stderr,"Send hello ack to %d.%d failed\n",tempMessage.dstIdent.level,tempMessage.dstIdent.position);
            }
            if (tempPrimus->m_Role==2)
            {
              // 向新连接的Node发送已有的LS
              tempMessage.ack=false;
              tempMessage.messageType=2;
              for (int i=0;i<tempPrimus->linkNum;i++)
              {
                // 此处的dstIdent是新连接的node
                if (tempMessage.dstIdent.level==1 || tempMessage.dstIdent.level==0);
                else if (tempMessage.dstIdent.level==2)
                {
                  if ((tempPrimus->linkTable[i].linkInfo.identA.level==2 && tempMessage.dstIdent.position%tempPrimus->m_LeafNodes==tempPrimus->linkTable[i].linkInfo.identA.position%tempPrimus->m_LeafNodes) 
                    || (tempPrimus->linkTable[i].linkInfo.identB.level==2 && tempMessage.dstIdent.position%tempPrimus->m_LeafNodes==tempPrimus->linkTable[i].linkInfo.identB.position%tempPrimus->m_LeafNodes));
                  else continue;
                }
                else if (tempMessage.dstIdent.level==3)
                {
                  if (tempPrimus->SameNode(tempMessage.dstIdent,tempPrimus->linkTable[i].linkInfo.identA) 
                    || tempPrimus->SameNode(tempMessage.dstIdent,tempPrimus->linkTable[i].linkInfo.identB));
                  else if ((tempPrimus->linkTable[i].linkInfo.identA.level==2) 
                    && (tempPrimus->linkTable[i].linkInfo.identA.position%tempPrimus->m_LeafNodes==tempMessage.dstIdent.position/(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)));
                  else continue;
                }
                
                
                if (tempPrimus->linkTable[i].linkInfo.addrA.sin_addr.s_addr!=tempPrimus->tempAddr.sin_addr.s_addr
                  && tempPrimus->linkTable[i].linkInfo.addrB.sin_addr.s_addr!=tempPrimus->tempAddr.sin_addr.s_addr)
                {
                  tempMessage.linkInfo.identA=tempPrimus->linkTable[i].linkInfo.identA;
                  tempMessage.linkInfo.identB=tempPrimus->linkTable[i].linkInfo.identB;
                  tempMessage.linkInfo.addrA=tempPrimus->linkTable[i].linkInfo.addrA;
                  tempMessage.linkInfo.addrB=tempPrimus->linkTable[i].linkInfo.addrB;
                  tempMessage.linkInfo.eventId=tempPrimus->linkTable[i].linkInfo.eventId;
                  tempMessage.linkInfo.linkStatus=tempPrimus->linkTable[i].linkInfo.linkStatus;
                  if ((ret=tempPrimus->SendMessageByTCP(sock,tempMessage))<=0) 
                  {
                    fprintf(stderr,"Send LS to new node[%d.%d] failed\n",tempMessage.dstIdent.level,tempMessage.dstIdent.position);
                  }
                }
              }
            }
            // cout << "send hello ack to sock(" << sock << ")." << endl;
            break;
          case 2:// link stauts report，2 type，controller recv or node recv
            if (tempPrimus->m_Role==1)// node recv
            {
              // static int recvNum=0;
              // cout << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position
              // << " recv[" << ++recvNum << "] " << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
              // << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
              // if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
              // else cout << "DONW";
              // cout << " from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position << endl;
              
              if (PRINT_NODE_MODIFY_TIME)
              {
                gettimeofday(&startStamp,NULL);
              }
              
              if (tempPrimus->UpdateLinkTable(tempMessage))//只处理链路状态变化
              {
                tempPrimus->UpdatePathTable(tempMessage.linkInfo);//处理成功
                tempMessage.dstIdent=tempMessage.srcIdent;
                tempMessage.srcIdent=tempPrimus->m_Ident;
                tempMessage.ack=true;

                tempPrimus->PrintMessage(tempMessage);
                tempPrimus->SendMessageByTCP(sock,tempMessage);// 向master返回response

                if (PRINT_NODE_MODIFY_TIME)
                {  
                  gettimeofday(&endStamp,NULL);
                  stringstream logFoutPath;
                  logFoutPath.str("");
                  logFoutPath << COMMON_PATH << "CostTime.txt";
                  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
                  
                  // Logfout << "[" << tempMessage.linkInfo.identA.level << "." 
                  // << tempMessage.linkInfo.identA.position << "--"
                  // << tempMessage.linkInfo.identB.level << "." 
                  // << tempMessage.linkInfo.identB.position << "/";
                  // if (tempMessage.linkInfo.linkStatus==true) Logfout << "UP";
                  // else Logfout << "DOWN";
                  // Logfout << "][eventID:" << tempMessage.linkInfo.eventId << "][cost:" 
                  // << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << " ms]." << endl;
                  Logfout << ((endStamp.tv_sec-startStamp.tv_sec)*1000+(endStamp.tv_usec-startStamp.tv_usec)*0.001) << endl;

                  Logfout.close();
                }
              }
              else 
              {
                // cout << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position 
                // << " don't need to UpdateLinkTable." << endl;
              }
            }
            else if (tempPrimus->m_Role==2)// master recv
            {
              if (tempPrimus->UpdateLinkTable(tempMessage))
              {
                tempPrimus->EnqueueMessageIntoEventQueue(tempMessage);
                Logfout << "Recv tcp packets:" << tempPrimus->recvTcpNum << "\nRecv udp packets:" << tempPrimus->recvUdpNum << endl;
                tempPrimus->PrintMessage(tempMessage);
              }
              else 
              {
                tempPrimus->PrintMessage(tempMessage);
                tempMessage.dstIdent=tempMessage.srcIdent;
                tempMessage.srcIdent=tempPrimus->m_Ident;
                tempMessage.ack=true;

                if (tempPrimus->SendMessageByTCP(sock,tempMessage));
                else cout << "Invaild nodeSock.";
              }
            }
            else if (tempPrimus->m_Role==3)// slave recv
            {
              if (tempMessage.srcIdent.level==0)// master 发来的同步信息
              {
                tempPrimus->UpdateLinkTable(tempMessage);
              }
              tempPrimus->PrintMessage(tempMessage);
            }
            break;
          case 3:// recv keepalive report
            // cout << "Recv keepalive from node " << tempMessage.srcIdent.level << "."
            // << tempMessage.srcIdent.position << endl;
            if (tempPrimus->UpdateNodeKeepAlive(tempMessage.srcIdent))
            {
              tempMessage.dstIdent=tempMessage.srcIdent;
              tempMessage.srcIdent=tempPrimus->m_Ident;
              tempMessage.ack=true;
              tempPrimus->SendMessageByTCP(sock,tempMessage);
              // cout << "Send keepalive ack to node " << tempMessage.dstIdent.level << "."
              // << tempMessage.dstIdent.position << endl;
            }
            break;
          case 4:// reelect
            if (tempPrimus->m_Role==1)// node recv
            {
              cout << "Recv RE from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position << "." << endl;
              for (int j=0;j<MAX_CTRL_NUM;j++)
              {
                if (tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempPrimus->tempIdent)) break; // 遍历完表未找到直接添加
                if (tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempMessage.srcIdent) 
                  && tempPrimus->controllerSockTable[j].controllerRole==3)// 将slave替换为master
                {
                  tempPrimus->controllerSockTable[j].controllerRole=2;
                }
                else if (!tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempMessage.srcIdent)
                  && tempPrimus->controllerSockTable[j].controllerRole==2)// 将原来的master替换成slave
                {
                  tempPrimus->controllerSockTable[j].controllerRole=3;
                }
              }
              // tempPrimus->PrintControllerSockTable();
              tempPrimus->PrintMessage(tempMessage);
            }
            else if (tempPrimus->m_Role==2)// master recv
            {
              // 
            }
            else if (tempPrimus->m_Role==3)// slave recv
            {
              if (tempMessage.srcIdent.level==0)// recv from controller
              {
                // 
              }
              else// recv from node
              {
                if (tempPrimus->m_RecvReElectFromNode==false)// 未收到请求
                {
                  tempPrimus->PrintMessage(tempMessage);
                  for (int j=0;j<MAX_CTRL_NUM;j++)
                  {
                    if (tempPrimus->controllerSockTable[j].controllerSock!=-1 
                      && tempPrimus->controllerSockTable[j].controllerRole==2
                      && !tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempPrimus->tempIdent))
                    {
                      tempMessage.dstIdent=tempPrimus->controllerSockTable[j].controllerIdent;
                      tempMessage.srcIdent=tempPrimus->m_Ident;
                      tempMessage.srcIdentRole=tempPrimus->m_Role;

                      tempPrimus->SendMessageByTCP(tempPrimus->controllerSockTable[j].controllerSock,tempMessage);

                      pthread_t SlaveWaitRSThreadID;
                      threadparama *tempThreadParam=new threadparama();
                      tempThreadParam->tempPrimus=tempPrimus;
                      tempThreadParam->tempEpollFd=-1;

                      if (pthread_create(&SlaveWaitRSThreadID,NULL,SlaveWaitRSThread,(void*)tempThreadParam)!=0)
                        cout << "Create SlaveWaitRSThread failed." << endl;
                      else 
                      {
                        tempPrimus->m_RecvReElectFromNode=true;
                      }
                      break;
                    }
                  }
                }
              }
            }
            break;
          default:
            break;
          }
        }
      }
      else// 转发
      {
        if (tempPrimus->udpSock!=sock && tempMessage.messageType==1 && tempMessage.ack==false && tempPrimus->m_Ident.level!=0)
        {
          tempPrimus->AddNodeSock(tempMessage.srcIdent,sock);// node作为转发节点添加套接字，应该是tcp
        }
        if (tempMessage.dstIdent.level==-1 || tempMessage.dstIdent.position==-1)// 转发给所有的controller
        {
          for (int j=0;j<MAX_CTRL_NUM;j++)
          {
            if (tempPrimus->controllerSockTable[j].controllerSock==-1
              || tempPrimus->SameNode(tempPrimus->tempIdent,tempPrimus->controllerSockTable[j].controllerIdent)) break;
            tempPrimus->SendMessageByTCP(tempPrimus->controllerSockTable[j].controllerSock,
                                         tempMessage);
            // cout << "Forward message from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position
            // << " to " << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << endl;
          }
        }
        else if (tempMessage.dstIdent.level==0)// 转发给某个特定的controller
        {
          for (int j=0;j<MAX_CTRL_NUM;j++)
          {
            if (tempPrimus->SameNode(tempPrimus->controllerSockTable[j].controllerIdent,tempMessage.dstIdent))
            {
              if (tempPrimus->controllerSockTable[j].controllerSock>0)
              {
                tempPrimus->SendMessageByTCP(tempPrimus->controllerSockTable[j].controllerSock,
                                         tempMessage);
                // cout << "Forward message from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position
                // << " to " << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << endl;
              }
              break;
            }
          }
        }
        else if (tempMessage.dstIdent.level!=0)//转发给某个特定的Node
        {
          int tempNodeSock=tempPrimus->GetNodeSock(tempMessage.dstIdent);
          if (tempNodeSock>0)
          {
            tempPrimus->SendMessageByTCP(tempNodeSock,tempMessage);
            // cout << "Forward message from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position
            // << " to " << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << endl;             
          }
          else // 通过udp转发
          {
            cout << "start to search path" << endl;
            int pathIndex=0;
            ident tempNextHopIdent=tempPrimus->tempIdent;
            sockaddr_in tempDstAddr=tempPrimus->tempAddr;
            if (tempPrimus->m_Ident.level==1)
            {
              if (tempMessage.dstIdent.level==1 && tempMessage.dstIdent.position%tempPrimus->m_ToRNodes!=tempPrimus->m_Pod)// 不在同一个pod内
              {
                if (tempMessage.dstIdent.position<tempPrimus->m_Ident.position)
                  pathIndex=tempMessage.dstIdent.position*tempPrimus->m_SpineNodes;
                else 
                  pathIndex=(tempMessage.dstIdent.position-tempPrimus->m_ToRNodes)*tempPrimus->m_SpineNodes;
                for (int j=0;j<tempPrimus->m_SpineNodes;j++)
                {
                  if (tempPrimus->pathTable[pathIndex+j].faultLinkCounter==0)
                  {
                    tempNextHopIdent=tempPrimus->pathTable[pathIndex+j].pathNodeIdent[1];
                    tempDstAddr=tempPrimus->pathTable[pathIndex+j].addrSet[0];
                    break;
                  }
                }
              }
              else if (tempMessage.dstIdent.level==1 && tempMessage.dstIdent.position%tempPrimus->m_ToRNodes==tempPrimus->m_Pod)// 在同一个pod内
              {
                if (tempMessage.dstIdent.position<tempPrimus->m_Ident.position)
                  pathIndex=tempPrimus->m_SpineNodes*(tempPrimus->m_nPods-1)*(tempPrimus->m_ToRNodes+1)+tempMessage.dstIdent.position*tempPrimus->m_SpineNodes;
                else 
                  pathIndex=tempPrimus->m_SpineNodes*(tempPrimus->m_nPods-1)*(tempPrimus->m_ToRNodes+1)+(tempMessage.dstIdent.position-1)*tempPrimus->m_SpineNodes;
                for (int j=0;j<tempPrimus->m_LeafNodes;j++)
                {
                  if (tempPrimus->pathTable[pathIndex+j].faultLinkCounter==0)
                  {
                    tempNextHopIdent=tempPrimus->pathTable[pathIndex+j].pathNodeIdent[1];
                    tempDstAddr=tempPrimus->pathTable[pathIndex+j].addrSet[0];
                    break;
                  }
                }
              }
              else if (tempMessage.dstIdent.level==2 && tempMessage.dstIdent.position%tempPrimus->m_LeafNodes!=tempPrimus->m_Pod)// 不在同一个pod内
              {
                if (tempMessage.dstIdent.position/tempPrimus->m_LeafNodes<tempPrimus->m_Pod)
                  pathIndex=tempPrimus->m_SpineNodes*(tempPrimus->m_nPods-1)*tempPrimus->m_ToRNodes+(tempMessage.dstIdent.position%tempPrimus->m_LeafNodes)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)*(tempPrimus->m_nPods-1)+(tempMessage.dstIdent.position/tempPrimus->m_LeafNodes)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                else
                  pathIndex=tempPrimus->m_SpineNodes*(tempPrimus->m_nPods-1)*tempPrimus->m_ToRNodes+(tempMessage.dstIdent.position%tempPrimus->m_LeafNodes)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)*(tempPrimus->m_nPods-1)+(tempMessage.dstIdent.position/tempPrimus->m_LeafNodes-1)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                for (int j=0;j<tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes;j++)
                {
                  if (tempPrimus->pathTable[pathIndex+j].faultLinkCounter==0)
                  {
                    tempNextHopIdent=tempPrimus->pathTable[pathIndex+j].pathNodeIdent[1];
                    tempDstAddr=tempPrimus->pathTable[pathIndex+j].addrSet[0];
                    break;
                  }
                }
              }
              else if (tempMessage.dstIdent.level==2 && tempMessage.dstIdent.position%tempPrimus->m_LeafNodes==tempPrimus->m_Pod)// 在同一个pod内
              {
                pathIndex=tempPrimus->m_SpineNodes*(tempPrimus->m_nPods-1)*(tempPrimus->m_ToRNodes+1)+tempPrimus->m_LeafNodes*(tempPrimus->m_ToRNodes-1)+tempPrimus->m_SpineNodes+tempMessage.dstIdent.position%tempPrimus->m_LeafNodes;
                for (int j=0;j<tempPrimus->m_LeafNodes;j++)
                {
                  if (tempPrimus->pathTable[pathIndex+j].faultLinkCounter==0)
                  {
                    tempNextHopIdent=tempPrimus->pathTable[pathIndex+j].pathNodeIdent[1];
                    tempDstAddr=tempPrimus->pathTable[pathIndex+j].addrSet[0];
                    break;
                  }
                }
              }
              else if (tempMessage.dstIdent.level==3)
              {
                pathIndex=tempPrimus->m_SpineNodes*(tempPrimus->m_nPods-1)*(tempPrimus->m_ToRNodes+1)+(tempPrimus->m_ToRNodes-1)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)+tempMessage.dstIdent.position;
                if (tempPrimus->pathTable[pathIndex].faultLinkCounter==0)
                {
                  tempNextHopIdent=tempPrimus->pathTable[pathIndex].pathNodeIdent[1];
                  tempDstAddr=tempPrimus->pathTable[pathIndex].addrSet[0];
                }
              }
            }
            else if (tempPrimus->m_Ident.level==2)
            {
              if (tempMessage.dstIdent.level==1 && tempMessage.dstIdent.position/tempPrimus->m_ToRNodes!=tempPrimus->m_Pod)// 不在同一个pod内
              {
                if (tempMessage.dstIdent.position/tempPrimus->m_ToRNodes<tempPrimus->m_Pod)
                  pathIndex=tempMessage.dstIdent.position*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                else if (tempMessage.dstIdent.position/tempPrimus->m_ToRNodes>tempPrimus->m_Pod)
                  pathIndex=(tempMessage.dstIdent.position-tempPrimus->m_ToRNodes)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                for (int j=0;j<tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes;j++)
                {
                  if (tempPrimus->pathTable[pathIndex+j].faultLinkCounter==0)
                  {
                    tempNextHopIdent=tempPrimus->pathTable[pathIndex+j].pathNodeIdent[1];
                    tempDstAddr=tempPrimus->pathTable[pathIndex+j].addrSet[0];
                    break;
                  }
                }
              }
              else if (tempMessage.dstIdent.level==1 && tempMessage.dstIdent.position/tempPrimus->m_ToRNodes==tempPrimus->m_Pod)// 在同一个pod内
              {
                pathIndex=(tempPrimus->m_nPods-1)*(tempPrimus->m_ToRNodes+1)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)+tempMessage.dstIdent.position%tempPrimus->m_ToRNodes;
                if (tempPrimus->pathTable[pathIndex].faultLinkCounter==0)
                {
                  tempNextHopIdent=tempPrimus->pathTable[pathIndex].pathNodeIdent[1];
                  tempDstAddr=tempPrimus->pathTable[pathIndex].addrSet[0];
                }
              }
              else if (tempMessage.dstIdent.level==2)
              {
                if (tempMessage.dstIdent.position/tempPrimus->m_LeafNodes<tempPrimus->m_Pod)
                  pathIndex=(tempPrimus->m_nPods-1)*tempPrimus->m_ToRNodes*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)+(tempMessage.dstIdent.position/tempPrimus->m_LeafNodes)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                else
                  pathIndex=(tempPrimus->m_nPods-1)*tempPrimus->m_ToRNodes*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)+(tempMessage.dstIdent.position/tempPrimus->m_LeafNodes-1)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                for (int j=0;j<tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes;j++)
                {
                  if (tempPrimus->pathTable[pathIndex+j].faultLinkCounter==0)
                  {
                    tempNextHopIdent=tempPrimus->pathTable[pathIndex+j].pathNodeIdent[1];
                    tempDstAddr=tempPrimus->pathTable[pathIndex+j].addrSet[0];
                    break;
                  }
                }
              }
              else if (tempMessage.dstIdent.level==3)
              {
                pathIndex=(tempPrimus->m_nPods-1)*(tempPrimus->m_ToRNodes+1)*(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes)+tempMessage.dstIdent.position%(tempPrimus->m_SpineNodes/tempPrimus->m_LeafNodes);
                if (tempPrimus->pathTable[pathIndex].faultLinkCounter==0)
                {
                  tempNextHopIdent=tempPrimus->pathTable[pathIndex].pathNodeIdent[1];
                  tempDstAddr=tempPrimus->pathTable[pathIndex].addrSet[0];
                }
              }
            }
            else if (tempPrimus->m_Ident.level==3)
            {
              if (tempMessage.dstIdent.level==1)
              {
                pathIndex=tempMessage.dstIdent.position;
                if (tempPrimus->pathTable[pathIndex].faultLinkCounter==0)
                {
                  tempNextHopIdent=tempPrimus->pathTable[pathIndex].pathNodeIdent[1];
                  tempDstAddr=tempPrimus->pathTable[pathIndex].addrSet[0];
                }
              }
              else if (tempMessage.dstIdent.level==2)
              {
                pathIndex=tempPrimus->m_nPods*tempPrimus->m_ToRNodes+tempMessage.dstIdent.position/tempPrimus->m_LeafNodes;
                if (tempPrimus->pathTable[pathIndex].faultLinkCounter==0)
                {
                  tempNextHopIdent=tempPrimus->pathTable[pathIndex].pathNodeIdent[1];
                  tempDstAddr=tempPrimus->pathTable[pathIndex].addrSet[0];
                }
              }
            }
            cout << "search path completely" << endl;
            if (!tempPrimus->SameNode(tempNextHopIdent,tempPrimus->tempIdent) && tempDstAddr.sin_addr.s_addr!=tempPrimus->tempAddr.sin_addr.s_addr)
            {
              cout << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position << " forward message["; 
              if (tempMessage.transportType==1) Logfout << "TCP][";
              else Logfout << "UDP][";
              if (tempPrimus->SameNode(tempMessage.fowIdent,tempPrimus->tempIdent)) Logfout << "Direct][";
              else Logfout << "InDirect][";
              if (tempMessage.messageType==1) cout << "HL";
              else if (tempMessage.messageType==2) cout << "LS";
              else if (tempMessage.messageType==3) cout << "KA";
              else if (tempMessage.messageType==4) cout << "RE";

              if (tempMessage.ack==true) cout << ":RS";
              else cout << ":RP";
              cout << ":" << tempMessage.linkInfo.eventId << "][src:" << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position;
              if (!tempPrimus->SameNode(tempMessage.fowIdent,tempPrimus->tempIdent)) cout << ",fow:" << tempMessage.fowIdent.level << "." << tempMessage.fowIdent.position;
              cout << ",dst:" << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << "]";

              if (tempMessage.messageType==2)
              {
                cout << "[" << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
                << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position;
                if (tempMessage.linkInfo.linkStatus==true) cout << "/UP]";
                else cout << "/DOWN]";
              } 
              // cout << ".\n"; 
              
              sockaddr_in tempLocalAddr=tempPrimus->GetLocalAddrByNeighborIdent(tempNextHopIdent);
              sockaddr_in tempGateAddr=tempPrimus->GetGateAddrByNeighborIdent(tempNextHopIdent);
              if (tempLocalAddr.sin_addr.s_addr!=tempPrimus->tempAddr.sin_addr.s_addr && tempGateAddr.sin_addr.s_addr!=tempPrimus->tempAddr.sin_addr.s_addr)
                tempPrimus->SendMessageByUDP(tempLocalAddr,tempGateAddr,tempMessage);
              cout << " completely!" << endl;
            }
            else
            {
              // cout << tempPrimus->m_Ident.level << "." << tempPrimus->m_Ident.position << " can't forward message[";
              // if (tempMessage.transportType==1) Logfout << "TCP][";
              // else Logfout << "UDP][";
              // if (tempPrimus->SameNode(tempMessage.fowIdent,tempPrimus->tempIdent)) Logfout << "Direct][";
              // else Logfout << "InDirect][";
              // if (tempMessage.messageType==1) cout << "HL";
              // else if (tempMessage.messageType==2) cout << "LS";
              // else if (tempMessage.messageType==3) cout << "KA";
              // else if (tempMessage.messageType==4) cout << "RE";

              // if (tempMessage.ack==true) cout << ":RS";
              // else cout << ":RP";
              // cout << ":" << tempMessage.linkInfo.eventId << "][src:" << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position;
              // if (!tempPrimus->SameNode(tempMessage.fowIdent,tempPrimus->tempIdent)) cout << ",fow:" << tempMessage.fowIdent.level << "." << tempMessage.fowIdent.position;
              // cout << ",dst:" << tempMessage.dstIdent.level << "." << tempMessage.dstIdent.position << "]";

              // if (tempMessage.messageType==2)
              // {
              //   cout << "[" << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position
              //   << "--" << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position;
              //   if (tempMessage.linkInfo.linkStatus==true) cout << "/UP]";
              //   else cout << "/DOWN]";
              // } 
              // cout << ".\n";           
            }
          }
        }
      }
    }
  }
}

int 
Primus::SendToAllAffectedNodes(struct message tempMessage,int tempStartIndex,int tempEndIndex)// 在此处确定下发范围规则
{
  int tempIndex=0;//链路中的leafnode在pod中的相对位置
  int nodeIndex=0;
  int numOfSentNodes=0;
  int ret=0;
  tempMessage.srcIdent=m_Ident;
  tempMessage.srcIdentRole=m_Role;

  // // link type:1)spinenode--leafnode;2)leafnode--tornode;
  cout << endl << endl << "SendToAllAffectedNodes [" << tempStartIndex << " to " << tempEndIndex << "]["
  << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position << "--"
  << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
  if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
  else cout << "DOWN";
  cout << "][eventId:" << tempMessage.linkInfo.eventId << "]." << endl;

  if (tempMessage.linkInfo.identA.level==2) tempIndex=tempMessage.linkInfo.identA.position%m_LeafNodes;
  else if (tempMessage.linkInfo.identB.level==2) tempIndex=tempMessage.linkInfo.identB.position%m_LeafNodes;
  else 
  {
    cout << "SendToAllAffectedNodes don't find tempIndex." << endl;
    return numOfSentNodes;
  }

  // 无论链路类型是哪种，tor和leafnode的下发规则是相同的
  srand((unsigned)time(NULL));
  for (int i=tempStartIndex;i<tempEndIndex && i<nodeSockNum;i++)
  {
    if (!SameNode(nodeSockTable[i].nodeIdent,tempIdent))
    {
      tempMessage.dstIdent=nodeSockTable[i].nodeIdent;
      if (nodeSockTable[i].nodeSock!=-1)
      {
        if (nodeSockTable[i].nodeIdent.level==0)// slave
        {
          SendMessageByTCP(nodeSockTable[i].nodeSock,tempMessage);
        }
        else if (nodeSockTable[i].nodeIdent.level==1)// tor，无脑发
        {
          // cout << "Send to " << nodeSockTable[i].nodeIdent.level << "." << nodeSockTable[i].nodeIdent.position 
          // << "[sock:" << nodeSockTable[i].nodeSock << "]";
          if ((ret=SendMessageByTCP(nodeSockTable[i].nodeSock,tempMessage))==MESSAGE_BUF_SIZE)// 发送成功
          {
            numOfSentNodes++;
          }
          // cout << "[ret:" << ret << "]." << endl;
        }
        else if (nodeSockTable[i].nodeIdent.level==2 
          && nodeSockTable[i].nodeIdent.position%m_LeafNodes==tempIndex)// leafnode，只有相对位置相同才发送
        {
          // cout << "Send to " << nodeSockTable[i].nodeIdent.level << "." << nodeSockTable[i].nodeIdent.position 
          // << "[sock:" << nodeSockTable[i].nodeSock << "]";
          if ((ret=SendMessageByTCP(nodeSockTable[i].nodeSock,tempMessage))==MESSAGE_BUF_SIZE)// 发送成功
          {
            numOfSentNodes++;
          }
          // cout << "[ret:" << ret << "]." << endl;
        }
        else if (nodeSockTable[i].nodeIdent.level==3
          && ((SameNode(tempMessage.linkInfo.identA,nodeSockTable[i].nodeIdent)) || (SameNode(tempMessage.linkInfo.identB,nodeSockTable[i].nodeIdent))))
        {
          // cout << "Send to " << nodeSockTable[i].nodeIdent.level << "." << nodeSockTable[i].nodeIdent.position 
          // << "[sock:" << nodeSockTable[i].nodeSock << "]";
          if ((ret=SendMessageByTCP(nodeSockTable[i].nodeSock,tempMessage))==MESSAGE_BUF_SIZE)// 发送成功
          {
            numOfSentNodes++;
          }
          // cout << "[ret:" << ret << "]." << endl;
        }
        else if (nodeSockTable[i].nodeIdent.level==3)
        {
          if (tempMessage.linkInfo.identA.level==3 || tempMessage.linkInfo.identB.level==3)//spinenode--leafnode;
          {
            if ((SameNode(tempMessage.linkInfo.identA,nodeSockTable[i].nodeIdent)) || (SameNode(tempMessage.linkInfo.identB,nodeSockTable[i].nodeIdent)))
            {
              // cout << "Send to " << nodeSockTable[i].nodeIdent.level << "." << nodeSockTable[i].nodeIdent.position 
              // << "[sock:" << nodeSockTable[i].nodeSock << "]";
              if ((ret=SendMessageByTCP(nodeSockTable[i].nodeSock,tempMessage))==MESSAGE_BUF_SIZE)// 发送成功
              {
                numOfSentNodes++;
              }
              // cout << "[ret:" << ret << "]." << endl;
            }
          }
          else if (tempMessage.linkInfo.identA.level==1 || tempMessage.linkInfo.identB.level==1)//leafnode--tornode
          {
            if (nodeSockTable[i].nodeIdent.position/(m_SpineNodes/m_LeafNodes)==tempIndex)
            {
              // cout << "Send to " << nodeSockTable[i].nodeIdent.level << "." << nodeSockTable[i].nodeIdent.position 
              // << "[sock:" << nodeSockTable[i].nodeSock << "]";
              if ((ret=SendMessageByTCP(nodeSockTable[i].nodeSock,tempMessage))==MESSAGE_BUF_SIZE)// 发送成功
              {
                numOfSentNodes++;
              }
              // cout << "[ret:" << ret << "]." << endl;
            }
          }
        }
      }
      for (int j=0;j<MAX_FOWNODE_NUM;j++)
      {
        nodeIndex=rand()%nodeSockNum;
        if (nodeSockTable[nodeIndex].nodeIdent.level!=0 && (i!=nodeIndex) && nodeSockTable[nodeIndex].nodeSock!=-1)
        {
          SendMessageByTCP(nodeSockTable[nodeIndex].nodeSock,tempMessage);
        }
      }
    }
  }
  cout << "SendToAllAffectedNodes completely.\n";
  return numOfSentNodes;
}

void* 
Primus::SendMessageThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparamb *)tempThreadParam)->tempPrimus;
  int tempStartIndex=((threadparamb *)tempThreadParam)->startIndex;
  int tempEndIndex=tempStartIndex+MAX_SOCKNUM_PER_THREAD;
  int sendQueueLen=0;
  int sendQueueHead=0;
  int sendQueueTail=0;
  int thisSendQueueHead=0;
  int thisEventNumOfNodesSentByThisThread=0;
  unsigned int sendQueueCounterIn=0;
  unsigned int numOfEventsSentByThisThread=0;

  while (1)
  {
    pthread_mutex_lock(&(tempPrimus->MsgQueueMutex));
    sendQueueLen=tempPrimus->messageEventQueue.len;
    sendQueueHead=tempPrimus->messageEventQueue.head;
    sendQueueTail=tempPrimus->messageEventQueue.tail;
    sendQueueCounterIn=tempPrimus->messageEventQueue.counterIn;
    pthread_mutex_unlock(&(tempPrimus->MsgQueueMutex));

    if (sendQueueLen<0 || sendQueueLen>MAX_OUTBOUNDING_EVENTS)
    {
      fprintf(stderr, "ERROR! sendQueueLen %d\n", sendQueueLen);
      exit(1);
    }

    if (sendQueueLen==0 || numOfEventsSentByThisThread==sendQueueCounterIn || (sendQueueTail+1)%MAX_OUTBOUNDING_EVENTS<=thisSendQueueHead)
    {
      // sched_yield();
      usleep(100);
      continue;
    }

    for (int i=0;i<(sendQueueTail+1)%MAX_OUTBOUNDING_EVENTS-thisSendQueueHead;i++)
    {
      pthread_mutex_lock(&(tempPrimus->MsgQueueEventMutex[thisSendQueueHead]));
      thisEventNumOfNodesSentByThisThread=tempPrimus->SendToAllAffectedNodes(
        tempPrimus->messageEventQueue.eventQueue[thisSendQueueHead].messageInfo,
        tempStartIndex,
        tempEndIndex);
      tempPrimus->messageEventQueue.eventQueue[thisSendQueueHead].numOfSwitchesSent+=thisEventNumOfNodesSentByThisThread;
      pthread_mutex_unlock(&(tempPrimus->MsgQueueEventMutex[thisSendQueueHead]));

      // cout << "Send to " << thisEventNumOfNodesSentByThisThread << " affected nodes." << endl;

      thisSendQueueHead=(thisSendQueueHead+1)%MAX_OUTBOUNDING_EVENTS;
      numOfEventsSentByThisThread++;
    }
  }
}

bool 
Primus::AddNodeSock(ident nodeIdent,int nodeSock)
{
  int nodeSockIndex=0;
  if (nodeIdent.level==1) nodeSockIndex=nodeIdent.position;
  else if (nodeIdent.level==2) nodeSockIndex=m_nPods*m_ToRNodes+nodeIdent.position;
  else if (nodeIdent.level==3) nodeSockIndex=m_nPods*(m_ToRNodes+m_LeafNodes)+nodeIdent.position;
  else if (nodeIdent.level==0) nodeSockIndex=m_nPods*(m_ToRNodes+m_LeafNodes)+m_SpineNodes+nodeIdent.position;

  if (SameNode(nodeSockTable[nodeSockIndex].nodeIdent,nodeIdent))
  {
    nodeSockTable[nodeSockIndex].nodeSock=nodeSock;
    nodeSockTable[nodeSockIndex].unRecvNum=0;
    // PrintNodeSockTable();
    return true;
  }
  else
  {
    fprintf(stderr,
      "AddNodeSock error!(nodeSockTable[%d]:%d.%d,nodeIdent:%d.%d).\n",
      nodeSockIndex,
      nodeSockTable[nodeSockIndex].nodeIdent.level,
      nodeSockTable[nodeSockIndex].nodeIdent.position,
      nodeIdent.level,
      nodeIdent.position);
    return false;
  }
}

bool 
Primus::ConnectWithMaster(string masterIP,string NICName)
{
  struct sockaddr_in masterAddr;
  memset(&masterAddr,0,sizeof(masterAddr)); //数据初始化--清零
  masterAddr.sin_family=AF_INET; //设置为IP通信
  masterAddr.sin_addr.s_addr=inet_addr(masterIP.c_str());
  masterAddr.sin_port=htons(PRIMUS_LISTEN_PORT);

  int nodeSock=0;
  int value=1;


  if ((nodeSock=socket(PF_INET,SOCK_STREAM,0))<0)
  {
    fprintf(stderr,"TCPRoute Create Socket Failed.\n");
    exit(1);
  }

  if (setsockopt(nodeSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)//设置端口复用
  {
    fprintf(stderr,"Set SO_REUSEPORT error.\n");
    exit(1);
  }

  if (setsockopt(nodeSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    fprintf(stderr,"Set SO_REUSEADDR error.\n");
    exit(1);
  }

  if (setsockopt(nodeSock, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
  {
    fprintf(stderr,"Set TCP_NODELAY error.\n");
    exit(1);
  }

  int nRecvBuf=TCP_BUF_SIZE;
  setsockopt(nodeSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
  // 发送缓冲区
  int nSendBuf=TCP_BUF_SIZE;
  setsockopt(nodeSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(nodeSock, (struct sockaddr *)&sin, &len) == -1)
  {
    perror("getsockname");
  }

  // struct ifreq ifr;
  // memset(&ifr,0x00,sizeof(ifr));
  // strncpy(ifr.ifr_name,NICName.c_str(),strlen(NICName.c_str()));
  // if (setsockopt(nodeSock,SOL_SOCKET,SO_BINDTODEVICE,(char *)&ifr,sizeof(ifr))<0)
  // {
  //   perror("TCPRoute Binding error.");
  // }

  while(1)
  {
    if ((connect(nodeSock,(const struct sockaddr *)&masterAddr,sizeof(masterAddr)))==0)
    {
      // printf("[%d.%d] Sock connected to master (%s:%d).\n"
      //   ,m_Ident.level,m_Ident.position
      //   ,inet_ntoa(masterAddr.sin_addr),ntohs(masterAddr.sin_port));
      // fflush(stdout);
      break;
    }
    else {
      cout << "Sock(" << nodeSock << ") connect error:" << strerror(errno) << "(errno:" << errno << ")." << endl;
      usleep(CONNECT_INTERVAL);
    }
  }
  
  AddSocketToEpoll(nodeSock);

  struct link tempLink={tempIdent,tempIdent,tempAddr,tempAddr,0,true};// 向master发送hello时，链路信息无效
  struct message tempMessage={tempLink,m_Ident,tempIdent,tempIdent,false,1,m_Role,-1,1};

  int ret=SendMessageByTCP(nodeSock,tempMessage);

  if (ret<0) 
  {
    printf("Send hello to master(%s) failed!\n",masterIP.c_str());
    exit(1);
  }

  // fprintf(stderr,"Send hello to master(%s) ret(%d) success.\n",masterIP.c_str(),ret);
  printf("[%d.%d] connected to master!\n",m_Ident.level,m_Ident.position);
  return true;
}

void 
Primus::InitiateMessageEventQueues()
{
  messageEventQueue.head=0;//First element is head
  messageEventQueue.tail=0;//Last element is tail-1
  messageEventQueue.len=0;
  messageEventQueue.counterIn=0;//How many events have ever been pushed in
  messageEventQueue.counterOut=0;//How many events have ever been popped out

  for (int i = 0; i < MAX_OUTBOUNDING_EVENTS; i++)
  {
    pthread_mutex_init(&MsgQueueEventMutex[i], NULL);
  }
}

int 
Primus::EnqueueMessageIntoEventQueue(struct message tempMessage)//Return queue len if enqueue succeed, else return -1
{ 
  // cout << "EnqueueMessageIntoEventQueue[" 
  // << tempMessage.linkInfo.identA.level << "." << tempMessage.linkInfo.identA.position << "--"
  // << tempMessage.linkInfo.identB.level << "." << tempMessage.linkInfo.identB.position << "/";
  // if (tempMessage.linkInfo.linkStatus==true) cout << "UP";
  // else cout << "DOWN";
  // cout << " from " << tempMessage.srcIdent.level << "." << tempMessage.srcIdent.position << "]." << endl;

  struct messageevent tempMessageEvent;
  memcpy(&(tempMessageEvent.messageInfo),&tempMessage,sizeof(struct message));
  if (tempMessage.linkInfo.identA.level==3 || tempMessage.linkInfo.identB.level==3)
    tempMessageEvent.numOfSwitchesShouldNotify=affectedNodeNumA;
  else if (tempMessage.linkInfo.identA.level==1 || tempMessage.linkInfo.identB.level==1)
    tempMessageEvent.numOfSwitchesShouldNotify=affectedNodeNumB;

  tempMessageEvent.numOfSwitchesSent=0;
  tempMessageEvent.numOfSwitchesResponsed=0;
  gettimeofday(&tempMessageEvent.startStamp,NULL);

  int queueLen=0;
  while (1)// 可能会造成无限循环
  {
    pthread_mutex_lock(&(MsgQueueMutex));
    if(messageEventQueue.len==MAX_OUTBOUNDING_EVENTS)
    {
      pthread_mutex_unlock(&(MsgQueueMutex));
      return -1;
    }

    tempMessageEvent.messageInfo.messageEventQueueIndex=messageEventQueue.tail;
    memcpy(&(messageEventQueue.eventQueue[messageEventQueue.tail]),&tempMessageEvent,sizeof(struct messageevent));

    //Tag the link-state start time
    // gettimeofday(&(messageEventQueue.eventQueue[messageEventQueue.tail].tvStart),NULL);

    messageEventQueue.tail=(messageEventQueue.tail+1)%MAX_OUTBOUNDING_EVENTS;
    messageEventQueue.len++;
    messageEventQueue.counterIn++;

    queueLen=messageEventQueue.len;
    pthread_mutex_unlock(&(MsgQueueMutex));
    
    // 直到成功
    if(queueLen<=0)
      sched_yield();//send queue is full, wait for the next round
    else
      break;
  }
  return queueLen;
}

bool 
Primus::DequeueMessageFromEventQueue()//Return true if dequeue succeed, else return false
{ 
  struct timeval now;
  pthread_mutex_lock(&(MsgQueueMutex));
  //lock the queue mutex when checking the queue status, in case of the recv thread is writing to this queue
  
  if(messageEventQueue.len<=0){
    pthread_mutex_unlock(&(MsgQueueMutex));
    return false;
  }

  //Perf time
  // gettimeofday(&now,NULL);
  // totalTimeElapsed=totalTimeElapsed+time_diff(messageEventQueue.eventQueue[messageEventQueue.head].tvStart,now);

  messageEventQueue.len--;
  messageEventQueue.head=(messageEventQueue.head+1)%MAX_OUTBOUNDING_EVENTS;
  messageEventQueue.counterOut++;
  pthread_mutex_unlock(&(MsgQueueMutex));

  return true;
}

void 
Primus::InitiateUDPServer()
{
  int serverSock;
  struct sockaddr_in localAddr; 
  struct sockaddr_in remoteAddr;

  bzero(&localAddr,sizeof(struct sockaddr_in));
  localAddr.sin_family=AF_INET; 
  localAddr.sin_addr.s_addr=htons(INADDR_ANY);
  localAddr.sin_port=htons(PRIMUS_LISTEN_PORT); 

  if ((serverSock=socket(PF_INET,SOCK_DGRAM,0))<0)
  {  
    cout << "HandleReadLinkInfo socket failed" << endl;
    exit(1);
  }

  int value=1;
  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    cout << "HandleReadLinkInfo set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    cout << "HandleReadLinkInfo set SO_REUSEADDR error" << endl;
    exit(0);
  }
  
  if (bind(serverSock,(struct sockaddr *)&localAddr,sizeof(localAddr))<0)
  {
    cout << "HandleReadLinkInfo bind failed" << endl;
    exit(1);
  }

  AddSocketToEpoll(serverSock);
  udpSock=serverSock;
}

void 
Primus::InitiateNDServer()
{
  pthread_t NDServerThreadID;
  threadparama *tempThreadParam=new threadparama();
  tempThreadParam->tempPrimus=this;
  tempThreadParam->tempEpollFd=-1;

  if (pthread_create(&NDServerThreadID,NULL,RecvNDMessageThread,(void*)tempThreadParam)!=0)
  {
    cout << "Create RecvNDMessageThread failed!" << endl;
  }
}

void 
Primus::ListenTCP()
{
  struct sockaddr_in localAddr;// 监听本地地址
  memset(&localAddr,0,sizeof(localAddr)); //数据初始化--清零
  localAddr.sin_family=AF_INET; //设置为IP通信
  localAddr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
  localAddr.sin_port=htons(PRIMUS_LISTEN_PORT);

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
    cout << strerror(errno) << "(errno:" << errno << ").\t";
    fprintf(stderr,"Set SO_REUSEPORT error.\n");
    exit(1);
  }

  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    cout << strerror(errno) << "(errno:" << errno << ").\t";
    fprintf(stderr,"Set SO_REUSEADDR error.\n");
    exit(1);
  }

  if (setsockopt(serverSock, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
  {
    cout << strerror(errno) << "(errno:" << errno << ").\t";
    fprintf(stderr,"Set TCP_NODELAY error.\n");
    exit(1);
  }

  /*将套接字绑定到服务器的网络地址上*/
  if(bind(serverSock,(struct sockaddr *)&localAddr,sizeof(struct sockaddr))<0)
  {
    cout << strerror(errno) << "(errno:" << errno << ").\t";
    fprintf(stderr,"Bind Socket Failed.\n");
        exit(1);
  }

  if(listen(serverSock,MAX_TCP_PENDING_ACCEPT_CONNS)<0)
  {
    cout << strerror(errno) << "(errno:" << errno << ").\t";
    fprintf(stderr,"Listen Socket Failed.\n");
    exit(1);
  }

  struct sockaddr_in clientAddr;
  unsigned sin_size=sizeof(struct sockaddr_in);
  int nodeSock=0;
  int acceptSockNum=0;

  while (1)
  {
    if ((nodeSock=accept(serverSock,(struct sockaddr *)&(clientAddr),&sin_size))<0)
    {
      fprintf(stderr,"Accept Socket Failed from %s:%d.\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
      cout << "Sock(" << nodeSock << ") connect error:" << strerror(errno) << "(errno:" << errno << ")." << endl << endl;
      exit(1);
    }
    else
    {
      // int sockIndex=AddNodeSock(nodeSock);// 获得插入的位置
      if (m_EpollFd==-1) CreateEpollFdAndRecvMessageThread();
      
      AddSocketToEpoll(nodeSock);
      
      // printf("Accept Socket(%d) from %s:%d.\n",nodeSock,inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));

      // 每一个send thread都只处理一定数量的node
      if (acceptSockNum%MAX_NODESUN_PER_SENDTHREAD==0)
      {
        pthread_t sendMessageThreadId;
        threadparamb *tempThreadParam=new threadparamb();
        tempThreadParam->tempPrimus=this;
        tempThreadParam->startIndex=acceptSockNum;

        if (pthread_create(&sendMessageThreadId,NULL,SendMessageThread,(void*)tempThreadParam)!=0)
        {
          fprintf(stderr, "ERROR! Thread create failed!\n");
          // exit(1);
        }
      }
      acceptSockNum++;
      //这个方法很不实用，如果前面accept的sock大多close，但数量的累计会使得主线程不停的创建新的发送线程
    }
  }
}

void 
Primus::CreateEpollFdAndRecvMessageThread()
{
  if (m_EpollFd==-1)
  {
    m_EpollFd=epoll_create1(0);// 需要创建一个新的epoll fd和recv线程
          
    if (m_EpollFd==-1) 
    {
      perror("create epoll fd failed.\n");
      exit(1);
    }

    pthread_t recvMessageThreadId;
    threadparama *tempThreadParam=new threadparama();
    tempThreadParam->tempPrimus=this;
    tempThreadParam->tempEpollFd=m_EpollFd;

    if (pthread_create(&recvMessageThreadId,NULL,RecvMessageThread,(void*)tempThreadParam)!=0)
    {
      fprintf(stderr, "ERROR! Thread create failed!\n");
      // exit(1);
    }  
  }
}

void
Primus::GenerateLinkStatusChange()
{
  srand((unsigned)time(NULL)); 
  int tempPosition=0;
  int linkIndex=0;
  int times=1;

  if (m_Ident.level==1)
  {
    for (int i=0;i<times;i++)
    {
      sleep((3+rand()%8));// 3-10s休眠
      tempPosition=rand()%m_LeafNodes+m_Pod*m_LeafNodes;
      struct link tempLink={m_Ident,{2,tempPosition},tempAddr,tempAddr,i+1,i};
      linkIndex=GetLinkIndex(tempLink.identA,tempLink.identB);
      SendLSToController(tempLink,linkIndex,tempIdent);
    }
  }
  else
  {
    for (int i=0;i<times;i++)
    {
      sleep((3+rand()%8));// 3-10s休眠
      tempPosition=(rand()%m_nPods)*m_LeafNodes+m_Ident.position/(m_SpineNodes/m_LeafNodes);
      struct link tempLink={m_Ident,{2,tempPosition},tempAddr,tempAddr,i+1,i};
      linkIndex=GetLinkIndex(tempLink.identA,tempLink.identB);
      SendLSToController(tempLink,linkIndex,tempIdent);
    }
  }
}

void* 
Primus::MasterGenerateLinkStatusChange(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparama *)tempThreadParam)->tempPrimus;
  srand((unsigned)time(NULL)); 
  int tempLinkNum=tempPrimus->linkNum;
  int tempLinkIndex=0;

  for (int i=0;i<10000;i++)
  {
    sleep(3+(rand()%3));
    tempLinkIndex=rand()%tempLinkNum;
    tempPrimus->linkTable[tempLinkIndex].linkInfo.eventId++;
    if (tempPrimus->linkTable[tempLinkIndex].linkInfo.linkStatus==true)
      tempPrimus->linkTable[tempLinkIndex].linkInfo.linkStatus=false;
    else 
      tempPrimus->linkTable[tempLinkIndex].linkInfo.linkStatus=true;
      
    struct message tempMessage={tempPrimus->linkTable[tempLinkIndex].linkInfo,
                              tempPrimus->linkTable[tempLinkIndex].linkInfo.identA,
                              tempPrimus->tempIdent,
                              tempPrimus->m_Ident,
                              false,
                              2,
                              1,
                              -1,
                              1};
    // tempPrimus->PrintMessage(tempMessage);
    tempPrimus->EnqueueMessageIntoEventQueue(tempMessage);
  }
}

void*
Primus::CheckKeepaliveThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparama *)tempThreadParam)->tempPrimus;
  int tempNodeSockNum=tempPrimus->nodeSockNum;

  while (1)
  {
    sleep(KEEPALIVE_INTERVAL);
    // 最好加锁
    for (int i=0;i<tempNodeSockNum;i++)
    {
      if (tempPrimus->nodeSockTable[i].nodeSock==-1)
        continue;
      tempPrimus->nodeSockTable[i].unRecvNum++;
      if (tempPrimus->nodeSockTable[i].unRecvNum==3)
      {
        // shutdown(tempPrimus->nodeSockTable[i].nodeSock,SHUT_RDWR);
        tempPrimus->nodeSockTable[i].nodeSock=-1;
        fprintf(stderr,"Can not touch %d.%d.\n",
          tempPrimus->nodeSockTable[i].nodeIdent.level,
          tempPrimus->nodeSockTable[i].nodeIdent.position);
      }
    }
  }
}

void* 
Primus::KeepaliveThread(void* tempThreadParam)
{
  Primus *tempPrimus=((threadparama *)tempThreadParam)->tempPrimus;
  
  // node主动发送keep alive
  link tempLink={tempPrimus->tempIdent,tempPrimus->tempIdent,tempPrimus->tempAddr,tempPrimus->tempAddr,-1,false};
  struct message tempMessage={tempLink,
                            tempPrimus->m_Ident,
                            tempPrimus->tempIdent,
                            tempPrimus->tempIdent,
                            false,
                            3,
                            tempPrimus->m_Role,
                            -1,
                            1};
  srand((unsigned)time(NULL));
  while (1)
  {
    sleep(KEEPALIVE_INTERVAL);
    for (int i=0;i<MAX_CTRL_NUM;i++)
    {
      if (tempPrimus->SameNode(tempPrimus->controllerSockTable[i].controllerIdent,tempPrimus->tempIdent))
        break;
      if (tempPrimus->controllerSockTable[i].unRecvNum>=3)
      {
        // shutdown(tempPrimus->controllerSockTable[i].controllerSock,SHUT_RDWR);
        tempPrimus->controllerSockTable[i].controllerSock=-1;
        cout << "Can't connect with master." << endl;
      }
      if (tempPrimus->controllerSockTable[i].controllerSock!=-1)
      {
        tempMessage.dstIdent=tempPrimus->controllerSockTable[i].controllerIdent;
        if (tempPrimus->SendMessageByTCP(tempPrimus->controllerSockTable[i].controllerSock,tempMessage)==MESSAGE_BUF_SIZE)
        {
          tempPrimus->controllerSockTable[i].unRecvNum++;
          // cout << "Send keepalive to master " << tempPrimus->controllerSockTable[i].controllerIdent.level
          // << "." << tempPrimus->controllerSockTable[i].controllerIdent.position << endl;
        }
      }
      else
      {
        // cout << "We need to choose a new path to connect with master!" << endl;
        int tempPathIndex=0;
        for (int j=0;j<10;j++)
        {
          // cout << "i:" << i << endl;
          tempPathIndex=rand()%(tempPrimus->pathNum);
          // cout << "tempPathIndex:" << tempPathIndex << endl;
          string masterIP=inet_ntoa(tempPrimus->pathTable[tempPathIndex].addrSet[0].sin_addr);
          // cout << "masterIP:" << masterIP << endl;
          // cout << "faultLinkCounter:" << tempPrimus->pathTable[tempPathIndex].faultLinkCounter << endl;
          if (tempPrimus->pathTable[tempPathIndex].faultLinkCounter==0
            && strcmp(masterIP.c_str(),"127.0.0.1"))
          {
            // cout << "Maybe a fine path" << endl;
            string tempNICName=tempPrimus->GetLocalNICNameByNeighborIdent(tempPrimus->pathTable[tempPathIndex].pathNodeIdent[1]);
            // cout << "tempNICName:" << tempNICName << endl;
            tempPrimus->ConnectWithMaster(masterIP,tempNICName);
            // cout << "Choose path[" << tempPathIndex << "] to connect with Master." << endl << endl;
            // cout << "Choose path[";
            // for (int k=0;k<MAX_PATH_LEN;k++)
            // {
            //   if ()
            // }
            break;
          }
          // cout << endl;
        }
      }
    }
  }
}

void
Primus::CreateKeepAliveThread()
{
  if (m_Role==2 || m_Role==3)//控制器
  {
    pthread_t checkKeepaliveThreadID; 
    threadparama *tempThreadParam=new threadparama();
    tempThreadParam->tempPrimus=this;
    tempThreadParam->tempEpollFd=-1;

    if ((pthread_create(&checkKeepaliveThreadID,NULL,CheckKeepaliveThread,(void*)tempThreadParam))!=0)
     cout << "Create CheckKeepaliveThread failed." << endl;
  }
  if (m_Role==1 || m_Role==3)
  {
    pthread_t keepaliveThreadID; 
    threadparama *tempThreadParam=new threadparama();
    tempThreadParam->tempPrimus=this;
    tempThreadParam->tempEpollFd=-1;

    if ((pthread_create(&keepaliveThreadID,NULL,KeepaliveThread,(void*)tempThreadParam))!=0)
     cout << "Create KeepaliveThread failed." << endl;
  }
}

void
Primus::Start()
{
  // fprintf(stderr, "toRNodes:%d,leafNodes:%d,spineNodes:%d,nPods:%d\n",
  //   m_ToRNodes,
  //   m_LeafNodes,
  //   m_SpineNodes,
  //   m_nPods);
  InitiateLinkTable();
  // PrintLinkTable();

  InitiateMessageEventQueues();
  InitiateNodeSockTable();

  CreateEpollFdAndRecvMessageThread();
  CreateKeepAliveThread();
  InitiateUDPServer();

  pthread_mutex_init(&MsgQueueMutex,NULL);
  pthread_mutex_init(&LinkTableMutex,NULL);

  if (m_Ident.level==0)//master
  {
    if (MASTER_TEST && m_Role==2)
    {
      pthread_t masterGenerateLinkStatusChangeID;
      threadparama *tempThreadParam=new threadparama();
      tempThreadParam->tempPrimus=this;
      tempThreadParam->tempEpollFd=-1;

      if ((pthread_create(&masterGenerateLinkStatusChangeID,NULL,MasterGenerateLinkStatusChange,(void*)tempThreadParam))!=0)
        cout << "Create MasterGenerateLinkStatusChange failed." << endl;
    }
    if (m_Role==3)
    {
      InitiateControllerTable();
      // PrintControllerSockTable();
      ConnectWithMaster("172.16.80.1",MGMT_INTERFACE);
    }
    
    ListenTCP();
  }
  else// node
  {
    InitiatePathTable();
    // PrintPathTable();

    InitiateControllerTable();
    // PrintControllerSockTable();

    InitiateNDServer();
    ConnectWithMaster("172.16.80.1",MGMT_INTERFACE);
    ConnectWithMaster("172.16.80.4",MGMT_INTERFACE);
    ConnectWithMaster("172.16.80.7",MGMT_INTERFACE);
    
    if (NODE_TEST) 
    {
      sleep(5);
      GenerateLinkStatusChange();
    }
    
    pthread_t listenNICThreadID;
    threadparama *tempThreadParam=new threadparama();
    tempThreadParam->tempPrimus=this;
    tempThreadParam->tempEpollFd=-1;
    if (pthread_create(&listenNICThreadID,NULL,ListenNICThread,(void*)tempThreadParam)!=0) 
      cout << "Create ListenNICThread failed!" << endl;

    ListenTCP();
  }
}