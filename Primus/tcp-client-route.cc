#include <iostream>
#include <fstream>
#include "node.h"
#include <pthread.h>

Node *
TCPClientRoute::m_node=NULL;

/*Logfout.setf(ios::fixed, ios::floatfield);
Logfout.precision(9);//设置保留的小数点位数*/

TCPClientRoute::TCPClientRoute ()
{
  /*NS_LOG_FUNCTION (this);*/
  memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
  remote_addr.sin_family=AF_INET; //设置为IP通信
  m_dataSize = 0;
  m_data = new char [m_dataSize];
  m_isConnected = new bool(false);
}
TCPClientRoute::TCPClientRoute (std::string remoteAddr,int remotePort)
{
  /*NS_LOG_FUNCTION (this);*/
  memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
  remote_addr.sin_family=AF_INET; //设置为IP通信
  remote_addr.sin_addr.s_addr =inet_addr(remoteAddr.c_str());
  remote_addr.sin_port=htons(remotePort); //服务器端口号
  m_dataSize = 0;
  m_data = new char [m_dataSize];
  m_isConnected = new bool(false);
}

TCPClientRoute::~TCPClientRoute()
{
  StopApplication();
}

void
TCPClientRoute::SetNode(Node *node,int defaultKeepaliveTimer)
{
  m_node = node;
  m_defaultKeepaliveTimer = defaultKeepaliveTimer;
}

string
TCPClientRoute::GetNow(){
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
TCPClientRoute::SetRemoteAddress(std::string remoteAddr)
{
  remote_addr.sin_addr.s_addr = inet_addr(remoteAddr.c_str());
}
void
TCPClientRoute::SetRemotePort(int remotePort)
{
  remote_addr.sin_port=htons(remotePort); //服务器端口号
}

bool
TCPClientRoute::StartApplication()
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
  if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
  {
    Logfout << GetNow() << "TCPClientRoute::StartApplication (): Create Socket Failed." << std::endl;
    return 0;
  }
   
  int nRecvBuf=1024*1024;
  setsockopt(client_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
  //发送缓冲区
  int nSendBuf=1024*1024;//设置为1M
  setsockopt(client_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
  //add by me
  struct ifreq ifr;
  memset(&ifr, 0x00, sizeof(ifr));
  strncpy(ifr.ifr_name, "eth0", strlen("eth0"));
  if (setsockopt(client_sockfd,SOL_SOCKET,SO_BINDTODEVICE,(char *)&ifr,sizeof(ifr)) < 0)
  {
    Logfout << GetNow() << "Binding error" << std::endl;
  }

  /*将套接字绑定到服务器的网络地址上*/
  if(connect(client_sockfd,(const struct sockaddr *)&remote_addr,sizeof(remote_addr))<0)
  {
    Logfout << GetNow() << "TCPClientRoute::StartApplication (): Bind Socket Failed." << std::endl;
    return 0;
  }
  char *ip = inet_ntoa(remote_addr.sin_addr);
  Logfout << GetNow() << "Try to connected with server [" << ip << "]" << std::endl;
  Logfout.close();
  return 1;
}

void
TCPClientRoute::StopApplication()
{
  SayGoodByeToMaster();
  close(client_sockfd);
}

void 
TCPClientRoute::SubmitLinkStatus(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag)
{
  // ofstream Logfout("/var/log/Primus.log",ios::app);
  // Logfout << GetNow() << "Client SubmitLinkStatus" << std::endl;
  string fill;//LinkStatus|sourceLevel|sourcePos|destLevel|destPos|flag
  fill += "LinkStatus|";
  fill += to_string(sourceLevel);
  fill += '|';
  fill += to_string(sourcePos);
  fill += '|';
  fill += to_string(destLevel);
  fill += '|';
  fill += to_string(destPos);
  fill += '|';
  fill += to_string((int)flag);
  fill += ';';
  SetFillAndSend (fill);
  // Logfout.close();
}

void 
TCPClientRoute::SayGoodByeToMaster()
{
  string fill;
  //Hello|level|positioin
  fill += "GoodBye|;";
  /*string fill = "[LinkStatus]" + "[" + source + "]" + "[" + destination + "]" + "[" + flag + "]";*/
  SetFillAndSend (fill); 
}

void 
TCPClientRoute::SetFillAndSend (std::string fill)
{
    /*NS_LOG_FUNCTION (this << fill);*/
    ofstream Logfout("/var/log/Primus.log",ios::app);
    int dataSize = fill.size () + 1;
    //while(m_dataSize != 0){}

    if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new char [dataSize];
      m_dataSize = dataSize;
    }

    memcpy (m_data, fill.c_str (), dataSize);

    m_dataSize = dataSize;
/*         //测试setsocket
  int optlen;
  char optval [16];
  strcpy(optval,"cubic");
  optlen = strlen(optval);
  int setsockoptNum;

  //客户端连接数据一半测试setsocket
  setsockoptNum = setsockopt(client_sockfd,IPPROTO_TCP,TCP_CONGESTION,optval,optlen);
  Logfout<<GetNow()<<"setsockoptNum is" << setsockoptNum <<std::endl;
  if(setsockoptNum < 0)
  {
    Logfout<<GetNow()<<"TCPServerRoute::StartApplication(): setsockopt Failed"<<std::endl;
  }
    */
    // ofstream Logfout("/var/log/Primus.log",ios::app);
    // Logfout << GetNow() << "Client SetFillAndSend and m_dataSize is " << m_dataSize << std::endl;
    Send(); 

    // Logfout.close();   
}

void 
TCPClientRoute::Send ()
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  /*test*/
  struct timeval tv;
  gettimeofday(&tv,NULL);
  Logfout.setf(ios::fixed, ios::floatfield);
  Logfout.precision(9);//设置保留的小数点位数
  /*end*/
  
  int i;
  if((i = send(client_sockfd,m_data,m_dataSize,0))<=0)
  {
    m_dataSize = 0;
    Logfout << GetNow() << "Client send error：" << i << std::endl;
  }
  Logfout << GetNow() << "Send to Master: " << m_data << "system time is " << tv.tv_sec+tv.tv_usec*0.000001 << std::endl;
/*  //测试setsocket
  int optlen;
  char optval [16];
  strcpy(optval,"cubic");
  optlen = strlen(optval);
  int setsockoptNum;

  //客户端连接数据一半测试setsocket
  setsockoptNum = setsockopt(client_sockfd,IPPROTO_TCP,TCP_CONGESTION,optval,optlen);
  Logfout<<GetNow()<<"setsockoptNum is" << setsockoptNum <<std::endl;
  if(setsockoptNum < 0)
  {
    Logfout<<GetNow()<<"TCPServerRoute::StartApplication(): setsockopt Failed"<<std::endl;
  }*/

  Logfout.close();     
}

