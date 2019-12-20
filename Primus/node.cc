#include "node.h"
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <mutex>

static char names[10][10] = {};
static int status[10];

Node *m_Node=NULL;

Node::Node(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultMasterTimer,int defaultKeepaliveTimer,bool IsCenterRouting,bool randomEcmpRouting)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Setmyident(level,position);
  m_defaultKeepaliveTimer=defaultKeepaliveTimer;
  m_globalRouting=new Ipv4GlobalRouting();
  m_globalRouting->SetAttribute(level,position,ToRNodes,LeafNodes,SpineNodes,nPods,Pod,nMaster,Links,defaultMasterTimer,IsCenterRouting,randomEcmpRouting);
  if(m_isMaster)
  {
    m_globalRouting->InitializeMasterLinkTable();
    m_tcpServer.SetNode(this,m_defaultKeepaliveTimer);
    //test
    /*static double totalTime=0;
    for (int i=0;i<10000;i++)
    {
      totalTime+=m_globalRouting->Test(true);//Master查找和修改链路表的时间测试
    }
    ofstream timeFout("/home/guolab/output/MasterTest.txt",ios::app);
    timeFout.setf(ios::fixed, ios::floatfield);
    timeFout.precision(9);//设置保留的小数点位数

    timeFout << "Server avarage time:" << totalTime/10000 << "s" << std::endl;
     
    exit(1);*/
    //end
    /*m_tcpServer.SetRecvCallback (&Node::HandleMessage,this);*/
  }
  else
  {
    m_globalRouting->InitializePathEntryTable();
    m_tcpClient.SetNode(this,m_defaultKeepaliveTimer);
    //add by Ting
    m_udpServer.SetNode(this);
    m_udpClient.SetNode(this);
    //test
    /*ofstream timeFout("/home/guolab/output/Test.txt",ios::app);
    timeFout.setf(ios::fixed, ios::floatfield);
    timeFout.precision(9);//设置保留的小数点位数
    static double totalTime=0;*/
    /*for (int i=0;i<10;i++)
    {
      totalTime+=m_globalRouting->Test(false);//node查找映射表、查找和修改路径表的时间
    }*/

    /*totalTime+=m_globalRouting->Test(false);//node查找映射表、查找和修改路径表的时间
    
    timeFout << "Node avarage time:" << totalTime << "s" << std::endl;
     
    exit(1);*/
    //end
    /*m_tcpClient.SetRecvCallback (&Node::HandleMessage,this);*/
  }
  m_globalRouting->SetNode(this);
  m_Node=this;
}

void
Node::Start(string masterAddress,int masterPort)
{
  ofstream Logfout("/home/guolab/output/center.log",ios::app);
  if(m_isMaster)
  {
    //std::cout << "I am master!" << std::endl;
    m_tcpServer.SetLocalPort(masterPort);
    m_tcpServer.StartApplication();
    // m_tcpServer.KeepAliveProcess();
  }
  else
  {
    //std::cout << "I am not master!" << std::endl;
    //add by Ting
    m_udpServer.StartApplication();
    m_udpClient.StartApplication();
    string str;
    m_tcpClient.SetRemoteAddress(masterAddress);
    m_tcpClient.SetRemotePort(masterPort);
    m_tcpClient.StartApplication();
    m_tcpClient.SayHelloToMaster(myident.level,myident.position);
    m_tcpClient.KeepAliveTimer(myident.level,myident.position);
    // test
    // if(myident.level == 3)
    // {
    //   int i;
    //   char sndBuf[1024];
    //   memset(sndBuf, 0, 1024);
    //   strcat(sndBuf,this->m_routeProtocal);

    //   SendRouteToMaster();

    //   if((i = send(m_tcpClient.GetClientSoket(),sndBuf,1024,0))<=0)
    //   {
    //     Logfout << GetNow() << "send routeProtocal faild:" << i << std::endl;
    //   }
    // }
    // end
    ListenInterfacesAndSubmit();
  }
  // test
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << "now " << endl;
  // end
  pthread_exit(NULL);
}

ident
Node::Getmyident()
{
  return myident;
}

