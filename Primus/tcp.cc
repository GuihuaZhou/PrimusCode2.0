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

TCPRoute::TCPRoute(Ipv4GlobalRouting *tempGlobalRouting,int tempDefaultKeepaliveTimer,bool tempChiefMaster)
{
	m_globalRouting=tempGlobalRouting;
	m_defaultKeepaliveTimer=tempDefaultKeepaliveTimer;
	myIdent=m_globalRouting->GetMyIdent();
  chiefMaster=tempChiefMaster;
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

int
TCPRoute::SendMessageTo(int sock,struct MNinfo tempMNInfo)// Master和Node都可以调用
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  char sendBuf[MNINFO_BUF_SIZE];
  memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));
  int value=0;
  
  if((value=send(sock,sendBuf,sizeof(struct MNinfo),0))<=0)
  {
    Logfout << GetNow() << "Send message error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
  }
  Logfout.close();
  return value;
}

void* 
TCPRoute::ServerThread(void* tempThreadParam)
{
  // pthread_detach(pthread_self());
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  
  TCPRoute *tempTCPRoute=((struct threadparamA *)tempThreadParam)->tempTCPRoute;
  int sock=((struct threadparamA *)tempThreadParam)->tempSock;
  struct sockaddr_in tempAddr=((struct threadparamA *)tempThreadParam)->tempAddr;

  int value=0;
  char recvBuf[MNINFO_BUF_SIZE];

  while(1)
  {
    memset(recvBuf,'\0',sizeof(recvBuf));
    if((value=recv(sock,recvBuf,MNINFO_BUF_SIZE,0))<=0)
    {
      Logfout << GetNow() << "Sock(" << sock << ") recv error:" << strerror(errno) << "(errno:" << errno << "),";
      break;
    }
    struct MNinfo tempMNInfo;
    memcpy(&tempMNInfo,recvBuf,sizeof(struct MNinfo));

    // Logfout << GetNow() << "Recv message" << "[value:" << value << "]." << endl;

    if (tempMNInfo.clusterMaster==true)// master内部通讯用
    {
      // Logfout << GetNow() << "clusterMaster message." << endl;
      if (tempMNInfo.pathNodeIdentA.level!=-1)
      {
        // tempMNInfo.pathNodeIdentA的indirnodenum发生了变化
        struct clustermasterinfo tempClusterMasterInfo;
        tempClusterMasterInfo.chiefMaster=tempMNInfo.chiefMaster;// 
        tempClusterMasterInfo.masterAddr=tempMNInfo.addr;// master的地址
        tempClusterMasterInfo.masterIdent=tempMNInfo.pathNodeIdentA;// master的ident
        tempClusterMasterInfo.inDirNodeNum=tempMNInfo.pathNodeIdentB.level;
        Logfout << GetNow() << "Recv master " << tempClusterMasterInfo.masterIdent.level << "." << tempClusterMasterInfo.masterIdent.position << "'s inDirNodeNum:" <<  tempClusterMasterInfo.inDirNodeNum << "[value:" << value << "]." << endl;

        m_globalRouting->UpdateClusterMasterInfo(tempClusterMasterInfo,1);
      }
      else if (tempMNInfo.pathNodeIdentA.level==-1)// common 收到信息，说明正在进行新的chiefmaster选举
      {
        Logfout << GetNow() << "Recv cluster master message from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "." << endl;
        m_globalRouting->NewChiefMasterElection(tempMNInfo.srcIdent);
      }
    }
    // master和node都可调用此部分代码
    else if (tempMNInfo.keepAlive==true)//keepalive信息
    {
      // destident就是自己
      if (tempTCPRoute->myIdent.level==tempMNInfo.destIdent.level && tempTCPRoute->myIdent.position==tempMNInfo.destIdent.position)
      {
        // 只有chief master会回复所有的keep alive，备份的master则只回复node发送的keep alive
        if ((tempTCPRoute->myIdent.level==0) && ((!tempTCPRoute->chiefMaster && tempMNInfo.srcIdent.level!=0) || (tempTCPRoute->chiefMaster)))
        {
          m_globalRouting->UpdateKeepAliveFaildNum(tempMNInfo.srcIdent);// 清零
          ident tempIdent=tempMNInfo.destIdent;
          tempMNInfo.destIdent=tempMNInfo.srcIdent;
          tempMNInfo.srcIdent=tempIdent;
          tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成master的ident
          tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;

          value=tempTCPRoute->SendMessageTo(sock,tempMNInfo);
          // Logfout << GetNow() << "Reply keepAlive to ";
          // if (tempMNInfo.destIdent.level==0) Logfout << "master ";
          // else Logfout << "node ";
          // Logfout << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;
        }
        else // node和备份master收到回复的keep alive
        {
          // Logfout << GetNow() << "Recv keepAlive from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "." << endl;
          m_globalRouting->UpdateKeepAlive(sock,tempMNInfo.srcIdent,true);
        }
      }
      // node转发keepalive信息，分master-->node和node-->master两种
      else if (tempTCPRoute->myIdent.level!=tempMNInfo.destIdent.level || tempTCPRoute->myIdent.position!=tempMNInfo.destIdent.position)
      {
        // Logfout << GetNow() << "forward keepAlive message" << "[value:" << value << "]." << endl;
        if (tempMNInfo.destIdent.level==0 || tempMNInfo.destIdent.level==-1)//node-->master
        {
          // Logfout << GetNow() << "Forward keepAlive from node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
          vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
          // tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成转发node的ident，表示间接连接
          // tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
          for (int i=0;i<tempMasterMapToSock.size();i++)// 查找目的ident的套接字
          {
            if (tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true && tempMasterMapToSock[i].masterIdent.level==tempMNInfo.destIdent.level && tempMasterMapToSock[i].masterIdent.position==tempMNInfo.destIdent.position)// 首先本node和该master的连接必须有效，然后判断目的ident
            {
              value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
              // Logfout << GetNow() << "Forward keepAlive from node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;
              break;
            }
          }
          // 需要考虑没有发送成功怎么办，让林老板来写吧
        }
        else//master-->node
        {
          // Logfout << GetNow() << "Forward keepAlive from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
          vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();//查找本node连接了哪些node
          // tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成转发node的ident，表示间接连接
          // tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
          for (int i=0;i<tempNodeMapToSock.size();i++)// 查找目的ident的套接字
          {
            if (tempNodeMapToSock[i].nodeIdent.level==tempMNInfo.destIdent.level && tempNodeMapToSock[i].nodeIdent.position==tempMNInfo.destIdent.position)// 判断目的ident
            {
              value=tempTCPRoute->SendMessageTo(tempNodeMapToSock[i].nodeSock,tempMNInfo);
              // Logfout << GetNow() << "Forward keepAlive from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;
              break;
            }
          }
          // 需要考虑没有发送成功怎么办，让林老板来写吧
        }
      }
    }
    else if (tempMNInfo.hello==true)//node的hello信息，node也有可能收到，因为它为其他节点转发
    {
      Logfout << GetNow() << "Recv hello from ";
      if (tempMNInfo.srcIdent.level==0) Logfout << "master ";
      else Logfout << "node ";
      Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;
      if (tempTCPRoute->myIdent.level==0)// master收到hello
      {
        bool direct=false;
        int inDirNodeNum=0;
        if (tempMNInfo.srcIdent.level==0) inDirNodeNum=tempMNInfo.pathNodeIdentB.level;// 记录common master的indirnodenum
        // 表示直连
        if (tempMNInfo.srcIdent.level==tempMNInfo.pathNodeIdentB.level && tempMNInfo.srcIdent.position==tempMNInfo.pathNodeIdentB.position) 
        {
          direct=true; 
          Logfout << "(direct)";
        }
        // 间接连接，间接连接时会上报间接路径
        else 
        {
          direct=false;
          Logfout << "(inDirect)";
          // master需要记录间接路径
          m_globalRouting->UpdateInDirPath(tempMNInfo.srcIdent,tempMNInfo.pathNodeIdentA,tempMNInfo.pathNodeIdentB,sock);
        }
        Logfout << "[value:" << value << "]." << endl;
        
        m_globalRouting->GetAddrByNICName(&(tempMNInfo.addr),"eth0");
        tempMNInfo.destIdent=tempMNInfo.srcIdent;
        tempMNInfo.srcIdent=tempTCPRoute->myIdent;
        if (direct)// 直接连接则修改，间接不改
        {
          tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成master的ident
          tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
        }
        if (tempTCPRoute->chiefMaster) tempMNInfo.chiefMaster=true; // chiefMaster在回复ACK时要表明自己的身份
        tempMNInfo.hello=false;
        tempMNInfo.ACK=true;
        
        value=tempTCPRoute->SendMessageTo(sock,tempMNInfo);
        Logfout << GetNow() << "Send ACK to ";
        if (tempMNInfo.destIdent.level==0) Logfout << "master ";
        else Logfout << "node ";
        Logfout << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;

        m_globalRouting->UpdateNodeMapToSock(tempMNInfo.destIdent,sock,direct);
      }
      else //node也有可能收到hello，因为它为其他节点转发
      {
        Logfout << "(forward)" << "[value:" << value << "]." << endl;
        // 获得本node和master的连接信息
        vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
        // tempMNInfo.pathNodeIdentA=tempTCPRoute->myIdent;// 换成转发node的ident，表示间接连接
        // tempMNInfo.pathNodeIdentB=tempTCPRoute->myIdent;
        for (int i=0;i<tempMasterMapToSock.size();i++)// 查找目的ident的套接字
        {
          if (tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true)// 首先本node和该master的连接必须有效
          {
            // 此时不需要判断目的ident，全部转发即可
            value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
            Logfout << GetNow() << "Forward Hello to master " << tempMasterMapToSock[i].masterIdent.level << "." << tempMasterMapToSock[i].masterIdent.position << "[value:" << value << "]." << endl;
            m_globalRouting->UpdateNodeMapToSock(tempMNInfo.srcIdent,sock,true);// 此处必定是直连 
          }
          else//无效该怎么处理，让林老板来写吧
          {
            Logfout << "2" << endl;
          }
        }
      }
    }
    else if (tempMNInfo.ACK==true)//node收到 ACK信息
    {
      // node收到master或者中间节点转发的ACK，故不再转发
      // Logfout << GetNow() << "Recv ACK from " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[value:" << value << "]." << endl;
      if (tempMNInfo.destIdent.level==tempTCPRoute->myIdent.level && tempMNInfo.destIdent.position==tempTCPRoute->myIdent.position)
      {
        Logfout << GetNow() << "Recv ACK from Master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[value:" << value << "]." << endl;
        // node收到ACK后需要判断
        // 如果是直连ACK或者chief master重新选举的ACK，不需要继续发送
        struct mastermaptosock tempMasterMapToSock;
        tempMasterMapToSock.chiefMaster=tempMNInfo.chiefMaster;// 这个有效
        tempMasterMapToSock.masterAddr=inet_ntoa(tempMNInfo.addr.sin_addr);// 这个也有效
        tempMasterMapToSock.masterIdent=tempMNInfo.srcIdent;// 这个有效
        tempMasterMapToSock.masterSock=sock;// 有效
        tempMasterMapToSock.NICName="";
        if (m_globalRouting->SameNode(tempMNInfo.srcIdent,tempMNInfo.pathNodeIdentA))// 直接连接
        {
          tempMasterMapToSock.direct=true;
        }
        else tempMasterMapToSock.direct=false;
        tempMasterMapToSock.middleAddr="";
        tempMasterMapToSock.keepAliveFaildNum=0;
        tempMasterMapToSock.recvKeepAlive=false;
        tempMasterMapToSock.isStartKeepAlive=false;
        m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);
        
        // 如果是发送间接路径的ACK包还要判断是否继续发送
        if (tempMNInfo.chiefMaster && tempTCPRoute->myIdent.level!=0 && tempMasterMapToSock.direct==false)
        {
          m_globalRouting->SendInDirPathToMaster(sock,tempMNInfo.pathNodeIdentA,tempMNInfo.pathNodeIdentB); 
        }    
        else if (tempTCPRoute->myIdent.level==0 && !tempTCPRoute->chiefMaster)// common master收到ack后要主动向chief 发送indirnodenum
        {
          // common master收到ack后主动发送ClusterMasterInfo
          struct clustermasterinfo tempClusterMasterInfo;
          tempClusterMasterInfo.chiefMaster=false;
          m_globalRouting->GetAddrByNICName(&(tempClusterMasterInfo.masterAddr),"eth0");// common master的地址
          // tempClusterMasterInfo.masterAddr=tempMNInfo.addr;
          tempClusterMasterInfo.masterIdent=tempTCPRoute->myIdent;// common master的ident
          tempClusterMasterInfo.inDirNodeNum=m_globalRouting->GetInDirNodeNum();

          m_globalRouting->SendInDirNodeNumToCommon(tempClusterMasterInfo);
        }
      }
      else
      {
        // 需要继续转发
        vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();//查找本node作为转发node连接了哪些node
        for (int i=0;i<tempNodeMapToSock.size();i++)
        {
          if (tempNodeMapToSock[i].nodeIdent.level==tempMNInfo.destIdent.level && tempNodeMapToSock[i].nodeIdent.position==tempMNInfo.destIdent.position)
          {
            value=tempTCPRoute->SendMessageTo(tempNodeMapToSock[i].nodeSock,tempMNInfo);
            Logfout << GetNow() << "Forward ACK from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;
            break;
          }
        }
      }
    }
    else if (tempMNInfo.chiefMaster==false && tempMNInfo.keepAlive==false && tempMNInfo.hello==false && tempMNInfo.ACK==false && tempMNInfo.clusterMaster==false)// 链路信息也有可能需要转发
    {
      // Logfout << GetNow() << "linkInfo message[value:" << value << "]." << endl;
      // 目的node就是本node或Master
      if ((tempMNInfo.destIdent.level==tempTCPRoute->myIdent.level && tempMNInfo.destIdent.position==tempTCPRoute->myIdent.position) || (tempMNInfo.destIdent.level==-1 && tempMNInfo.destIdent.position==-1 && tempTCPRoute->myIdent.level==0))// 无需转发
      {
        Logfout << GetNow();
        if (tempTCPRoute->myIdent.level==0) Logfout << "Master ";
        else Logfout << "Node ";
        Logfout << tempTCPRoute->myIdent.level << "." << tempTCPRoute->myIdent.position << " recv ";
        Logfout << tempMNInfo.pathNodeIdentA.level << "." << tempMNInfo.pathNodeIdentA.position << "--" << tempMNInfo.pathNodeIdentB.level << "." << tempMNInfo.pathNodeIdentB.position;
        if (tempMNInfo.linkFlag==true) Logfout << " up ";
        else if (tempMNInfo.linkFlag==false) Logfout << " down ";
        Logfout << "from ";
        if (tempMNInfo.srcIdent.level==0) Logfout << "Master ";
        else Logfout << "Node ";
        Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[value:" << value << "].";
        Logfout << endl;

        m_globalRouting->HandleMessage(tempMNInfo.pathNodeIdentA,tempMNInfo.pathNodeIdentB,tempMNInfo.linkFlag);
        
        if (tempMNInfo.srcIdent.level!=0)// 此信息不是master发来的，则需要转发给其他master
        {
          if (tempTCPRoute->myIdent.level==0)
          {
            // chief转发给backup master，
            if (tempTCPRoute->chiefMaster)
            {
              vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();
              tempMNInfo.srcIdent=tempTCPRoute->myIdent;
              for (int i=0;i<tempNodeMapToSock.size();i++)
              {
                if (tempNodeMapToSock[i].nodeIdent.level==0)
                {
                  tempMNInfo.destIdent=tempNodeMapToSock[i].nodeIdent;
                  value=tempTCPRoute->SendMessageTo(tempNodeMapToSock[i].nodeSock,tempMNInfo);
                  // Logfout << GetNow() << "Forward linkInfo to common master " << tempNodeMapToSock[i].nodeIdent.level << "." << tempNodeMapToSock[i].nodeIdent.position << "[value:" << value << "]." << endl;
                }
                else break;
              }
            }
            else//backup master转发给chief
            {
              vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();
              tempMNInfo.srcIdent=tempTCPRoute->myIdent;
              for (int i=0;i<tempMasterMapToSock.size();i++)
              {
                if (tempMasterMapToSock[i].masterIdent.level==0)
                {
                  tempMNInfo.destIdent=tempMasterMapToSock[i].masterIdent;
                  value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
                  // Logfout << GetNow() << "Forward linkInfo to chief master " << tempMasterMapToSock[i].masterIdent.level << "." << tempMasterMapToSock[i].masterIdent.position << "[value:" << value << "]." << endl;
                }
                else break;
              }
            }
          }
        }
      }
      else// node需要转发，-1，-1表示发给所有的master
      {
        // Logfout << "forward from " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to (master/node) " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << endl;
        // 转发给所有的master，比如直连刚断掉，node会上传直连断掉的信息，但此时masterSock里的ident还未初始化完成
        if (tempMNInfo.destIdent.level==-1 && tempMNInfo.destIdent.position==-1 && tempTCPRoute->myIdent.level!=0)
        {
          vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
          for (int i=0;i<tempMasterMapToSock.size();i++)
          {
            if (tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true)// 首先本node和该master的连接必须有效
            {
              value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
              Logfout << GetNow() << "Forward linkInfo to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;
              break;
            }
          }
          // 没找到？
        }
        // 转发给node
        else if (tempTCPRoute->myIdent.level!=0)
        {
          vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();//查找本node作为转发node连接了哪些node
          bool isFind=false;
          for (int i=0;i<tempNodeMapToSock.size();i++)
          {
            if (tempNodeMapToSock[i].nodeIdent.level==tempMNInfo.destIdent.level && tempNodeMapToSock[i].nodeIdent.position==tempMNInfo.destIdent.position)
            {
              value=tempTCPRoute->SendMessageTo(tempNodeMapToSock[i].nodeSock,tempMNInfo);
              Logfout << GetNow() << "Forward linkInfo from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "]." << endl;
              isFind=true;
              break;
            }
          }
          // 没找到，说明这是tempMNInfo.destIdent的间接连接失效，Master正在尝试让其重新连接，需要通过UDP通知tempMNInfo.destIdent
          if (isFind==false)
          {
            if (m_globalRouting->InformUnreachableNode(tempMNInfo.destIdent,tempMNInfo.srcIdent))// 可以回复master，但是MNinfo真的需要重新设计
            {
              Logfout << GetNow() << "Yes! I can inform node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
            }
            else
            {
              Logfout << GetNow() << "Sorry! I can not inform node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "." << endl;
            }
          }
        }
      }
    }
    else
    {
      Logfout << GetNow() << "Other message" << "[value:" << value << "]." << endl;
      // 应该处理一下非法信息
    }
  }
  // shutdown(sock,SHUT_RDWR);
  Logfout << "TCPServerThread[sock:" << sock << "] thread down[value:" << value << "]." << endl;
  Logfout.close();
  // pthread_exit(0);
}

