#include <iostream>
#include <fstream>
#include <pthread.h>
#include "ipv4-global-routing.h"

Ipv4GlobalRouting *
TCPRoute::m_globalRouting=NULL;

TCPRoute::TCPRoute()
{
	// 
}

TCPRoute::TCPRoute(Ipv4GlobalRouting *tempGlobalRouting,int tempDefaultKeepaliveTimer)
{
	m_globalRouting=tempGlobalRouting;
	m_defaultKeepaliveTimer=tempDefaultKeepaliveTimer;
	myIdent=m_globalRouting->GetMyIdent();
}

TCPRoute::~TCPRoute()
{
	// 
}

string 
TCPRoute::GetNow()
{
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );
  stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

void
TCPRoute::SendMessageTo(int sock,struct MNinfo tempMNIfo)// Master和Node都可以调用
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  char sendBuf[BUF_SIZE];
  memcpy(sendBuf,&tempMNIfo,sizeof(struct MNinfo));

  if((send(sock,sendBuf,sizeof(struct MNinfo),0))<=0)
  {
    Logfout << GetNow() << "Send message failed." << endl;
  }
  Logfout.close();
}

void* 
TCPRoute::ServerThread(void* tempThreadParam)
{
  pthread_detach(pthread_self());
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  
  TCPRoute *tempTCPRoute=((struct threadparam *)tempThreadParam)->tempTCPRoute;
  int clientSock=((struct threadparam *)tempThreadParam)->tempSock;
  char recvBuf[BUF_SIZE];

  while(1)
  {
    memset(recvBuf,'\0',sizeof(recvBuf));
    if((recv(clientSock,recvBuf,BUF_SIZE,0))<=0)
    {
      Logfout << GetNow() << "TCPRoute::ServerThread(): recv Socket Failed. clientSock is " << clientSock <<std::endl;
      break;
    }
    struct MNinfo tempMNInfo;
    memcpy(&tempMNInfo,recvBuf,sizeof(struct MNinfo));
    // 可以写详细一些，把错误内容打印出来
    // if ((tempMNInfo.hello!=true && tempMNInfo.ACK!=true) && (tempMNInfo.destIdent.level!=tempTCPRoute->myIdent.level && tempMNInfo.destIdent.position!=tempTCPRoute->myIdent.position))// 目的地址错误
    // {
    //   Logfout << GetNow() << "Recv error message from ";
    //   if (tempMNInfo.srcIdent.level==0) Logfout << "master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
    //   else Logfout << "node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
    //   Logfout << endl;
    //   continue;
    // }

    // master和node都可调用此部分代码
    if (tempMNInfo.keepAlive==true)//keepalive信息
    {
      // 
    }
    else if (tempMNInfo.hello==true)//node的hello信息，node也有可能收到，因为它为其他节点转发
    {
      Logfout << GetNow() << "Recv hello from node " << tempMNInfo.pathNodeIdentA.level << "." << tempMNInfo.pathNodeIdentA.position << "." << endl;
      if (tempTCPRoute->myIdent.level==0)// master收到hello
      {
        if (tempMNInfo.srcIdent.level==tempMNInfo.pathNodeIdentA.level && tempMNInfo.srcIdent.position==tempMNInfo.pathNodeIdentA.position) // 表示直连
        {
          m_globalRouting->UpdateNodeMapToSock(tempMNInfo.srcIdent,clientSock,true);
        }
        else 
        {
          m_globalRouting->UpdateNodeMapToSock(tempMNInfo.srcIdent,clientSock,false);
        }
        usleep(100000);//限速保护
        tempMNInfo.destIdent=tempMNInfo.srcIdent;
        tempMNInfo.srcIdent=tempTCPRoute->myIdent;
        tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成master的ident
        tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
        tempMNInfo.hello=false;
        tempMNInfo.ACK=true;
        
        char sendBuf[BUF_SIZE];
        memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));

        if((send(clientSock,sendBuf,sizeof(struct MNinfo),0))<=0)
        {
          Logfout << GetNow() << "Send hello Failed." << endl;
          break;
        }
        Logfout << GetNow() << "Send ACK to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
      }
      else //node也有可能收到，因为它为其他节点转发
      {
        // 获得本node和master的连接信息
        vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
        tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成转发node的ident，表示间接连接
        tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
        for (int i=0;i<tempMasterMapToSock.size();i++)// 查找目的ident的套接字
        {
          if (tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true)// 首先本node和该master的连接必须有效
          {
            // 转发信息
            char sendBuf[BUF_SIZE];
            memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));

            if((send(tempMasterMapToSock[i].masterSock,sendBuf,sizeof(struct MNinfo),0))<=0)
            {
              Logfout << GetNow() << "Forward hello Failed." << endl;
              break;
            }
            Logfout << GetNow() << "Forward Hello to master " << tempMasterMapToSock[i].masterIdent.level << "." << tempMasterMapToSock[i].masterIdent.position << "." << endl;
            m_globalRouting->UpdateNodeMapToSock(tempMNInfo.srcIdent,clientSock,true);// 此处必定是直连 
          }
          else//无效该怎么处理，让林老板来写吧
          {
            // 
          }
        }
      }
    }
    else if (tempMNInfo.ACK==true)//node收到 ACK信息
    {
      // node收到master或者中间节点转发的ACK，故不再转发
      if (tempMNInfo.destIdent.level==tempTCPRoute->myIdent.level && tempMNInfo.destIdent.position==tempTCPRoute->myIdent.position)
      {
        Logfout << GetNow() << "Recv ACK from Master " << tempMNInfo.pathNodeIdentB.level << "." << tempMNInfo.pathNodeIdentB.position << "." << endl;
        struct mastermaptosock tempMasterMapToSock;
        tempMasterMapToSock.masterAddr="";
        tempMasterMapToSock.masterIdent=tempMNInfo.srcIdent;// 只有这个有效
        tempMasterMapToSock.masterSock=clientSock;
        tempMasterMapToSock.NICName="";
        tempMasterMapToSock.direct=true;// 只要判断一下pathNodeIdentA和srcIdent是否相等就可以知道是否直连
        tempMasterMapToSock.middleAddr="";
        tempMasterMapToSock.keepAliveFaildNum=0;
        tempMasterMapToSock.keepAliveFlag=false;
        m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);
      }
      else
      {
        // 需要继续转发
        vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();//查找本node作为转发node连接了哪些node
        tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成转发node的ident，表示间接连接
        tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
        for (int i=0;i<tempNodeMapToSock.size();i++)
        {
          if (tempNodeMapToSock[i].nodeIdent.level==tempMNInfo.destIdent.level && tempNodeMapToSock[i].nodeIdent.position==tempMNInfo.destIdent.position)
          {
            // 转发信息
            char sendBuf[BUF_SIZE];
            memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));

            if((send(tempNodeMapToSock[i].nodeSock,sendBuf,sizeof(struct MNinfo),0))<=0)
            {
              Logfout << GetNow() << "Forward ACK failed." << endl;
              break;
            }
            Logfout << GetNow() << "Forward ACK from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
            break;
          }
        }
      }
    }
    else if (tempMNInfo.keepAlive==false && tempMNInfo.hello==false && tempMNInfo.ACK==false)// 链路信息也有可能需要转发
    {
      if (tempMNInfo.destIdent.level==tempTCPRoute->myIdent.level && tempMNInfo.destIdent.position==tempTCPRoute->myIdent.position)// 无需转发
      {
        Logfout << GetNow();
        if (tempTCPRoute->myIdent.level==0) Logfout << "Master ";
        else Logfout << "Node ";
        Logfout << tempTCPRoute->myIdent.level << "." << tempTCPRoute->myIdent.position << " recv ";
        Logfout << tempMNInfo.pathNodeIdentA.level << "." << tempMNInfo.pathNodeIdentA.position << "--" << tempMNInfo.pathNodeIdentB.level << "." << tempMNInfo.pathNodeIdentB.position;
        if (tempMNInfo.linkFlag==true) Logfout << " up.";
        else if (tempMNInfo.linkFlag==false) Logfout << " down.";
        Logfout << endl;

        m_globalRouting->HandleMessage(tempMNInfo.pathNodeIdentA,tempMNInfo.pathNodeIdentB,tempMNInfo.linkFlag);
      }
      else// 需要转发
      {
        Logfout << GetNow() << "-------------Recv linkInfo to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
        vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();//查找本node作为转发node连接了哪些node
        for (int i=0;i<tempNodeMapToSock.size();i++)
        {
          if (tempNodeMapToSock[i].nodeIdent.level==tempMNInfo.destIdent.level && tempNodeMapToSock[i].nodeIdent.position==tempMNInfo.destIdent.position)
          {
            // 转发信息
            char sendBuf[BUF_SIZE];
            memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));

            if((send(tempNodeMapToSock[i].nodeSock,sendBuf,sizeof(struct MNinfo),0))<=0)
            {
              Logfout << GetNow() << "Forward linkInfo failed." << endl;
              break;
            }
            Logfout << GetNow() << "-------------Forward linkInfo from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
            break;
          }
        }
      }
    }
    else
    {
      // 应该处理一下非法信息
    }
  }
  Logfout << GetNow() << "TCPServerThread(" << clientSock << ") thread down!!!!!!!!!!!" << endl;
  Logfout.close();
  pthread_exit(0);
}