void
Node::Setmyident(int pre,int suf) 
{ 
  myident.level=pre;
  myident.position=suf;
  if(pre==0 && suf==0)
  {
    m_isMaster = true;
  }
  else
  {
    m_isMaster = false;
  }
}

void 
Node::SendMessageToMaster (int i,bool interfaceFlag)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  // Logfout << GetNow() << "SendMessageToMaster and interfaceFlag is " << interfaceFlag << std::endl;
  if(!m_isMaster )//0.0代表master
  {
    int nextHopInterface = 0;
    ident destident = m_globalRouting->GetNextHopIdent(myident.level,myident.position,i,&nextHopInterface);
    if (interfaceFlag) m_tcpClient.SubmitLinkStatus(myident.level,myident.position,destident.level,destident.position,true);//client向Master发送消息，interface up
    else if (!interfaceFlag) m_tcpClient.SubmitLinkStatus(myident.level,myident.position,destident.level,destident.position,false);//client向Master发送消息，interface down
  }
  Logfout.close();
}

void 
Node::SendMessageToNode(ident high,ident low,bool flag)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Master send message to nodes......" << std::endl;
  //确定转发范围
  vector<int> sock;//用于存储套接字
  if (high.level==2)//状态发生变化的链路位于一二层之间
  {
    for (int i=0;i<nodeMapToSock.size();i++)
    {
      if (nodeMapToSock[i].node.level==1) sock.push_back(nodeMapToSock[i].nodeSock);
      else if (nodeMapToSock[i].node.level==2 && nodeMapToSock[i].node.position%4==high.position%4)//此处的4需要注意,最好用变量表示
      {
        sock.push_back(nodeMapToSock[i].nodeSock);
      }
      else if (nodeMapToSock[i].node.level==3 && nodeMapToSock[i].node.position/4==high.position%4)//此处的4需要注意
      {
        sock.push_back(nodeMapToSock[i].nodeSock);
      }
    }
  }
  else if (high.level==3)//状态发生变化的链路位于二三层之间
  {
  for (int i=0;i<nodeMapToSock.size();i++)
    {
      if (nodeMapToSock[i].node.level==1) sock.push_back(nodeMapToSock[i].nodeSock);
      else if (nodeMapToSock[i].node.level==2 && nodeMapToSock[i].node.position%4==low.position%4)//此处的4需要注意,最好用变量表示
      {
        sock.push_back(nodeMapToSock[i].nodeSock);
      }
      else if (nodeMapToSock[i].node.level==3 && nodeMapToSock[i].node.position==high.position)
      {
        sock.push_back(nodeMapToSock[i].nodeSock);
        break;
      }
    }
  }
  m_tcpServer.NoticeToNode(high.level,high.position,low.level,low.position,flag,sock);
  Logfout.close();
}

void 
Node::SetUp(int ifaceIndex)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Node " << myident.level << "." << myident.position << " interface " << ifaceIndex << " up......" << std::endl;
  SendMessageToMaster(ifaceIndex,true);
  Logfout.close();
}

void 
Node::SetDown(int ifaceIndex)
{ 
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Node " << myident.level << "." << myident.position << " interface " << ifaceIndex << " down......" << std::endl;
  SendMessageToMaster(ifaceIndex,false);
  Logfout.close();
}

int 
Node::GetInterfaceStatus(char *name)
{
    struct ifreq ifr;  
    int sockfd;  
 
    ofstream Logfout("/var/log/Primus.log",ios::app);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  
    {  
        Logfout << GetNow() << "Create socket fails!" << std::endl;
        return -1;    
    }   
 
    strcpy(ifr.ifr_name, name);  
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)  
    {  
        Logfout << GetNow() << "ioctl SIOCGIFFLAGS fails!\n";
        close(sockfd);  
        return -1;    
    }  
 
    close(sockfd);  
    Logfout.close();
    return ifr.ifr_flags& IFF_UP; 
}

