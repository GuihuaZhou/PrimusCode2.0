#include <iostream>
#include <fstream>
#include <pthread.h>
#include "ipv4-global-routing.h"

Ipv4GlobalRouting *
TCPRoute::m_globalRouting=NULL;

pthread_mutex_t mutexB;

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
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  char sendBuf[MNINFO_BUF_SIZE];
  memcpy(sendBuf,&tempMNInfo,sizeof(struct MNinfo));
  int value=0;
  
  if((value=send(sock,sendBuf,sizeof(struct MNinfo),0))<=0)
  {
    // Logfout << GetNow() << "Send message error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
  }
  // Logfout.close();
  return value;
}

void* 
TCPRoute::ServerThread(void* tempThreadParam)
{
  // pthread_detach(pthread_self());
  // stringstream logFoutPath;
  // logFoutPath.str("");
  // logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  // ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  
  TCPRoute *tempTCPRoute=((struct threadparamA *)tempThreadParam)->tempTCPRoute;
  int sock=((struct threadparamA *)tempThreadParam)->tempSock;
  struct sockaddr_in tempAddr=((struct threadparamA *)tempThreadParam)->tempAddr;// 另一端地址

  int value=0;
  char recvBuf[MNINFO_BUF_SIZE];

  while(1)
  {
    memset(recvBuf,'\0',sizeof(recvBuf));
    if((value=recv(sock,recvBuf,MNINFO_BUF_SIZE,0))<=0)
    {
      // Logfout << GetNow() << "Sock(" << sock << ") recv error:" << strerror(errno) << "(errno:" << errno << "),";
      break;
    }
    struct MNinfo tempMNInfo;
    memcpy(&tempMNInfo,recvBuf,sizeof(struct MNinfo));

    // Logfout << GetNow() << "Recv message" << "[value:" << value << "]." << endl;

    if (tempMNInfo.bye==true)
    {
      // Logfout << GetNow() << "Recv bye from " << inet_ntoa(tempAddr.sin_addr) << "." << endl;
      shutdown(sock,SHUT_RDWR);
      // Logfout << GetNow() << "Sock(" << sock << ") recv error:" << strerror(errno) << "(errno:" << errno << "),";
      break;
    }

    if (tempMNInfo.reachable==true)// 可达，一般情况
    {
      if (tempMNInfo.clusterMaster==true)// master内部通讯用
      {
        // Logfout << GetNow() << "clusterMaster message." << endl;
        if (tempMNInfo.pathNodeIdent[0].level!=-1)
        {
          // tempMNInfo.pathNodeIdentA的indirnodenum发生了变化
          struct clustermasterinfo tempClusterMasterInfo;
          tempClusterMasterInfo.chiefMaster=tempMNInfo.chiefMaster;// 
          tempClusterMasterInfo.masterAddr=tempMNInfo.addr;// master的地址
          tempClusterMasterInfo.masterIdent=tempMNInfo.pathNodeIdent[0];// master的ident
          tempClusterMasterInfo.inDirNodeNum=tempMNInfo.pathNodeIdent[1].level;
          // Logfout << GetNow() << "Recv master " << tempClusterMasterInfo.masterIdent.level << "." << tempClusterMasterInfo.masterIdent.position << "'s inDirNodeNum:" <<  tempClusterMasterInfo.inDirNodeNum << "[value:" << value << "][sock:" << sock << "]." << endl;

          m_globalRouting->UpdateClusterMasterInfo(tempClusterMasterInfo,1);
        }
        else if (tempMNInfo.pathNodeIdent[0].level==-1)// common 收到信息，说明正在进行新的chiefmaster选举
        {
          // Logfout << GetNow() << "Recv cluster master message from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[sock:" << sock << "]." << endl;
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
            tempMNInfo.pathNodeIdent[0]=tempTCPRoute->myIdent;// 换成master的ident
            tempMNInfo.pathNodeIdent[1]=tempTCPRoute->myIdent;

            value=tempTCPRoute->SendMessageTo(sock,tempMNInfo);
            // Logfout << GetNow() << "Reply keepAlive to ";
            // if (tempMNInfo.destIdent.level==0) Logfout << "master ";
            // else Logfout << "node ";
            // Logfout << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
          }
          else // node和备份master收到回复的keep alive
          {
            // Logfout << GetNow() << "Recv keepAlive from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[sock:" << sock << "]." << endl;
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
        if (tempMNInfo.ACK==false)// 不是hello的ack
        {
          // Logfout << GetNow() << "Recv hello from ";
          // if (tempMNInfo.srcIdent.level==0) Logfout << "master ";
          // else Logfout << "node ";
          // Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position;

          if (tempTCPRoute->myIdent.level==0 || m_globalRouting->SameNode(tempTCPRoute->myIdent,tempMNInfo.destIdent))// master收到hello
          {
            bool direct=false;
            int inDirNodeNum=0;
            if (tempMNInfo.srcIdent.level==0) inDirNodeNum=tempMNInfo.pathNodeIdent[1].level;// 记录common master的indirnodenum
            // 表示直连
            if (tempMNInfo.srcIdent.level==tempMNInfo.pathNodeIdent[1].level && tempMNInfo.srcIdent.position==tempMNInfo.pathNodeIdent[1].position) 
            {
              direct=true; 
              // Logfout << "(direct)";
            }
            // 间接连接，间接连接时会上报间接路径
            else 
            {
              direct=false;
              // Logfout << "(inDirect)";
              // master需要记录间接路径
              m_globalRouting->UpdateInDirPath(tempMNInfo.srcIdent,tempMNInfo.pathNodeIdent,sock);
            }
            // Logfout << "[value:" << value << "][sock:" << sock << "]." << endl;

            if (tempTCPRoute->myIdent.level!=0) //Logfout << GetNow() << "I should connect with master by another indirpath[sock:" << sock << "]." << endl;

            if (tempTCPRoute->myIdent.level==0) tempMNInfo.addr=*(m_globalRouting->GetAddrByNICName(MGMT_INTERFACE));// 此处有问题，应该是那个网口连接的，就返回那个网口的地址，但是做不到
            tempMNInfo.destIdent=tempMNInfo.srcIdent;
            tempMNInfo.srcIdent=tempTCPRoute->myIdent;
            if (direct)// 直接连接则修改，间接不改
            {
              tempMNInfo.pathNodeIdent[0]=tempTCPRoute->myIdent;// 换成master的ident
              tempMNInfo.pathNodeIdent[1]=tempTCPRoute->myIdent;
            }
            tempMNInfo.chiefMaster=tempTCPRoute->chiefMaster; // chiefMaster在回复ACK时要表明自己的身份
            tempMNInfo.ACK=true;

            value=tempTCPRoute->SendMessageTo(sock,tempMNInfo);
            // Logfout << GetNow() << "Send ACK to ";
            // if (tempMNInfo.destIdent.level==0) Logfout << "master ";
            // else Logfout << "node ";
            // Logfout << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;

            m_globalRouting->UpdateNodeMapToSock(tempMNInfo.destIdent,inet_ntoa(tempAddr.sin_addr),sock,direct);
          }
          else //node也有可能收到hello，因为它为其他节点转发
          {
            // 目的ident不是本node，作转发
            // Logfout << "(forward)" << "[value:" << value << "][sock:" << sock << "]." << endl;
            bool isFind=false;
            // 获得本node和master的连接信息
            vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
            for (int i=0;i<tempMasterMapToSock.size();i++)// 查找目的ident的套接字
            {
              if (tempMasterMapToSock[i].masterAddr==inet_ntoa(tempMNInfo.addr.sin_addr) && tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true && tempMasterMapToSock[i].masterSock>0)// 首先本node和该master的连接必须有效
              {
                // 此时不需要判断目的ident，全部转发即可
                tempMNInfo.destIdent=tempMasterMapToSock[i].masterIdent;
                value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
                // Logfout << GetNow() << "Forward Hello to master " << tempMasterMapToSock[i].masterIdent.level << "." << tempMasterMapToSock[i].masterIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
                m_globalRouting->UpdateNodeMapToSock(tempMNInfo.srcIdent,inet_ntoa(tempAddr.sin_addr),sock,true);// 此处必定是直连 
                isFind=true;
                break;
              }
            }
            if (isFind==false)
            {
              // Logfout << GetNow() << "I try to forward a hello message from node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to master[" << inet_ntoa(tempMNInfo.addr.sin_addr) << "],but there is not a sock to master in my masterMapToSock table." << endl;
            }
          }
        }
        else if (tempMNInfo.ACK==true)// hello 的ack
        {
          // node收到master或者中间节点转发的ACK，故不再转发
          // Logfout << GetNow() << "Recv ACK from " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[value:" << value << "]." << endl;
          if (tempMNInfo.destIdent.level==tempTCPRoute->myIdent.level && tempMNInfo.destIdent.position==tempTCPRoute->myIdent.position)
          {
            // Logfout << GetNow() << "Recv ACK from Master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
            // node收到ACK后需要判断
            // 如果是直连ACK或者chief master重新选举的ACK，不需要继续发送
            struct mastermaptosock tempMasterMapToSock;
            tempMasterMapToSock.chiefMaster=tempMNInfo.chiefMaster;// 这个有效
            tempMasterMapToSock.masterAddr=inet_ntoa(tempMNInfo.addr.sin_addr);// 这个也有效
            tempMasterMapToSock.masterIdent=tempMNInfo.srcIdent;// 这个有效
            tempMasterMapToSock.masterSock=sock;// 有效
            tempMasterMapToSock.NICName="";
            if (m_globalRouting->SameNode(tempMNInfo.srcIdent,tempMNInfo.pathNodeIdent[0]))// 直接连接
            {
              tempMasterMapToSock.direct=true;
            }
            else tempMasterMapToSock.direct=false;
            tempMasterMapToSock.middleAddr="";
            tempMasterMapToSock.keepAliveFaildNum=0;
            tempMasterMapToSock.recvKeepAlive=false;
            tempMasterMapToSock.isStartKeepAlive=false;
            tempMasterMapToSock.inDirPath=NULL;
            m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);
             
            if (tempTCPRoute->myIdent.level==0 && !tempTCPRoute->chiefMaster)// common master收到ack后要主动向chief 发送indirnodenum
            {
              // common master收到ack后主动发送ClusterMasterInfo
              struct clustermasterinfo tempClusterMasterInfo;
              tempClusterMasterInfo.chiefMaster=false;
              tempClusterMasterInfo.masterAddr=*(m_globalRouting->GetAddrByNICName(MGMT_INTERFACE));// common master的地址
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
                // Logfout << GetNow() << "Forward ACK from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
                break;
              }
            }
          }
        }
      }
      else if (tempMNInfo.chiefMaster==false && tempMNInfo.keepAlive==false && tempMNInfo.hello==false && tempMNInfo.clusterMaster==false)// 链路信息也有可能需要转发
      {
        // Logfout << GetNow() << "linkInfo message[value:" << value << "]." << endl;
        // 目的node就是本node或Master
        if ((m_globalRouting->SameNode(tempMNInfo.destIdent,tempTCPRoute->myIdent)) || (tempMNInfo.destIdent.level==-1 && tempMNInfo.destIdent.position==-1 && tempTCPRoute->myIdent.level==0))
        {
          if (tempMNInfo.ACK==false)
          {
            // Logfout << GetNow();
            // if (tempTCPRoute->myIdent.level==0) Logfout << "Master ";
            // else Logfout << "Node ";
            // Logfout << tempTCPRoute->myIdent.level << "." << tempTCPRoute->myIdent.position << " recv ";
            // Logfout << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
            // if (tempMNInfo.linkFlag==true) Logfout << " up ";
            // else if (tempMNInfo.linkFlag==false) Logfout << " down ";
            // Logfout << "from ";
            // if (tempMNInfo.srcIdent.level==0) Logfout << "Master ";
            // else Logfout << "Node ";
            // Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " by tcp [value:" << value << "][eventId:" << tempMNInfo.eventId << "][sock:" << sock << "].";
            // Logfout << endl;

            // struct stampinfo tempStampInfo;
            // tempStampInfo.identA=tempMNInfo.pathNodeIdent[0];
            // tempStampInfo.identB=tempMNInfo.pathNodeIdent[1];
            // tempStampInfo.linkFlag=tempMNInfo.linkFlag;
            // tempStampInfo.note="(tcp) recv info";
            // struct timespec tv;
            // clock_gettime(CLOCK_MONOTONIC,&tv);
            // tempStampInfo.stamp=tv.tv_sec+tv.tv_nsec*0.000000001;
            // m_globalRouting->stampInfo.push_back(tempStampInfo);

            m_globalRouting->HandleMessage(tempMNInfo,"tcp");

            // Logfout << "HandleMessage complete!" << endl;
          }
          else if (tempMNInfo.ACK==true)
          {
            // Logfout << GetNow() << "Recv response from ";
            // if (tempMNInfo.srcIdent.level==0) Logfout << " Master ";
            // else Logfout << " Node ";
            // Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " [eventId:" << tempMNInfo.eventId << "]." << endl;
            
            m_globalRouting->UpdateResponseRecord(tempMNInfo.eventId,tempMNInfo.pathNodeIdent[0],tempMNInfo.pathNodeIdent[1],-1);// 
          }
        }
        else// 只有node才能转发
        {
          // Logfout << "forward from " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to (master/node) " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << endl;
          if (tempTCPRoute->myIdent.level!=0)
          {
            if (tempMNInfo.destIdent.level==-1)// 转发给所有Master
            {
              vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
              for (int i=0;i<tempMasterMapToSock.size();i++)
              {
                if (tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true)// 首先本node和该master的连接必须有效
                {
                  value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
                  // Logfout << GetNow() << "Forward linkInfo from node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.destIdent.position << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
                }
              }
              // 没找到？
            }
            else if (tempMNInfo.destIdent.level==0)// 转发给单个master
            {
              vector<struct mastermaptosock> tempMasterMapToSock=m_globalRouting->GetMasterMapToSock();//查找本node连接了哪些master
              for (int i=0;i<tempMasterMapToSock.size();i++)
              {
                if (tempMasterMapToSock[i].masterIdent.level!=-1 && tempMasterMapToSock[i].direct==true && m_globalRouting->SameNode(tempMasterMapToSock[i].masterIdent,tempMNInfo.destIdent))// 首先本node和该master的连接必须有效
                {
                  value=tempTCPRoute->SendMessageTo(tempMasterMapToSock[i].masterSock,tempMNInfo);
                  // Logfout << GetNow() << "Forward linkInfo from node " << tempMNInfo.srcIdent.level << "." << tempMNInfo.destIdent.position << " to master " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
                  break;
                }
              }
            }
            else // 转发给Node
            {
              vector<struct nodemaptosock> tempNodeMapToSock=m_globalRouting->GetNodeMapToSock();//查找本node作为转发node连接了哪些node
              bool isFind=false;
              for (int i=0;i<tempNodeMapToSock.size();i++)
              {
                if (m_globalRouting->SameNode(tempNodeMapToSock[i].nodeIdent,tempMNInfo.destIdent))
                {
                  value=tempTCPRoute->SendMessageTo(tempNodeMapToSock[i].nodeSock,tempMNInfo);
                  // Logfout << GetNow() << "Forward linkInfo from master " << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " to node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[value:" << value << "][sock:" << sock << "]." << endl;
                  isFind=true;
                  break;
                }
              }
              if (isFind==true) continue;
              // 没找到，说明这是tempMNInfo.destIdent的间接连接失效，Master正在尝试让其重新连接，
              if (isFind==false && m_globalRouting->SameNode(tempMNInfo.forwardIdent,tempTCPRoute->myIdent))
              {
                struct pathtableentry *tempPathTableEntry=m_globalRouting->InformUnreachableNode(tempMNInfo.destIdent,tempMNInfo.srcIdent);
                if (tempPathTableEntry!=NULL)// 可以回复master，但是MNinfo真的需要重新设计
                {
                  int tempSock=0;
                  if ((tempSock=tempTCPRoute->SendHelloToMaster(tempMNInfo.destIdent,inet_ntoa(tempPathTableEntry->nodeAddr.addr.sin_addr),"",m_globalRouting->GetNICNameByRemoteAddr(tempPathTableEntry->nextHopAddr.addr),NULL,true))>0)
                  {
                    tempMNInfo.reachable=true;
                    if ((value=tempTCPRoute->SendMessageTo(tempSock,tempMNInfo))>0)
                    {
                      tempMNInfo.bye=true;// 关闭这个连接
                      if ((value=tempTCPRoute->SendMessageTo(tempSock,tempMNInfo))>0)
                      {
                        // Logfout << GetNow() << "Yes! I can inform node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[sock:" << sock << "]." << endl;
                        continue;
                      }
                    }
                  }
                }
                // Logfout << GetNow() << "Sorry! I can not inform node " << tempMNInfo.destIdent.level << "." << tempMNInfo.destIdent.position << "[sock:" << sock << "]." << endl;
                // 应该返回给master
                ident tempIdent;
                tempIdent=tempMNInfo.srcIdent;
                tempMNInfo.srcIdent=tempMNInfo.destIdent;
                tempMNInfo.destIdent=tempIdent;
                tempMNInfo.reachable=false;
                value=tempTCPRoute->SendMessageTo(sock,tempMNInfo);
              }
              else
              {
                // Logfout << GetNow() << "Other message(1) " << "[value:" << value << "][sock:" << sock << "]." << endl;
              }
            }
          }
        }
      }
      else
      {
        // Logfout << GetNow() << "Other message(2) " << "[value:" << value << "][sock:" << sock << "]." << endl;
        // 应该处理一下非法信息
      }
    }
    else// 中转node返回的不可达message
    {
      if (tempMNInfo.clusterMaster==true)
      {
        // 
      }
      else if (tempMNInfo.hello==true)
      {
        // 
      }
      else if (tempMNInfo.ACK==true)
      {
        // 
      }
      else if (tempMNInfo.chiefMaster==false && tempMNInfo.keepAlive==false && tempMNInfo.hello==false && tempMNInfo.ACK==false && tempMNInfo.clusterMaster==false)// 不可达的链路变化信息
      {
        // Logfout << GetNow() << "Recv message from node " << tempMNInfo.forwardIdent.level << "." << tempMNInfo.forwardIdent.position << " that I need to inform ";
        // if (tempMNInfo.srcIdent.level==0) Logfout << "master ";
        // else Logfout << "node ";
        // Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " again[sock:" << sock << "]." << endl;
        ident tempIdent;
        tempIdent=tempMNInfo.srcIdent;
        tempMNInfo.srcIdent=tempMNInfo.destIdent;
        tempMNInfo.destIdent=tempIdent;
        tempMNInfo.reachable=true;
        m_globalRouting->ChooseNodeToInformInDirNode(tempMNInfo.destIdent,tempMNInfo.forwardIdent,tempMNInfo);
      }
    }
  }

  m_globalRouting->CloseSock(sock);
  // Logfout << "TCPServerThread[sock:" << sock << "] thread down[value:" << value << "]." << endl;
  // Logfout.close();
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

  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "TCPRoute set SO_REUSEADDR error" << endl;
    exit(0);
  }

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
  }
  Logfout << GetNow() << "TCPRoute listen thread[serverSock:" << serverSock << "] down." << endl;
  Logfout.close();
}