void 
TCPRoute::StartListen()// 创建接收数据的线程，起服务端的作用
{
	stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  memset(&serverAddr,0,sizeof(serverAddr)); //数据初始化--清零
  serverAddr.sin_family=AF_INET; //设置为IP通信
  serverAddr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
  // serverAddr.sin_port=htons(MN_PORT);
  // sonic test
  if (myIdent.level==0) serverAddr.sin_port=htons(MN_PORT);
  else serverAddr.sin_port=htons(myIdent.level*1000+myIdent.position*100);// node 作转发时监听了整个交换机，所以设置不同的端口号
  // end

  int serverSock,clientSock;
  /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
  if((serverSock=socket(PF_INET,SOCK_STREAM,0))<0)
  {
    Logfout << GetNow() << "TCPRoute Create Socket Failed." << endl;
    exit(0);
  }

  int value=1;
  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "TCPRoute set SO_REUSEPORT error" << endl;
    exit(0);
  }

  // if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  // {
  //   Logfout << GetNow() << "TCPRoute set SO_REUSEADDR error" << endl;
  //   exit(0);
  // }

  /*将套接字绑定到服务器的网络地址上*/
  if(bind(serverSock,(struct sockaddr *)&serverAddr,sizeof(struct sockaddr))<0)
  {
    Logfout << GetNow() << "TCPRoute Bind Socket Failed." << endl;
    exit(0);
  }

  listen(serverSock,10);

  Logfout << GetNow() << "TCPRoute start listen......" << endl;
  while (1)
  {
    struct sockaddr_in clientAddr;
    unsigned sin_size=sizeof(struct sockaddr_in);
    /*等待客户端连接请求到达*/
    if ((clientSock=accept(serverSock,(struct sockaddr *)&(clientAddr),&sin_size))==-1)
    {
      Logfout << GetNow() << "accept socket error: " << strerror(errno) << " (errno: " << errno <<  ")" << endl;
      break;
    }

    int nRecvBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
    //发送缓冲区
    int nSendBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

    Logfout << GetNow() << "Connect with [" << inet_ntoa(clientAddr.sin_addr) << "] and clientSock is " << clientSock << endl;

        
    struct threadparam *tempThreadParam=(struct threadparam *)malloc(sizeof(struct threadparam));
    tempThreadParam->tempTCPRoute=this;
    tempThreadParam->tempSock=clientSock;

    if(pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
    {
      Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
      break;
    }
  }
  Logfout << GetNow() << "TCPRoute listen thread down." << endl;
  Logfout.close();
}