void 
TCPRoute::UpdateChiefMaster(bool tempChiefMaster)
{
  chiefMaster=tempChiefMaster;
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
  serverAddr.sin_port=htons(MN_PORT);
  // sonic test
  // if (myIdent.level==0) serverAddr.sin_port=htons(MN_PORT);
  // else serverAddr.sin_port=htons(myIdent.level*1000+myIdent.position*100);// node 作转发时监听了整个交换机，所以设置不同的端口号
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

  listen(serverSock,1024);

  Logfout << GetNow() << "TCPRoute start listen[serverSock:" << serverSock << "]......" << endl;
  while (1)
  {
    struct sockaddr_in clientAddr;
    unsigned sin_size=sizeof(struct sockaddr_in);
    /*等待客户端连接请求到达*/
    if ((clientSock=accept(serverSock,(struct sockaddr *)&(clientAddr),&sin_size))==-1)
    {
      Logfout << GetNow() << "Sock(" << clientSock << ") accept socket error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
      continue;
    }

    int nRecvBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
    //发送缓冲区
    int nSendBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

    Logfout << GetNow() << "Connect with [" << inet_ntoa(clientAddr.sin_addr) << "][clientSock:" << clientSock << "]." << endl;
   
    struct threadparamA *tempThreadParam=(struct threadparamA *)malloc(sizeof(struct threadparamA));
    tempThreadParam->tempTCPRoute=this;
    tempThreadParam->tempAddr=clientAddr;
    tempThreadParam->tempSock=clientSock;

    if(pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
    {
      Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
      break;
    }
    // pthread_detach(server_thread);
  }
  Logfout << GetNow() << "TCPRoute listen thread[serverSock:" << serverSock << "] down." << endl;
  Logfout.close();
}

void
TCPRoute::SendHelloToMaster(vector<string> masterAddress,string middleAddress,string NICName)// 可能是直连，或者是通过中间结点间接连接，所以masterAddress里也可能是中间结点的地址
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
    masterAddr.sin_port=htons(MN_PORT);
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
      Logfout << GetNow() << "Sock(" << clientSock << ") connect error:" << strerror(errno) << "(errno:" << errno << ")." << endl;
      exit(0);
    }

    struct mastermaptosock tempMasterMapToSock;
    tempMasterMapToSock.chiefMaster=false;
    tempMasterMapToSock.masterIdent.level=-1;// 等收到ACK后再填写
    tempMasterMapToSock.masterIdent.position=-1;
    tempMasterMapToSock.masterSock=clientSock;// 有效
    tempMasterMapToSock.NICName=NICName;// 有效
    tempMasterMapToSock.direct=false;// 有效
    tempMasterMapToSock.middleAddr=middleAddress;// 有效
    tempMasterMapToSock.keepAliveFaildNum=0;// 有效
    tempMasterMapToSock.recvKeepAlive=false;// 有效
    tempMasterMapToSock.isStartKeepAlive=false;

    for (int i=0;i<masterAddress.size();i++)
    {
      tempMasterMapToSock.masterAddr=masterAddress[i];
      m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);
    }      

    // 为该套接字创建接收线程
    struct threadparamA *tempThreadParam=(struct threadparamA *)malloc(sizeof(struct threadparamA));
    tempThreadParam->tempTCPRoute=this;
    tempThreadParam->tempAddr=masterAddr;
    tempThreadParam->tempSock=clientSock;

    if(pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
    {
      Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
      exit(0);
    }
    // pthread_detach(server_thread);

    Logfout << GetNow() << "Try to connected with forward node[" << inet_ntoa(masterAddr.sin_addr) << "][sock:" << clientSock << "]." << endl;

    m_globalRouting->SendInDirPathToMaster(clientSock,tempMasterMapToSock.masterIdent,tempMasterMapToSock.masterIdent);// 通过hello包来发送路径
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
        Logfout << GetNow() << "Sock(" << clientSock << ") connect error:" << strerror(errno) << "(errno:" << errno << ")." << endl;
        exit(0);
      }

      struct mastermaptosock tempMasterMapToSock;
      tempMasterMapToSock.chiefMaster=false;
      tempMasterMapToSock.masterAddr=masterAddress[i];
      tempMasterMapToSock.masterIdent.level=-1;// 等收到ACK后再填写
      tempMasterMapToSock.masterIdent.position=-1;
      tempMasterMapToSock.masterSock=clientSock;
      tempMasterMapToSock.NICName=NICName;
      tempMasterMapToSock.direct=true;
      tempMasterMapToSock.middleAddr="";
      tempMasterMapToSock.keepAliveFaildNum=0;
      tempMasterMapToSock.recvKeepAlive=false;
      tempMasterMapToSock.isStartKeepAlive=false;

      m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);

      // 为该套接字创建接收线程
      struct threadparamA *tempThreadParam=(struct threadparamA *)malloc(sizeof(struct threadparamA));
      tempThreadParam->tempTCPRoute=this;
      tempThreadParam->tempAddr=masterAddr;
      tempThreadParam->tempSock=clientSock;

      if(pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
      {
        Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
        break;
      }
      // pthread_detach(server_thread);

      Logfout << GetNow() << "Try to connected with master[" << inet_ntoa(masterAddr.sin_addr) << "][sock:" << clientSock << "]." << endl;

      struct MNinfo helloMNIfo;
      helloMNIfo.addr.sin_family=AF_INET;
      inet_aton("255.255.255.255",&(helloMNIfo.addr.sin_addr));
      helloMNIfo.addr.sin_port=htons(0);
      m_globalRouting->GetAddrByNICName(&(helloMNIfo.addr),"eth0");
      helloMNIfo.destIdent.level=-1;// 此时还不知道master的ident
      helloMNIfo.destIdent.position=-1;
      helloMNIfo.srcIdent=myIdent;
      helloMNIfo.pathNodeIdentA=myIdent;
      helloMNIfo.pathNodeIdentB=myIdent;
      helloMNIfo.clusterMaster=false;
      helloMNIfo.chiefMaster=chiefMaster;
      helloMNIfo.keepAlive=false;
      helloMNIfo.linkFlag=false;
      helloMNIfo.hello=true;
      helloMNIfo.ACK=false;

      SendMessageTo(clientSock,helloMNIfo);
    }
  }
  Logfout.close();
}