void*
Node::ListeningNICProcess(void* i)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  int interfaceId = *(int*)i;
  Logfout << GetNow() << "Node is listening interface " << interfaceId << " ......" << std::endl;
  while(1)
  {
    int sta = m_Node->GetInterfaceStatus(names[interfaceId]);
    if(sta != status[interfaceId])
    {
      if(sta == 1) Logfout << GetNow() << "Interface: " << names[interfaceId] << " is Up.\n";  //printf("Interface: [%s] is Up.\n",names[interfaceId]);
      else Logfout << GetNow() << "Interface: " << names[interfaceId] << " is Down.\n";  //printf("Interface: [%s] is Down.\n",names[interfaceId]);
      status[interfaceId] = sta;
      if(sta != 1)
      {
        Logfout << GetNow() << "Submit: " << names[interfaceId] << " is Down.\n"; //printf();
        m_Node->SetDown((int)(names[interfaceId][3]-48+1));            
      }
      else
      {
        Logfout << GetNow() << "Start Hello: " << names[interfaceId] << " is Up.\n"; //printf();
        m_Node->SetUp((int)(names[interfaceId][3]-48+1));
      }
    }
    usleep(10000);
  }
  Logfout.close();
}

void 
Node::ListenInterfacesAndSubmit()
{
    ofstream Logfout("/var/log/Primus.log",ios::app);
    Logfout << GetNow() << "Node ListenInterfaces And Submit" << std::endl;
    // listen to interfaces
    struct ifaddrs *ifc, *ifc1;  
    char ips[10][64] = {};  
    char nms[10][64] = {};  
    char ip[64] = {};
    char nm[64] = {};
    char netmask[16] = {};

    int count = 0;

    if(0 != getifaddrs(&ifc)) 
    {
        printf("ListenInterfaces failed.\n");
        return;
    } 
    ifc1 = ifc;  

    //获取网卡信息，并存储到数组

    for (; NULL != ifc; ifc = (*ifc).ifa_next) 
    {  
      if (NULL != (*ifc).ifa_addr) 
      {  
        inet_ntop(AF_INET, &(((struct sockaddr_in*)((*ifc).ifa_addr))->sin_addr), ip, 64);  
      } 
      else 
      {  
        //printf("\t\t");  
      }  
      if (NULL != (*ifc).ifa_netmask)
      {  
        inet_ntop(AF_INET, &(((struct sockaddr_in*)((*ifc).ifa_netmask))->sin_addr), nm, 64);  
        inet_ntop(AF_INET, &(((struct sockaddr_in*)((*ifc).ifa_netmask))->sin_addr), netmask, 16);
      } 
      else 
      {  
        //printf("\t\t");  
      }  
      //printf("\n");  
      if(count < 10 )
      {            
        if(NULL != (*ifc).ifa_addr && NULL != (*ifc).ifa_netmask)
        {
          char example[16] = "255.255.255.254"; 
          int j;
          for(j=0;j<16;j++)
          {
            if(netmask[j]!=example[j])break;
          }
          if(j!=16)continue;

          strcpy(names[count], (*ifc).ifa_name);
          strcpy(ips[count], ip);
          strcpy(nms[count], nm);
          status[count] = GetInterfaceStatus(names[count]);
          SetUp((int)(names[count][3]-48+1));
          count++;               
        }    
      }
      else
      {
        Logfout << GetNow() << "interfaces number too large\n"; //printf();
      }
    }  
    freeifaddrs(ifc1);  
    //add by hua
    int* k;
    for(int i = 0 ;i < count; i++)
    {
      k=(int*)malloc(sizeof(int));
      *k=i;
      //对于每一个记录的网卡信息，新建线程
      if (pthread_create(&NIC_thread,NULL,ListeningNICProcess,(void*) k)<0)
      {
        Logfout << GetNow() << "Node created thread error" << std::endl;
      }
    }
    Logfout.close();
}

// void //netlink
// Node::ListenInterfacesAndSubmit()
// {
//     ofstream Logfout("/var/log/Primus.log",ios::app);
//     Logfout << GetNow() << "Node ListenInterfaces And Submit" << std::endl;
//     int fd;
//     int ret;
//     struct sockaddr_nl addr;
//     char buf[20480] = {0};
//     int len = 1048576;

//     struct {
//         struct nlmsghdr nh;
//         struct ifinfomsg msg;
//     } req;

