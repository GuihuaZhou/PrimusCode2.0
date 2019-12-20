#include <pthread.h>
#include <iostream>
#include <fstream>
#include "node.h"

int 
TCPServerRoute::client_count = 0;

Node *
TCPServerRoute::m_node=NULL;

TCPServerRoute::TCPServerRoute ()
{
  /*NS_LOG_FUNCTION (this);*/
  memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零
  my_addr.sin_family=AF_INET; //设置为IP通信
  my_addr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
  my_addr.sin_port=htons(6666); //服务器端口号
  m_dataSize = 20;
  m_data = new char[m_dataSize];
}

TCPServerRoute::TCPServerRoute (int Localport)
{
  /*NS_LOG_FUNCTION (this);*/
  memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零
  my_addr.sin_family=AF_INET; //设置为IP通信
  my_addr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
  my_addr.sin_port=htons(Localport); //服务器端口号
  m_dataSize = 20;
  m_data = new char[m_dataSize];
}

TCPServerRoute::~TCPServerRoute()
{
  StopApplication();
}

void
TCPServerRoute::SetNode(Node *node,int defaultKeepaliveTimer)
{
  m_node=node;
  m_defaultKeepaliveTimer = defaultKeepaliveTimer;
}

void
TCPServerRoute::SetLocalPort(int localPort)
{
  my_addr.sin_port=htons(localPort); //服务器端口号
}