//创建线程用
void* 
TCPClientRoute::HandleRead (void* threadParam)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Client created thread HandleRead successfully......" << std::endl;
  //Logfout.flush();
  threadparam *param=(threadparam *)threadParam;
  TCPClientRoute* pThis=param->m_TCPClientRoute;
  int sock=param->index;

  int length;
  char buf[BUFSIZ];
  while(1)
  {
    memset(buf,'\0',sizeof(buf));
    if((length=recv(sock,buf,BUFSIZ,0))<=0)
    {
      Logfout << GetNow() << "Client Receive Socket Failed." << std::endl;
      break;
    }
    if (length>0) 
    {
      // Logfout << std::endl << GetNow() << "Client received message " << buf << std::endl;
      if (buf[0]!='K') Logfout << std::endl << GetNow() << "Client received message " << buf << std::endl;
      string messageSet = buf;
      // Logfout << std::endl << GetNow() << "Client received message " << messageSet << std::endl;
      int start = 0;
      int finish = messageSet.find(";",start);
      while(start < messageSet.size() && finish < messageSet.size())
      {
        string message = messageSet.substr(start,finish-start);
        int begin = 0;
        int end = message.find('|',begin);
        if(message == "Hello|SUCCESS")
        {
          *(pThis->m_isConnected)=true;
          Logfout << GetNow() << "Client connect Socket Success!"<< std::endl;
        }
        else if(message.substr(begin,end-begin) == "LinkStatus")
        {
          begin = end+1;
          end = message.find('|',begin);
          int sourceLevel = strtoul(message.substr(begin,end-begin).c_str(), NULL, 10);
          begin = end+1;
          end = message.find('|',begin);
          int sourcePos = strtoul(message.substr(begin,end-begin).c_str(), NULL, 10);
          begin = end+1;
          end = message.find('|',begin);
          int destLevel = strtoul(message.substr(begin,end-begin).c_str(), NULL, 10);
          begin = end+1;
          end = message.find('|',begin);
          int destPos = strtoul(message.substr(begin,end-begin).c_str(), NULL, 10);
          begin = end+1;
          end = message.size();
          bool flag = (bool)strtoul(message.substr(begin,end-begin).c_str(), NULL, 10);
          //thread_node->HandleMessage(sourceLevel,sourcePos,destLevel,destPos,flag);
          m_node->HandleMessage(sourceLevel,sourcePos,destLevel,destPos,flag);
        }
        else if(message.substr(begin,end-begin) == "KeepAlive")//add by pannian
        {
          pThis->keepAliveFlag=true; 
          // Logfout<< GetNow() << "Master accept keepAlive Success!"<< std::endl;
        }//end
        else
        {
          Logfout << GetNow() << "TCPClientRoute::HandleRead (): Invalid Message." << std::endl;
        }
        start = finish + 1;
        finish = messageSet.find(';',start);
      }
    }
      //sleep(2);
  }
  Logfout.close();
}