//     struct iovec iov;
//     struct msghdr msg;

//     fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
//     if (fd == -1) {
//         printf("socket failed, %s\n", strerror(errno));
//         return -1;
//     }

//     ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
//     if (ret == -1) {
//         printf("setsockopt failed, %s\n", strerror(errno));
//         return -1;
//     }

//     bzero(&addr, sizeof(addr));
//     addr.nl_family = AF_NETLINK;
//     ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
//     if (ret == -1) {
//         printf("bind failed, %s\n", strerror(errno));
//         return -1;
//     }

//     bzero(&ret, sizeof(ret));
//     req.nh.nlmsg_len = NLMSG_ALIGN(sizeof(req));
//     req.nh.nlmsg_type = RTM_GETLINK;
//     req.nh.nlmsg_flags = NLM_F_ROOT | NLM_F_REQUEST;
//     req.nh.nlmsg_pid = 0;
//     req.nh.nlmsg_seq = 0;
//     req.msg.ifi_family = AF_UNSPEC;

//     while(1){
//       ret = send(fd, &req, sizeof(req), 0);
//       if (ret == -1) {
//           printf("send failed, %s\n", strerror(errno));
//           return -1;
//       }

//       iov.iov_base = buf;
//       iov.iov_len = sizeof(buf);
//       msg.msg_name = &addr;
//       msg.msg_namelen = sizeof(addr);
//       msg.msg_iov = &iov;
//       msg.msg_iovlen = 1;

//       while(1) {
//           int found = 0;
//           struct nlmsghdr *nh;
//           ret = recvmsg(fd, &msg, 0);
//           if (ret == -1) {
//               if (errno == EINTR || errno == EAGAIN)
//                   continue;
//               printf("recvmsg failed, %s\n", strerror(errno));
//               return -1;
//           }

//           if (ret == 0) {
//               printf("recv EOF\n");
//               return -1;
//           }

//           nh = (struct nlmsghdr *)buf;

//           while(NLMSG_OK(nh, ret)) {
//               if (nh->nlmsg_type == NLMSG_DONE) {
//                   found = 1;
//                   break;
//               } else if (nh->nlmsg_type == NLMSG_ERROR) {
//                   printf("NLMSG_ERROR\n");
//                   return -1;
//               }

//               struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nh);
//               struct rtattr *rta = IFLA_RTA(ifi);
//               int payload_len = IFLA_PAYLOAD(nh);           

//               while(RTA_OK(rta, payload_len)) {
//                 if (rta->rta_type == IFLA_IFNAME) {
//             if (ifi->ifi_flags & IFF_RUNNING) { //判断网卡是否工作
//               if (ifi->ifi_type==ARPHRD_ETHER){//判断是否为Ethernet 10/100Mbps,ifi_type定义在 "/usr/include/net/if_arp.h"
//                 char *ifname = (char *)RTA_DATA(rta);
//                           printf("%s,flag=%u,type=%u\n",ifname,ifi->ifi_flags,ifi->ifi_type);
//                           // 此处为每个新的网卡建立线程，发送hello给邻居
//                           // function future
//               }
//                     }
//                   }
//                   rta = RTA_NEXT(rta, payload_len);
//               }
//               nh = NLMSG_NEXT(nh, ret);
//           }
//           if (found) {
//               break;
//           }
//       }
//       sleep(10);
//       printf("-----------------------\n\n");
//     }
//     Logfout.close();
// }

void
Node::IssueInformation(ident source,ident destination,bool flag)//master向node发送消息
{
  //Ting
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Master send message to nodes......" << std::endl;
  Logfout.close();
  // if(m_isMaster)
  // {
  //   //tingting's code
  //   m_tcpServer.IssueLinkStatus(source.level,source.position,destination.level,destination.position,flag);
  // } 
}