void
TCPRoute::SendHelloToMaster(vector<string> masterAddress,string middleAddress,string NICName,int port)// 可能是直连，或者是通过中间结点间接连接，所以masterAddress里也可能是中间结点的地址
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  if (middleAddress!="")// 间接连接
  {
    struct sockaddr_in masterAddr;
    memset(&masterAddr,0,sizeof(masterAddr)); //数据初始化--清零
    masterAddr.sin_family=AF_INET; //设置为IP通信
    masterAddr.sin_addr.s_addr=inet_addr(middleAddress.c_str());
    masterAddr.sin_port=htons(port);

    // Logfout << GetNow() << "MasterAddress is " << masterAddress[i] << endl; 
    int clientSock;
    /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
    if((clientSock=socket(PF_INET,SOCK_STREAM,0))<0)
    {
      Logfout << GetNow() << "TCPRoute Create Socket Failed." << endl;
      exit(0);
    }

    int value=1;
    if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)//设置端口复用
    {
      Logfout << GetNow() << "Set SO_REUSEPORT error" << endl;
      exit(0);
    }
     
    int nRecvBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
    // 发送缓冲区
    int nSendBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
    
    // 绑定到网络设备上
    struct ifreq ifr;
    memset(&ifr,0x00,sizeof(ifr));
    strncpy(ifr.ifr_name,NICName.c_str(),strlen(NICName.c_str()));
    if (setsockopt(clientSock,SOL_SOCKET,SO_BINDTODEVICE,(char *)&ifr,sizeof(ifr))<0)
    {
      Logfout << GetNow() << "TCPRoute Binding error" << endl;
      exit(0);
    }

    if((connect(clientSock,(const struct sockaddr *)&masterAddr,sizeof(masterAddr)))<0)
    {
      Logfout << GetNow() << "connect error:" << strerror(errno) << "(errno:" << errno << ")" << endl;
      exit(0);
    }

    struct mastermaptosock tempMasterMapToSock;
    tempMasterMapToSock.masterIdent.level=-1;// 等收到ACK后再填写
    tempMasterMapToSock.masterIdent.position=-1;
    tempMasterMapToSock.masterSock=clientSock;
    tempMasterMapToSock.NICName=NICName;
    tempMasterMapToSock.direct=false;
    tempMasterMapToSock.middleAddr=middleAddress;
    tempMasterMapToSock.keepAliveFaildNum=0;
    tempMasterMapToSock.keepAliveFlag=0;

    for (int i=0;i<masterAddress.size();i++)
    {
      tempMasterMapToSock.masterAddr=masterAddress[i];
      m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);
    }

    // 为该套接字创建接收线程
    struct threadparam *tempThreadParam=(struct threadparam *)malloc(sizeof(struct threadparam));
    tempThreadParam->tempTCPRoute=this;
    tempThreadParam->tempSock=clientSock;

    if(pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
    {
      Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
      exit(0);
    }

    Logfout << GetNow() << "Try to connected with forward node [" << inet_ntoa(masterAddr.sin_addr) << "]." << endl;

    struct MNinfo helloMNIfo;
    helloMNIfo.destIdent.level=-1;// 此时还不知道master的ident
    helloMNIfo.destIdent.position=-1;
    helloMNIfo.srcIdent=myIdent;
    helloMNIfo.pathNodeIdentA=myIdent;
    helloMNIfo.pathNodeIdentB=myIdent;
    helloMNIfo.keepAlive=false;
    helloMNIfo.linkFlag=false;
    helloMNIfo.hello=true;
    helloMNIfo.ACK=false;

    SendMessageTo(clientSock,helloMNIfo);
  }
  else 
  {
    for (int i=0;i<masterAddress.size();i++)
    {
      struct sockaddr_in masterAddr;
      memset(&masterAddr,0,sizeof(masterAddr)); //数据初始化--清零
      masterAddr.sin_family=AF_INET; //设置为IP通信
      masterAddr.sin_addr.s_addr=inet_addr(masterAddress[i].c_str());
      masterAddr.sin_port=htons(MN_PORT);
      
      // Logfout << GetNow() << "MasterAddress is " << masterAddress[i] << endl; 
      int clientSock;
      /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
      if((clientSock=socket(PF_INET,SOCK_STREAM,0))<0)
      {
        Logfout << GetNow() << "TCPRoute Create Socket Failed." << endl;
        exit(0);
      }

      int value=1;
      if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)//设置端口复用
      {
        Logfout << GetNow() << "Set SO_REUSEPORT error" << endl;
        exit(0);
      }

      // if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
      // {
      //   Logfout << GetNow() << "TCPRoute set SO_REUSEADDR error" << endl;
      //   exit(0);
      // }
       
      int nRecvBuf=TCP_BUF_SIZE*1024;
      setsockopt(clientSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
      // 发送缓冲区
      int nSendBuf=TCP_BUF_SIZE*1024;
      setsockopt(clientSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
      
      // 绑定到网络设备上
      struct ifreq ifr;
      memset(&ifr,0x00,sizeof(ifr));
      strncpy(ifr.ifr_name,NICName.c_str(),strlen(NICName.c_str()));
      if (setsockopt(clientSock,SOL_SOCKET,SO_BINDTODEVICE,(char *)&ifr,sizeof(ifr))<0)
      {
        Logfout << GetNow() << "TCPRoute Binding error" << endl;
        exit(0);
      }

      if((connect(clientSock,(const struct sockaddr *)&masterAddr,sizeof(masterAddr)))<0)
      {
        Logfout << GetNow() << "connect error:" << strerror(errno) << "(errno:" << errno << ")" << endl;
        exit(0);
      }

      struct mastermaptosock tempMasterMapToSock;
      tempMasterMapToSock.masterAddr=masterAddress[i];
      tempMasterMapToSock.masterIdent.level=-1;// 等收到ACK后再填写
      tempMasterMapToSock.masterIdent.position=-1;
      tempMasterMapToSock.masterSock=clientSock;
      tempMasterMapToSock.NICName=NICName;
      tempMasterMapToSock.direct=true;
      tempMasterMapToSock.middleAddr="";
      tempMasterMapToSock.keepAliveFaildNum=0;
      tempMasterMapToSock.keepAliveFlag=0;

      m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);

      // 为该套接字创建接收线程
      struct threadparam *tempThreadParam=(struct threadparam *)malloc(sizeof(struct threadparam));
      tempThreadParam->tempTCPRoute=this;
      tempThreadParam->tempSock=clientSock;

      if(pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
      {
        Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
        break;
      }

      Logfout << GetNow() << "Try to connected with master [" << inet_ntoa(masterAddr.sin_addr) << "]." << endl;

      struct MNinfo helloMNIfo;
      helloMNIfo.destIdent.level=-1;// 此时还不知道master的ident
      helloMNIfo.destIdent.position=-1;
      helloMNIfo.srcIdent=myIdent;
      helloMNIfo.pathNodeIdentA=myIdent;
      helloMNIfo.pathNodeIdentB=myIdent;
      helloMNIfo.keepAlive=false;
      helloMNIfo.linkFlag=false;
      helloMNIfo.hello=true;
      helloMNIfo.ACK=false;

      SendMessageTo(clientSock,helloMNIfo);
    }
  }
  Logfout.close();
}