void* 
TCPRoute::SendHelloToChiefThread(void* tempThreadParam)
{
  // pthread_detach(pthread_self());
  TCPRoute *tempTCPRoute=((struct threadparamB *)tempThreadParam)->tempTCPRoute;
  vector<string> masterAddress;
  masterAddress.push_back(((struct threadparamB *)tempThreadParam)->masterAddress);
  string NICName=((struct threadparamB *)tempThreadParam)->NICName;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempTCPRoute->myIdent.level << "." << tempTCPRoute->myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  int interval=1;//单位s
  int failCounter=0;//超出10次就挂了

  while (1)
  {
    tempTCPRoute->SendHelloToMaster(masterAddress,"",NICName);
    sleep(interval);
    interval*=2;// double
    vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();
    if (tempMasterMapToSock.size()>0)
    {
      if (tempMasterMapToSock[0].masterIdent.level!=-1 && tempMasterMapToSock[0].masterIdent.position!=-1)
      {
        Logfout << GetNow() << "Connect with chiefMaster[" << masterAddress[0] << "]......" << endl;
        break;
      }
      else continue;
    }
    else continue;
  }
  Logfout.close();
  // pthread_exit(0);
}

void // master尝试和chiefMaster建立连接
TCPRoute::SendHelloToChief(string masterAddress,string NICName)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // struct threadparamB *tempThreadParam=(struct threadparamB *)malloc(sizeof(struct threadparamB));
  threadparamB *tempThreadParam=new threadparamB();
  tempThreadParam->tempTCPRoute=this;
  tempThreadParam->masterAddress=masterAddress;
  tempThreadParam->NICName=NICName;

  if (pthread_create(&hellotochief_thread,NULL,SendHelloToChiefThread,(void *)tempThreadParam)<0)
  {
    Logfout << GetNow() << "Create thread to connect with chiefMaster failed!!!!!!!!!" << endl;
    exit(0);
  }
  // pthread_detach(hellotochief_thread);
  Logfout.close();
}