void
Node::HandleMessage(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  ident high,low;
  if (sourceLevel>destLevel)
  {
    high.level=sourceLevel;
    high.position=sourcePos;
    low.level=destLevel;
    low.position=destPos;
  }
  else if (sourceLevel<destLevel)
  {
    low.level=sourceLevel;
    low.position=sourcePos;
    high.level=destLevel;
    high.position=destPos;
  }
  if(m_isMaster)
  {
    /*test*/
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC,&tv);
    Logfout.setf(ios::fixed, ios::floatfield);
    Logfout.precision(9);//设置保留的小数点位数
    /*end*/
    bool isNeedToNotice=false;
    m_globalRouting->ModifyMasterLinkTable(high,low,&isNeedToNotice,flag);//同时判断链路信息是否需要转发，因为链路信息可能已经处理了
    Logfout << GetNow() << "Node " << myident.level << "." << myident.position << " handle message " << ",isNeedToNotice is " << isNeedToNotice << ",system time is " << tv.tv_sec+tv.tv_nsec*0.000000001 << "......" << std::endl;
    if (isNeedToNotice)
    {
      SendMessageToNode(high,low,flag);
    } 
  }
  else
  {
    Logfout << GetNow() << "Node " << myident.level << "." << myident.position << " handle message......" << std::endl;
    m_globalRouting->ModifyPathEntryTable(high,low,flag);   
  }
  Logfout.close();
}

void
Node::RecordNodeMapToSock(int level,int postion,int sock)
{
  struct nodemaptosock sample;
  sample.node.level=level;
  sample.node.position=postion;
  sample.nodeSock=sock;
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
      if (nodeMapToSock[j+1].node.level<nodeMapToSock[j].node.level)//大体分为一二三层Node
      {
        temp=nodeMapToSock[j+1];
        nodeMapToSock[j+1]=nodeMapToSock[j];
        nodeMapToSock[j]=temp;
      }
      else if (nodeMapToSock[j+1].node.level==nodeMapToSock[j].node.level)//属于同一层Node
      {
        if (nodeMapToSock[j].node.level==1 || nodeMapToSock[j].node.level==3)//第一三层Node
        {
          if (nodeMapToSock[j+1].node.position<nodeMapToSock[j].node.position)//升序排列
          {
            temp=nodeMapToSock[j+1];
            nodeMapToSock[j+1]=nodeMapToSock[j];
            nodeMapToSock[j]=temp;
          }
        }
        else if (nodeMapToSock[j].node.level==2)//第二层Node，对LeafNode取余，相等的放一起，同时升序排列
        {
          if (nodeMapToSock[j+1].node.position%4<nodeMapToSock[j].node.position%4)//此处的4需要注意
          {
            temp=nodeMapToSock[j+1];
            nodeMapToSock[j+1]=nodeMapToSock[j];
            nodeMapToSock[j]=temp;
          }
        }
      }
    }
  }
  //test
  ofstream Logfout("/var/log/nodemaptosock.log",ios::trunc);
  for (int i=0;i<nodeMapToSock.size();i++)
  {
    Logfout << nodeMapToSock[i].node.level << "." << nodeMapToSock[i].node.position << "  " << nodeMapToSock[i].nodeSock << endl;
  }
  Logfout << endl;
  Logfout.close();
}

void 
Node::PrintMasterLink()
{
  m_globalRouting->PrintMasterLinkTable();
}

void 
Node::PrintPathTable()
{
  m_globalRouting->PrintPathEntryTable();
}

void 
Node::PrintMappingTable()
{
  m_globalRouting->PrintMappingTable();
}

string
Node::GetNow(){
  
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );
  /*cout << tt << endl;*/

  /*printf("[%d-%02d-%02d %02d:%02d:%02d]:",
           t->tm_year + 1900,
           t->tm_mon + 1,
           t->tm_mday,
           t->tm_hour,
           t->tm_min,
           t->tm_sec);*/
  /*string time = to_string(t->tm_hour) +":"+ to_string(t->tm_min) +":"+ to_string(t->tm_sec);*/
  stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

void
Node::Test()
{
  /*test master timer*/
  ofstream Logfout("/var/log/Primus.log",ios::app);
  sleep(10);
  // Logfout << GetNow() << "sleep time is " << sleep(10) << "s" << std::endl;//waiting for 10s
  bool testFlag=false;
  for (int i=0;i<10;i++)
  {
    usleep(100000);//每隔100ms提交一次
    Logfout << GetNow() << "第" << i << "次提交------" << std::endl;
    m_tcpClient.SubmitLinkStatus(2,3,1,0,testFlag);
    Logfout << std::endl;
    testFlag=(testFlag==true)?false:true;
  }
}
// for (int i=0;i<nodeMapToSock.size()-1;i++)
//   {
//     for (int j=0;j<nodeMapToSock.size()-i-1;j++)