int 
TCPRoute::SendHelloToMaster(ident destIdent,string masterAddress,string middleAddress,string NICName,struct pathtableentry *inDirPath,bool direct)// 可能是直连，或者是通过中间结点间接连接，所以masterAddress里也可能是中间结点的地址
{
  // pthread_mutex_lock(&mutexB);
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // 先检查是否有到目的地址的套接字，
  // 如果没有，则先建立tcp连接，再发送hello；如果有，则直接发送hello
  // Logfout << "SendHelloToMaster masterAddress[" << masterAddress << "] middleAddress[" << middleAddress << "] NICName[" << NICName << "]." << endl;

  string tempAddress="";
  int clientSock=0;
  int value=1;
  if (direct==true) tempAddress=masterAddress;
  else tempAddress=middleAddress;

  if ((clientSock=m_globalRouting->GetSockByAddress(tempAddress))<=0)// 不存在，则先建立tcp连接
  {
    Logfout << GetNow() << "There is not sock to [" << tempAddress << "]." << endl; 
    struct sockaddr_in masterAddr;
    memset(&masterAddr,0,sizeof(masterAddr)); //数据初始化--清零
    masterAddr.sin_family=AF_INET; //设置为IP通信
    masterAddr.sin_addr.s_addr=inet_addr(tempAddress.c_str());
    masterAddr.sin_port=htons(MN_PORT);

    if ((clientSock=socket(PF_INET,SOCK_STREAM,0))<0)
    {
      Logfout << GetNow() << "TCPRoute Create Socket Failed." << endl;
      Logfout.close();
      return -1;
    }

    if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)//设置端口复用
    {
      Logfout << GetNow() << "Set SO_REUSEPORT error" << endl;
      Logfout.close();
      return -1;
    }

    if (setsockopt(clientSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
    {
      Logfout << GetNow() << "Set SO_REUSEADDR error" << endl;
      Logfout.close();
      return -1;
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
      Logfout.close();
      return -1;
    }

    if ((connect(clientSock,(const struct sockaddr *)&masterAddr,sizeof(masterAddr)))<0)
    {
      Logfout << GetNow() << "Sock(" << clientSock << ") connect error:" << strerror(errno) << "(errno:" << errno << ")." << endl;
      Logfout.close();
      return -1;
    }

    // 为该套接字创建接收线程
    struct threadparamA *tempThreadParam=(struct threadparamA *)malloc(sizeof(struct threadparamA));
    tempThreadParam->tempTCPRoute=this;
    tempThreadParam->tempAddr=masterAddr;
    tempThreadParam->tempSock=clientSock;

    if (pthread_create(&server_thread,NULL,ServerThread,(void*)tempThreadParam)<0)
    {
      Logfout << GetNow() << "Create thread for a connect failed!!!!!!!!!" << endl;
      Logfout.close();
      return -1;
    }
  }

  // tcp连接建立成功或者本已存在，发送hello
  struct mastermaptosock tempMasterMapToSock;
  tempMasterMapToSock.chiefMaster=false;
  tempMasterMapToSock.masterAddr=masterAddress;
  tempMasterMapToSock.masterIdent.level=-1;// 等收到ACK后再填写
  tempMasterMapToSock.masterIdent.position=-1;
  tempMasterMapToSock.masterSock=clientSock;
  tempMasterMapToSock.NICName=NICName;
  tempMasterMapToSock.direct=direct;
  tempMasterMapToSock.middleAddr=middleAddress;
  tempMasterMapToSock.keepAliveFaildNum=0;
  tempMasterMapToSock.recvKeepAlive=false;
  tempMasterMapToSock.isStartKeepAlive=false;
  tempMasterMapToSock.inDirPath=inDirPath;

  m_globalRouting->UpdateMasterMapToSock(tempMasterMapToSock,1);

  struct MNinfo helloMNInfo;
  helloMNInfo.addr.sin_family=AF_INET;
  inet_aton(masterAddress.c_str(),&(helloMNInfo.addr.sin_addr));
  helloMNInfo.addr.sin_port=htons(0);
  helloMNInfo.destIdent=destIdent;// 此时可能还不知道目的地址的ident
  helloMNInfo.forwardIdent=myIdent;
  helloMNInfo.srcIdent=myIdent;
  helloMNInfo.eventId=-1;
  helloMNInfo.clusterMaster=false;
  helloMNInfo.chiefMaster=false;
  helloMNInfo.reachable=true;
  helloMNInfo.keepAlive=false;
  helloMNInfo.linkFlag=false;
  helloMNInfo.hello=true;
  helloMNInfo.ACK=false;
  helloMNInfo.bye=false;

  if (direct==true) 
  {
    Logfout << GetNow() << "Try to connect with master[" << masterAddress << "][sock:" << clientSock << "]." << endl;

    helloMNInfo.pathNodeIdent[0]=myIdent;
    helloMNInfo.pathNodeIdent[1]=myIdent;
    for (int i=2;i<MAX_PATH_LEN;i++)
    {
      helloMNInfo.pathNodeIdent[i].level=-1;
      helloMNInfo.pathNodeIdent[i].position=-1;
    }
    value=SendMessageTo(clientSock,helloMNInfo);
  }
  else
  {
    Logfout << GetNow() << "Try to connect with forward node[" << middleAddress << "][sock:" << clientSock << "]." << endl;
    for (int i=0;i<MAX_PATH_LEN;i++) helloMNInfo.pathNodeIdent[i]=inDirPath->pathNodeIdent[i];
    value=SendMessageTo(clientSock,helloMNInfo);// 通过hello包来发送路径
  }
  Logfout.close();
  return clientSock;
}

void* 
TCPRoute::SendHelloToThread(void* tempThreadParam)
{
  // pthread_detach(pthread_self());
  TCPRoute *tempTCPRoute=((struct threadparamB *)tempThreadParam)->tempTCPRoute;
  string masterAddress=((struct threadparamB *)tempThreadParam)->masterAddress;
  ident destIdent=((struct threadparamB *)tempThreadParam)->destIdent;

  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << tempTCPRoute->myIdent.level << "." << tempTCPRoute->myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  ident tempIdent;
  tempIdent.level=-1;
  tempIdent.position=-1;

  int interval=0;//单位s
  int result=-1;//
  struct pathtableentry *tempInDirPath=new pathtableentry();
  tempInDirPath=NULL;

  // Logfout << "SendHelloToThread:" << masterAddress << endl;

  do
  {
    if ((tempTCPRoute->myIdent.level!=0 && interval==0) || tempTCPRoute->myIdent.level==0)// Node第一次尝试则选择管理网口，common master则只能尝试管理网口
    {
      // 首先检测管理网口是否正常工作
      if ((m_globalRouting->GetAddrByNICName(MGMT_INTERFACE))!=NULL)// 管理网口正常，尝试直接连接
      {
        result=tempTCPRoute->SendHelloToMaster(destIdent,masterAddress,"",MGMT_INTERFACE,NULL,true);
        interval+=2;// 
        sleep(interval);
      }
    }
    if (tempTCPRoute->myIdent.level!=0 && result==-1)// 不是第一次尝试或者上一次连接失败（包括通过管理网口连接失败，只有node才能调用）
    {
      tempInDirPath=m_globalRouting->GetPathToMaster(tempInDirPath);
      if (tempInDirPath==NULL) 
      {
        Logfout << GetNow() << "I can not connect with master[" << masterAddress << "] any longer." << endl;
        break;
      }
      result=tempTCPRoute->SendHelloToMaster(destIdent,masterAddress,inet_ntoa(tempInDirPath->nodeAddr.addr.sin_addr),m_globalRouting->GetNICNameByRemoteAddr(tempInDirPath->nextHopAddr.addr),tempInDirPath,false);
      interval+=2;// 
      sleep(interval);
      if (result==-1)// 连接失败，等待重连
      {
        Logfout << GetNow() << "Send hello to master[" << masterAddress << "] again." << endl;
        continue;
      }
      else 
      {
        Logfout << GetNow() << "InDirPath:[";
        for (int i=0;i<MAX_PATH_LEN;i++) 
        {
          if (tempInDirPath->pathNodeIdent[i].level!=-1) Logfout << tempInDirPath->pathNodeIdent[i].level << "." << tempInDirPath->pathNodeIdent[i].position << "\t";
          else break;
        }
        Logfout << "] to master[" << masterAddress << "]." << endl;
      }
    }
    // 发送hello成功，检查是否已经收到了ACK
    if (m_globalRouting->IsLegalMaster(masterAddress))
    {
      Logfout << GetNow() << "Recv hello-ACK from master[" << masterAddress << "]." << endl;
      break;// 收到ACK
    }
  }while (interval<20);// 每次累加2，总共尝试10次
  Logfout << GetNow() << "Stop to SendHelloTo master[" << masterAddress << "]." << endl;
  Logfout.close();
  // pthread_exit(0);
}

void // master尝试和chiefMaster建立连接
TCPRoute::SendHelloTo(ident destIdent,string masterAddress)
{
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  // Logfout << "SendHelloTo to " << masterAddress << endl;
  threadparamB *tempThreadParam=new threadparamB();
  tempThreadParam->tempTCPRoute=this;
  tempThreadParam->masterAddress=masterAddress;
  tempThreadParam->destIdent=destIdent;
  
  if (pthread_create(&helloto_thread,NULL,SendHelloToThread,(void *)tempThreadParam)<0)
  {
    Logfout << GetNow() << "Create thread to connect with chiefMaster failed!!!!!!!!!" << endl;
    exit(0);
  }
  Logfout.close();
}