void
TCPClientRoute::SayHelloToMaster(int level,int position)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  string fill;
  fill += "Hello|";
  fill += to_string(level);
  fill += "|";
  fill += to_string(position);
  fill += ";";
  SetFillAndSend(fill);

  // add by pannian
  threadparam *param=new threadparam();
  param->m_TCPClientRoute=this;
  param->index=client_sockfd;

  if (pthread_create(&client_thread,NULL,HandleRead,param)!=0)
  {
    Logfout << GetNow() << "Client created HandleRead thread error......" << std::endl;
  }
  Logfout.close();
}

//add by pannian
void*
TCPClientRoute::SayKeepAliveToMaster(void* threadParam)//向Master发送keep alive信息
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Client created thread SayKeepAliveToMaster successfully......" << std::endl;
  threadparam *param=(threadparam *)threadParam;
  TCPClientRoute* pThis=param->m_TCPClientRoute;
  int sock=param->index;
  while (!pThis->m_isConnected)//连接未建立，等待
  {
    sleep(1);
  }

  while(1)
  {
    string fill;
    //KeepAlive|level|position
    fill += "KeepAlive|";
    fill += to_string(pThis->m_level);
    fill += "|";
    fill += to_string(pThis->m_position);
    fill += ";";

    int dataSize = fill.size() + 1;
    if (dataSize !=pThis->m_dataSize)
    {
      delete [] pThis->m_data;
      pThis->m_data = new char[dataSize];
      pThis->m_dataSize = dataSize;
    }
    memcpy (pThis->m_data, fill.c_str (), dataSize);
    pThis->m_dataSize = dataSize;
    send(sock,pThis->m_data,pThis->m_dataSize,0);//发送

    sleep(pThis->m_defaultKeepaliveTimer);//休眠

    //再次发送前需要判断未接收到的Master回复是否超过3次
    if(!pThis->keepAliveFlag)
    {
      pThis->keepAliveFaildNum++;
    }
    else
    {
      pThis->keepAliveFaildNum=0;
      pThis->keepAliveFlag=false;
    }
    // Logfout << GetNow() << "keepAliveFaildNum is " << pThis->keepAliveFaildNum << endl;
    if(pThis->keepAliveFaildNum>=3)
    {
      Logfout << GetNow() << "Send KeepAlive to Master failed!" << std::endl;
      break; 
    }
  }
  Logfout.close();
}
 
void 
TCPClientRoute::KeepAliveTimer(int level,int position)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  threadparam *param=new threadparam();
  param->m_TCPClientRoute=this;
  param->index=client_sockfd;
  this->m_level=level;
  this->m_position=position;
  
  if (pthread_create(&keepAlive_thread,NULL,SayKeepAliveToMaster,param)!=0)
  {
    Logfout << GetNow() << "Client created keepAlive thread error......" << std::endl;
  }
  Logfout.close();
}
// end