void 
Node::RecordKeepAliveFlag(int sock)//收到一个keep alive，找到对应的map，将标志位置为true
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  for (vector<struct nodemaptosock>::iterator iter=nodeMapToSock.begin();iter!=nodeMapToSock.end();iter++)
  {
    if((*iter).nodeSock==sock)
    {
      // Logfout << GetNow() << "RecordKeepAliveFlag and node is " << (*iter).node.level << "." << (*iter).node.position << ",keepAliveFlag is " << (*iter).keepAliveFlag << endl;
      (*iter).keepAliveFlag=true;
      // Logfout << GetNow() << "RecordKeepAliveFlag and node is " << (*iter).node.level << "." << (*iter).node.position << ",keepAliveFlag is " << (*iter).keepAliveFlag << endl;
      break;
    }
  }
}

//定期检查keep alive的接收情况，若收到keep alive，将标志位置为false；若未收到keep alive，num++，超过3次宣告连接无效
void
Node::CheckKeepAliveFlag()
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  // Logfout << GetNow() << "CheckKeepAliveFlag" << endl;
  for (vector<struct nodemaptosock>::iterator iter=nodeMapToSock.begin();iter!=nodeMapToSock.end();iter++)
  {
    if ((*iter).keepAliveFlag==true) 
    {
      (*iter).keepAliveFlag==false;
      (*iter).keepAliveFaildNum=0;
      // Logfout << GetNow() << " 1   " << (*iter).node.level << "." << (*iter).node.position << "'s keepAliveFlag is " << (*iter).keepAliveFlag;
      // Logfout << " and keepAliveFaildNum is " << (*iter).keepAliveFaildNum << endl;
    }
    else if ((*iter).keepAliveFlag==false)
    {
      (*iter).keepAliveFaildNum++;
      // Logfout << GetNow() << " 2   " << (*iter).node.level << "." << (*iter).node.position << "'s keepAliveFlag is " << (*iter).keepAliveFlag;
      // Logfout << " and keepAliveFaildNum is " << (*iter).keepAliveFaildNum << endl;
    } 
    // Logfout << GetNow() << (*iter).node.level << "." << (*iter).node.position << "'s keepAliveFlag is " << (*iter).keepAliveFlag;
    // Logfout << " and keepAliveFaildNum is " << (*iter).keepAliveFaildNum << endl;
    if ((*iter).keepAliveFaildNum>=3)// 判断为失效后应该清除该连接的信息，以后处理
    {
      Logfout << GetNow() << "connect with " << (*iter).node.level << "." << (*iter).node.position << " failed------" << endl;
    }
  }
  // Logfout << endl;
  Logfout.close();
}
//end

//add by pannian
//客户端获取到netlink消息
// int
// Node::rtnl_receive(int fd, struct msghdr *msg, int flags)
// {
//   ofstream Logfout("/home/guolab/output/center.log",ios::app);
//   int len;

//   do { 
//     len = recvmsg(fd, msg, flags);
//   } while (len < 0 && (errno == EINTR || errno == EAGAIN));

//   if (len < 0) {
//     Logfout << GetNow() << "Netlink receive failed" << std::endl;
//      return -errno;
//   }

//   if (len == 0) { 
//     Logfout << GetNow() << "EOF on netlink" << std::endl;
//     //perror("EOF on netlink");
//     return -ENODATA;
//   }

//   return len;
// }

// static int 
// Node::rtnl_recvmsg(int fd, struct msghdr *msg, char **answer)
// {
//   ofstream Logfout("/home/guolab/output/center.log",ios::app);
//   struct iovec *iov = msg->msg_iov;
//   char *buf;
//   int len;

//   iov->iov_base = NULL;
//   iov->iov_len = 0;