bool
TCPServerRoute::StartApplication()
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
  if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
  {
    Logfout <<GetNow()<<"TCPServerRoute::StartApplication (): Create Socket Failed."<<std::endl;
    return false;
  }

  int on=6666;//add by me
  if ((setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
  {
    Logfout <<GetNow()<<"TCPServerRoute::StartApplication (): Set Socket Failed."<<std::endl;
    return false;
  }

  int client_sockfd;

  int nRecvBuf=1024*1024;
  setsockopt(client_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
  //发送缓冲区
  int nSendBuf=1024*1024;//设置为1M
  setsockopt(client_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

  /*将套接字绑定到服务器的网络地址上*/
  if(bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
  {
    Logfout<<GetNow()<<"TCPServerRoute::StartApplication (): Bind Socket Failed."<<std::endl;
    return false;
  }
  
  Logfout << GetNow() << "TCPServer Start!" << std::endl;

  // TCPServerRoute* object=this;
  // if(pthread_create(&keepalive_thread,NULL,KeepAliveThread,(void*)object)<0)
  // {
  //   Logfout << GetNow() << "could not create keepalive thread" << endl;
  //   return false; 
  // }

  HandleRead(); 
  Logfout.close();
  return true;
}

void
TCPServerRoute::StopApplication()
{
  for(int i=0;i<client_count;i++)
  {
    close(client_sockfds[i]);
  }
  close(server_sockfd);
}

void* 
TCPServerRoute::ServerThread(void* client_sockfd_ptr)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  int sock=*(int*)client_sockfd_ptr;
  int len;
  char buf[BUFSIZ] = {};
  //char optval [TCP_CA_NAME_MAX]; 
  Logfout << GetNow() << "Server created server thread successfully and socket is " << sock << std::endl;
  while(1)
  {
    memset(buf,'\0',sizeof(buf));
    if((len=recv(sock,buf,BUFSIZ,0))<=0)
    {
      Logfout << GetNow() << "TCPServerRoute::HandleRead (): Accept Socket Failed. sock is " << sock <<std::endl;
      break;
    }
    // Logfout << std::endl << GetNow() << "Server recieve message " << buf << "......" << std::endl;
    if (buf[0]!='K') Logfout << std::endl << GetNow() << "Server recieve message " << buf << "......" << std::endl;
    string messageSet = buf;
    int start = 0;
    int finish = messageSet.find(';',start);
    while(start < messageSet.size() && finish < messageSet.size())
    {
      string message = messageSet.substr(start,finish-start);
      int begin = start;
      int end = message.find('|',begin);
      if(message.substr(begin,end-begin) == "Hello")
      {
        send(sock,"Hello|SUCCESS;",14,0);
        Logfout << GetNow() << "Server send Hello|SUCCESS to Node and sock is " << sock << std::endl;
        //add by me,在server端建立Node编号和套接字之间的映射关系
        int levelBegin=end+1;
        int levelEnd=message.find('|',levelBegin);
        int nodeLevel=atoi((message.substr(levelBegin,levelEnd-levelBegin)).c_str());

        int positionBegin=levelEnd+1;
        int positionEnd=message.find('|',positionBegin);
        int nodePosition=atoi((message.substr(positionBegin,positionEnd-positionBegin)).c_str());

        m_node->RecordNodeMapToSock(nodeLevel,nodePosition,sock);
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
        m_node->HandleMessage(sourceLevel,sourcePos,destLevel,destPos,flag);
      }
      else if(message.substr(begin,end-begin) == "KeepAlive")//add by pannian
      {
        send(sock,"KeepAlive|0|0;",14,0);
        m_node->RecordKeepAliveFlag(sock);
        // Logfout << GetNow() << "Server send KeepAlive to Node......" << std::endl;
      }//end
      else if(message.substr(begin,end-begin) == "GoodBye") break;
      else Logfout << "TCPServerRoute::HandleRead (): Invalid Message."<<std::endl;
      start = finish + 1;
      finish = messageSet.find(';',start);
    }
  }
  Logfout.close();
}

void*
TCPServerRoute::KeepAliveThread(void* object)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "KeepAliveProcess" << std::endl;
  TCPServerRoute *pThis=(TCPServerRoute *)object;
  while(1)
  {
    m_node->CheckKeepAliveFlag();//定期检查所有连接的Node
    sleep(pThis->m_defaultKeepaliveTimer);
  }
}

void 
TCPServerRoute::HandleRead ()
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  Logfout << GetNow() << "Server created thread HandleRead successfully......" << std::endl;
  /*监听连接请求--监听队列长度为5*/
  listen(server_sockfd,100);

  TCPServerRoute* object=this;
  if(pthread_create(&keepalive_thread,NULL,KeepAliveThread,(void*)object)<0)
  {
    Logfout << GetNow() << "could not create keepalive thread" << endl;
    return; 
  }

  while(1)
  {
    sin_size=sizeof(struct sockaddr_in);

    /*等待客户端连接请求到达*/

    int client_sockfd = -1;
    if ((client_sockfd=accept(server_sockfd,(struct sockaddr *)&(remote_addr[client_count]),&sin_size))==-1)
    {
      Logfout<<GetNow()<<"TCPServerRoute::HandleRead (): Accept Socket Failed."<<std::endl;
      return;
    }

    int nRecvBuf=1024*1024;
    setsockopt(client_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
    //发送缓冲区
    int nSendBuf=1024*1024;//设置为1M
    setsockopt(client_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

      
    client_sockfds[client_count] = client_sockfd; 
    char *ip = inet_ntoa(remote_addr[client_count].sin_addr);
    Logfout << GetNow() << "Connect with [" << ip << "] and client_sockfd is " << client_sockfd <<std::endl;
    int clientIndex = client_count;
    client_count++;

    //add by hua
    client_sockfd_ptr=(int*)malloc(sizeof(int));
    *client_sockfd_ptr = client_sockfd;
    if(pthread_create(&server_thread,NULL,ServerThread,(void*)client_sockfd_ptr)<0)
    {
      perror("could not create server thread");
      return; 
    }
  }
  Logfout.close();
}

//modified by zhou
void 
TCPServerRoute::NoticeToNode(int sourceLevel,int sourcePos,int destLevel,int destPos,bool flag,vector<int> sock)
{
  ofstream Logfout("/var/log/Primus.log",ios::app);
  string fill;
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

  int dataSize = fill.size () + 1;
  if (dataSize != m_dataSize)
  {
    delete [] m_data;
    m_data = new char [dataSize];
    m_dataSize = dataSize;
  }
  memcpy (m_data, fill.c_str (), dataSize);
  m_dataSize = dataSize;

  for (int i=0;i<sock.size();i++)
  {
    int k=send(sock[i],m_data,m_dataSize,0);
    //处理异常
    if(k>0) Logfout<< GetNow() << "IssueInformation to switch:" << m_data << " client_sockfd is " << sock[i] << std::endl; 
    else if (k==0) Logfout << GetNow() << "client_sockfd " << sock[i] << "off" << std::endl; 
    else if (k<0) Logfout << GetNow() << "client_sockfd " << sock[i] << "error" << std::endl; 
  } 
  Logfout << std::endl;
  Logfout.close();
}

string
TCPServerRoute::GetNow(){
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );

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