//   len = rtnl_receive(fd, msg, MSG_PEEK | MSG_TRUNC);

//   if (len < 0) {
//         return len;
//   }

//   buf = (char *)malloc(len);

//   if (!buf) {
//     Logfout << GetNow() << "malloc failed" << std::endl;
//     //perror("malloc failed");
//     return -ENOMEM;
//   }

//   iov->iov_base = buf;
//   iov->iov_len = len;

//   len = rtnl_receive(fd, msg, 0);

//   if (len < 0) {
//     free(buf);
//     return len;
//   }

//   *answer = buf;

//   return len;
// }

// void 
// Node:: parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
// {
//   memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

//   while (RTA_OK(rta, len)) {
//     if (rta->rta_type <= max) {
//       tb[rta->rta_type] = rta;
//     }

//     rta = RTA_NEXT(rta,len);
//   }
// }

// static inline int 
// Node::rtm_get_table(struct rtmsg *r, struct rtattr **tb)
// {
//   __u32 table = r->rtm_table;

//   if (tb[RTA_TABLE]) {
//     table = *(__u32 *)RTA_DATA(tb[RTA_TABLE]);
//   }

//   return table;
// }

// void
// Node:: print_route(struct nlmsghdr* nl_header_answer)
// {
//   // printf("print_route---------------\n\n");
//   ofstream Logfout("/home/guolab/output/center.log",ios::app);
//   struct rtmsg* r = (struct rtmsg *)NLMSG_DATA(nl_header_answer);
//   struct rtm Rtm ;
//   int len = nl_header_answer->nlmsg_len;
//   struct rtattr* tb[RTA_MAX+1];
//   int table;
//   char rootFlag = '3';
//   char buf[256];
//   char routeData[20];
//   char routeOif[20];

//   len -= NLMSG_LENGTH(sizeof(*r));

//   if (len < 0) {
//     Logfout << GetNow() << "Wrong message length" << std::endl;
//     //printf("Wrong message length");
//     return;
//   }
  
//   parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

//   table = rtm_get_table(r, tb);

//   if (r->rtm_family != AF_INET && table != RT_TABLE_MAIN) {
//      Logfout << GetNow() << "error2\n" << std::endl;
//     //printf("error2\n");
//     return;
//   }

//   if (tb[RTA_DST]) {
//     /*if ((r->rtm_dst_len != 24) && (r->rtm_dst_len != 16)) {
//       printf("error3\n");
//       return;
//     }*/
//     //printf("--长度：%d----",r->rtm_dst_len);
//      Logfout << GetNow() <<inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_DST]), buf, sizeof(buf))<<"/"<<r->rtm_dst_len<< std::endl;
//      strcpy(routeData,inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_DST]), buf, sizeof(buf)));
//    // printf("%s/%u ", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_DST]), buf, sizeof(buf)), r->rtm_dst_len);
 
//   } else if (r->rtm_dst_len) {
//      Logfout << GetNow() << r->rtm_dst_len << std::endl;
//    // printf("/%u ", r->rtm_dst_len);
//   } else {
//    Logfout << GetNow() << "default " << std::endl;
//     //printf("default ");
//   }

//   if (tb[RTA_GATEWAY]) {//网关
//      Logfout << GetNow() << "via "<< inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), buf, sizeof(buf))<< std::endl;
//    // printf("via %s", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), buf, sizeof(buf)));
//   }

//   if (tb[RTA_OIF]) {
//      char if_nam_buf[IF_NAMESIZE];
//      int ifidx = *(__u32 *)RTA_DATA(tb[RTA_OIF]);
//      Logfout << GetNow() << "dev "<< if_indextoname(ifidx, if_nam_buf)<< std::endl;
//     //printf(" dev %s", if_indextoname(ifidx, if_nam_buf));
//      strcpy(routeOif,if_indextoname(ifidx, if_nam_buf));
//   }

//   if (tb[RTA_SRC]) {
//      Logfout << GetNow() << "src  "<<inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_SRC]), buf, sizeof(buf))<< std::endl;
//    // printf("src %s", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_SRC]), buf, sizeof(buf)));
//   }
//     Logfout << GetNow() << "协议：  "<< r->rtm_protocol << std::endl;
//     this->m_routeProtocal = r->rtm_protocol;
//     if(r->rtm_protocol != rootFlag)
//     {
//       Rtm.m_routeProtocal = r->rtm_protocol;
//       strcpy(Rtm.m_routeData,routeData);
//       strcpy(Rtm.m_routeOif,routeOif);
//     }
//     rtm.push_back(RTM);
//  // printf("-------------%d",r->rtm_protocol);

//   //printf("\n");
// }

// int 
// Node::open_netlink()
// {
//   ofstream Logfout("/home/guolab/output/center.log",ios::app);
//   struct sockaddr_nl saddr;

//   int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

//   if (sock < 0) {
//      Logfout << GetNow() << "Failed to open netlink socket  "<< std::endl;
//    // perror("Failed to open netlink socket");
//     return -1;
//   }
 
//   memset(&saddr, 0, sizeof(saddr));

//   saddr.nl_family = AF_NETLINK;
//   saddr.nl_pid = getpid();
//   Logfout << GetNow() << "pid is "<<saddr.nl_pid << std::endl;
//   //printf("pid is %d\n",saddr.nl_pid );

//   if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
//     perror("Failed to bind to netlink socket");
//     close(sock);
//     return -1;
//   }

//   return sock;
// }

// int 
// Node::do_route_dump_requst(int sock)
// {
//     struct {
//         struct nlmsghdr nlh;
//         struct rtmsg rtm;
//     } nl_request;

//   nl_request.nlh.nlmsg_type = RTM_GETROUTE;//消息长度包括标题
//   nl_request.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;//附加标志
//   nl_request.nlh.nlmsg_len = sizeof(nl_request);
//   nl_request.nlh.nlmsg_seq = time(NULL);//
//   nl_request.rtm.rtm_family = AF_INET;

//   return send(sock, &nl_request, sizeof(nl_request), 0);
// }

// int 
// Node::get_route_dump_response(int sock)
// {
//   ofstream Logfout("/home/guolab/output/center.log",ios::app);
//   struct sockaddr_nl saddr;
//   struct sockaddr_nl nladdr;
//   struct iovec iov;//多缓冲区的发送和接收处理就是一个struct iovec的数组
//   struct msghdr msg = {//得到对方的地址和端口
//     .msg_name = &nladdr,
//     .msg_namelen = sizeof(nladdr),
//     .msg_iov = &iov,
//     .msg_iovlen = 1,//buffer缓冲区的个数
//   };

//   char *buf;
//   int dump_intr = 0;

//   int status = rtnl_recvmsg(sock, &msg, &buf);

//   struct nlmsghdr *h = (struct nlmsghdr *)buf;
//   int msglen = status;
//   int i = 0;

//   Logfout << GetNow() << "Main routing table IPv4 "<< std::endl;
//   //printf("Main routing table IPv4\n");

//   while (NLMSG_OK(h, msglen)) {
//     if (h->nlmsg_flags & NLM_F_DUMP_INTR) {
//       fprintf(stderr, "Dump was interrupted\n");
//       free(buf);
//       return -1;
//     } 

//     if (nladdr.nl_pid != 0) {
//       continue;
//     }

//     if (h->nlmsg_type == NLMSG_ERROR) {
//       Logfout << GetNow() <<"netlink reported error"<< std::endl;
//       //perror("netlink reported error");
//       free(buf);
//     }

//     print_route(h);
//     this->routeProtocal[i] = this->m_routeProtocal;
//     i++;

//     h = NLMSG_NEXT(h, msglen);
//   }

//   free(buf);

//   return status;
// }

// void 
// Node::SendRouteToMaster()
// {
//   ofstream Logfout("/home/guolab/output/center.log",ios::app);
//   int nl_sock = open_netlink();

//   if (do_route_dump_requst(nl_sock) < 0) {
//     Logfout << GetNow() <<"Failed to perfom request"<< std::endl;
//     //perror("Failed to perfom request");
//     close(nl_sock);
//     return -1;
//   }

//   get_route_dump_response(nl_sock);

//   close (nl_sock);

//   return 0;
// } 
// end