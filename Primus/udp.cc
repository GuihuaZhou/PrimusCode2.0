#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include "ipv4-global-routing.h"

string GetNow(){
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );
	stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

Ipv4GlobalRouting *
UDPServer::m_globalRouting=NULL;

UDPServer::UDPServer()
{
	// 
}

UDPServer::~UDPServer()
{
	// 
}

void
UDPServer::SetGlobalRouting(Ipv4GlobalRouting *tempGlobalRouting)
{
    m_globalRouting=tempGlobalRouting;
    myIdent=m_globalRouting->GetMyIdent();
}

bool
UDPServer::StartApplication()
{   
	//create thread and wait for packets 
	stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

	// vector<string> tempListenNIC=m_globalRouting->GetListenNIC();
	// vector<struct sockaddr_in> tempListenNICAddr;

	// struct ifaddrs *ifa;

	// if (0!=getifaddrs(&ifa))
 //  {
 //    printf("getifaddrs error\n");
 //    exit(1);
 //  }
 //  for (;ifa!=NULL;)
 //  {
 //    if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && ifa->ifa_name[0]=='E')// 交换机
 //    {
 //    	for (int i=0;i<tempListenNIC.size();i++)
 //    	{
 //    		if (ifa->ifa_name==tempListenNIC[i]) 
 //    		{
 //    			tempListenNICAddr.push_back(*((struct sockaddr_in *)(ifa->ifa_addr)));
 //    			break;
 //    		}
 //      }
 //    }
 //    ifa=ifa->ifa_next;
 //  }
 //  Logfout << "tempListenNICAddr" << endl;
 //  for (int i=0;i<tempListenNICAddr.size();i++)
 //  {
 //  	Logfout << inet_ntoa(tempListenNICAddr[i].sin_addr) << "\t";
 //  }
 //  Logfout << endl;

 //  freeifaddrs(ifa);

	// pthread_t selectThread,pathTableThread;
	// for (int i=0;i<tempListenNICAddr.size();i++)
	// {
	// 	struct sockaddr_in * object = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	// 	*object = tempListenNICAddr[i];
	// 	if(pthread_create(&selectThread, 0, HandleReadND, (void *)object) != 0)// 接收邻居发现时的hello信息
 //    {
 //    	Logfout<<GetNow()<<"UDPServer::StartApplication (): Create Thread Failed."<<std::endl;
 //    	return false;
 //    }  
	// }	

	// for (int i=0;i<tempListenNICAddr.size();i++)
	// {
	// 	struct sockaddr_in * object = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	// 	*object = tempListenNICAddr[i];
 //    if (pthread_create(&pathTableThread,0,HandleReadPathTable,(void *)object)!=0)//接收路径信息
 //    {
 //    	Logfout<<GetNow()<<"UDPServer::StartApplication (): Create HandleReadPathTable Failed."<<std::endl;
 //    	return false;
 //    }
	// }
	
	// Logfout << GetNow() << "UDPServer::StartApplication over" << endl;

	pthread_t linkInfoThread;
	threadparamA *tempThreadParam=new threadparamA();
	tempThreadParam->tempUDPServer=this;
	if (pthread_create(&linkInfoThread,0,HandleReadLinkInfo,(void*)tempThreadParam)!=0)// 转发linkinfo
  {
  	Logfout << GetNow() << "UDPServer::StartApplication():Create HandleReadLinkInfo Failed." << endl;
  	return false;
  }

	if (myIdent.level!=0)// node还需要启动邻居发现
	{
		pthread_t selectThread;
		if(pthread_create(&selectThread,0,HandleReadND,NULL)!= 0)// 接收邻居发现时的hello信息
	  {
	  	Logfout << GetNow() << "UDPServer::StartApplication():Create HandleReadND Failed." << endl;
	  	return false;
	  }

	  pthread_t pathTableThread;
	  if (pthread_create(&pathTableThread,0,HandleReadPathTable,NULL)!=0)//接收路径信息
	  {
	  	Logfout << GetNow() << "UDPServer::StartApplication():Create HandleReadPathTable Failed." << endl;
	  	return false;
	  }
	}

  return true;
}

void* 
UDPServer::HandleReadLinkInfo(void* tempThreadParam)// node转发linkinfo至master
{
	// pthread_detach(pthread_self());
	stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

  UDPServer *tempUDPServer=((struct threadparamA *)tempThreadParam)->tempUDPServer;

	int server_sockfd;
	int remote_len = sizeof(struct sockaddr_in);
	struct sockaddr_in my_addr; 
	struct sockaddr_in remote_addr;

	bzero(&my_addr,sizeof(struct sockaddr_in));
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=htons(INADDR_ANY);
	// my_addr.sin_addr.s_addr=(*((struct sockaddr_in *)object)).sin_addr.s_addr;// sonic
	my_addr.sin_port=htons(UDP_TF_PORT); 

	bzero(&remote_addr,sizeof(struct sockaddr_in));
	remote_addr.sin_family=AF_INET; 
	remote_addr.sin_addr.s_addr=htons(INADDR_ANY);
	remote_addr.sin_port=htons(UDP_TF_PORT); 

	if ((server_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{  
		Logfout << GetNow() << "HandleReadLinkInfo socket failed" << endl;
		exit(1);
	}

	int value=1;
  if (setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "HandleReadLinkInfo set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "HandleReadLinkInfo set SO_REUSEADDR error" << endl;
    exit(0);
  }
	
	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))<0)
	{
		Logfout << GetNow() << "HandleReadLinkInfo bind failed" << endl;
		exit(1);
	}
	
	Logfout << GetNow() << "Local NIC is waiting for linkInfo......" << endl;
	value=0;
	while (1)
	{
		char recvBuf[MNINFO_BUF_SIZE];

		if((value=recvfrom(server_sockfd,recvBuf,MNINFO_BUF_SIZE,0,(struct sockaddr *)&remote_addr,(socklen_t*)&remote_len))<0)
		{
			Logfout << "HandleReadLinkInfo recvfrom failed" << endl;
			continue;
		} 
		
		struct MNinfo tempMNInfo;
    memcpy(&tempMNInfo,recvBuf,sizeof(struct MNinfo));

    if (m_globalRouting->SameNode(tempUDPServer->myIdent,tempMNInfo.destIdent))// 目的地是本地
    {
    	Logfout << GetNow();
    	if (tempUDPServer->myIdent.level==0) Logfout << "Master ";
    	else Logfout << "Node ";
    	Logfout << tempUDPServer->myIdent.level << "." << tempUDPServer->myIdent.position << " recv ";
      Logfout << tempMNInfo.pathNodeIdent[0].level << "." << tempMNInfo.pathNodeIdent[0].position << "--" << tempMNInfo.pathNodeIdent[1].level << "." << tempMNInfo.pathNodeIdent[1].position;
      if (tempMNInfo.linkFlag==true) Logfout << " up ";
      else if (tempMNInfo.linkFlag==false) Logfout << " down ";
      Logfout << "from ";
      if (tempMNInfo.srcIdent.level==0) Logfout << "Master ";
      else Logfout << "Node ";
      Logfout << tempMNInfo.srcIdent.level << "." << tempMNInfo.srcIdent.position << " by udp [forwardNode:" << tempMNInfo.forwardIdent.level << "." << tempMNInfo.forwardIdent.position << "][value:" << value << "][eventId:" << tempMNInfo.eventId << "][sock:" << server_sockfd << "]." << endl;

      m_globalRouting->HandleMessage(tempMNInfo);
    }
    else if (m_globalRouting->SameNode(tempUDPServer->myIdent,tempMNInfo.forwardIdent))// 作为转发node转发upd message至leader master
    {
    	m_globalRouting->TransferTo(tempMNInfo);// 转发至chief或者目的node
    }
	}
	Logfout.close();
	Logfout << GetNow() << "LocalNIC(" << inet_ntoa(my_addr.sin_addr) << ") receive pathInfo thread down!!!!!!" << endl;
	// pthread_exit(0);
}

void*
UDPServer::HandleReadND(void* object)
{
	// pthread_detach(pthread_self());
  stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  
	int value = -1;
	int from_len = sizeof(struct sockaddr_in);
	int sock = -1;
  struct sockaddr_in serverAddr;//服务端地址
	struct sockaddr_in fromAddr;//客户端地址
	
	bzero(&serverAddr,sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;//Set as IP communication
	serverAddr.sin_addr.s_addr = htons(INADDR_ANY);//server IP address:allowed to connect any local address
	// serverAddr.sin_addr.s_addr = (*((struct sockaddr_in *)object)).sin_addr.s_addr;// sonic
	serverAddr.sin_port = htons(ND_PORT);//server port
    
	//广播地址
	bzero(&fromAddr,sizeof(struct sockaddr_in));
	fromAddr.sin_family = AF_INET;//Set as IP communication
	fromAddr.sin_addr.s_addr = htons(INADDR_ANY);//server IP address:allowed to connect any local address
	fromAddr.sin_port = htons(ND_PORT);//server port
	//set dgram for server
	// Logfout << GetNow() << "LocalNIC(" << inet_ntoa(serverAddr.sin_addr) << ") is waiting for ND......" << endl;

  if((sock=socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	  Logfout << GetNow() << "HandleReadND Create Socket Failed." << endl;
	  exit(1);
	}

	value=1;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "HandleReadND set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "HandleReadND set SO_REUSEADDR error" << endl;
    exit(0);
  }

	//bind 
	if((bind(sock,(struct sockaddr*) &serverAddr,sizeof(serverAddr))) < 0)
	{
		Logfout << GetNow() << "HandleReadND Bind Socket Failed(" << inet_ntoa(serverAddr.sin_addr) << ")" << endl;
    exit(1);
	}
	
	Logfout << GetNow() << "Local NIC is waiting for ND......" << endl;
	char recvBuf[ND_BUF_SIZE];
	while(1)
	{
		memset(recvBuf,'\0',ND_BUF_SIZE);
		if((value=recvfrom(sock,recvBuf,ND_BUF_SIZE,0,(struct sockaddr *)&fromAddr,(socklen_t*)&from_len)) < 0)
		{
			Logfout << GetNow() << "UDPServer::HandleReadND (): Socket recvfrom Failed." << endl;
			break;
		}
		//排除本地广播
		if(strstr((char *)inet_ntoa(fromAddr.sin_addr), "127.0.0.1"))
		{
			continue;
		}

		// 先处理，再回复
		struct NDinfo tempNDInfo;
		memcpy(&tempNDInfo,recvBuf,sizeof(struct NDinfo));

		if (tempNDInfo.myIdent.level==0)// master
		{
			Logfout << GetNow() << "I should connect with master by another indirpath[sock:" << sock << "." << endl;
			// m_globalRouting->ReconnectWithMaster();
		}
		else 
		{
			Logfout << GetNow() << "Recv ND from " << inet_ntoa(fromAddr.sin_addr) << "[value:" << value << "]." << endl;
			if ((value=sendto(sock,recvBuf,sizeof(struct NDinfo),0,(struct sockaddr*)&fromAddr,from_len))<0)
			{
				Logfout << GetNow() << "Send ND reply error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
	  		break;
			}	

			Logfout << GetNow() << "Send ND reply to " << inet_ntoa(fromAddr.sin_addr) << "[value:" << value << "]." << endl;
			m_globalRouting->NDRecvND(tempNDInfo);	
			// usleep(SEND_PATH_INTERVAL);
		}
	}
	Logfout << GetNow() << "LocalNIC(" << inet_ntoa(serverAddr.sin_addr) << ") receive ND thread down!!!!!!" << endl;
	close(sock);
	Logfout.close();
	// pthread_exit(0);
}

void* 
UDPServer::HandleReadPathTable(void* object)
{
	// pthread_detach(pthread_self());
	stringstream logFoutPath;
  logFoutPath.str("");
  logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);

	int server_sockfd;
	int remote_len = sizeof(struct sockaddr_in);
	struct sockaddr_in my_addr; 
	struct sockaddr_in remote_addr;

	bzero(&my_addr,sizeof(struct sockaddr_in));
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=htons(INADDR_ANY);
	// my_addr.sin_addr.s_addr=(*((struct sockaddr_in *)object)).sin_addr.s_addr;// sonic
	my_addr.sin_port=htons(PT_PORT); 

	bzero(&remote_addr,sizeof(struct sockaddr_in));
	remote_addr.sin_family=AF_INET; 
	remote_addr.sin_addr.s_addr=htons(INADDR_ANY);
	remote_addr.sin_port=htons(PT_PORT); 

	if ((server_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{  
		Logfout << GetNow() << "HandleReadPathTable socket failed" << endl;
		exit(1);
	}

	int value=1;
  if (setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "HandleReadPathTable set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "HandleReadPathTable set SO_REUSEADDR error" << endl;
    exit(0);
  }

	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))<0)
	{
		Logfout << GetNow() << "HandleReadPathTable bind failed" << endl;
		exit(1);
	}
	
	Logfout << GetNow() << "Local NIC is waiting for pathInfo......" << endl;
	value=0;
	while (1)
	{
		char recvBuf[PT_BUF_SIZE];
		// Logfout << endl << "------waiting-------" << endl;
		if((value=recvfrom(server_sockfd,recvBuf,PT_BUF_SIZE,0,(struct sockaddr *)&remote_addr,(socklen_t*)&remote_len))<0)
		{
			Logfout << "HandleReadPathTable recvfrom failed" << endl;
			continue;
		} 
		
		struct pathinfo tempPathInfo;
		memcpy(&tempPathInfo,recvBuf,sizeof(tempPathInfo));
		
	  m_globalRouting->FreshPathTable(&tempPathInfo,remote_addr);
	}
	Logfout.close();
	Logfout << GetNow() << "LocalNIC(" << inet_ntoa(my_addr.sin_addr) << ") receive pathInfo thread down!!!!!!" << endl;
	// pthread_exit(0);
}



Ipv4GlobalRouting *
UDPClient::m_globalRouting=NULL;

UDPClient::UDPClient()
{
	
}

UDPClient::~UDPClient()
{

}

void
UDPClient::SetGlobalRouting(Ipv4GlobalRouting *tempGlobalRouting)
{
    m_globalRouting=tempGlobalRouting;
    myIdent=m_globalRouting->GetMyIdent();
}

void*
UDPClient::SendNDThread(void* tempThreadParam)// 发出ND信息，并等待回复
{
	// pthread_detach(pthread_self());
	stringstream logFoutPath;
	logFoutPath.str("");
	logFoutPath << "/var/log/Primus-" << m_globalRouting->GetMyIdent().level << "." << m_globalRouting->GetMyIdent().position << ".log";
  ofstream Logfout(logFoutPath.str().c_str(),ios::app);
	
	struct sockaddr_in clientAddr=((struct threadparamA *)tempThreadParam)->localAddr;
	struct sockaddr_in bcastAddr=((struct threadparamA *)tempThreadParam)->remoteAddr;//广播地址
	struct NDinfo tempNDInfo=((struct threadparamA *)tempThreadParam)->tempNDInfo;
	struct sockaddr_in fromAddr;//服务端端地址
	bzero(&fromAddr,sizeof(struct sockaddr_in));
	int sock = -1;
	int so_broadcast = 1;
	int from_len = sizeof(struct sockaddr_in);
	struct timeval timeout;
	timeout.tv_sec=3;
	timeout.tv_usec=0;
	
	//set dgram for client
  if((sock=socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	    Logfout << GetNow() << "SendNDThread Create Socket Failed." << std::endl;
	    exit(0);
	}

	int value=1;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "SendNDThread set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "SendNDThread set SO_REUSEADDR error" << endl;
    exit(0);
  }

	if((bind(sock,(struct sockaddr*)&(clientAddr),sizeof(clientAddr))<0))
	{
		Logfout << GetNow() << "SendNDThread Bind interface Failed." << endl;
    	exit(0);
	}

	//the default socket doesn't support broadcast,so we should set socket discriptor to do it.
	if((setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast))) < 0)
	{
		Logfout << GetNow() << "SendNDThread SetSocketOpt Failed." << endl;
    exit(0);
	}
	int time_live=TTL;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));

	char sendBuf[ND_BUF_SIZE],recvBuf[ND_BUF_SIZE];
	memcpy(sendBuf,&tempNDInfo,sizeof(struct NDinfo));
	value=0;

  if (tempNDInfo.myIdent.level==0)// 协助Master通知间接连接的Node
  {
  	if((value=sendto(sock,sendBuf,sizeof(struct NDinfo),0,(struct sockaddr *)&bcastAddr,sizeof(bcastAddr)))<0)
	  {
	  	Logfout << GetNow() << "SendND error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
	  	close(sock);
			Logfout.close();
			// pthread_exit(0);
	  } 
	  Logfout << GetNow() << "Send ND to " << inet_ntoa(bcastAddr.sin_addr) << "[value:" << value << "]." << endl;
  }
  else// 是ND
  {
  	int timeoutNum=0;
  	while (1)
		{
			if((value=sendto(sock,sendBuf,sizeof(struct NDinfo),0,(struct sockaddr *)&bcastAddr,sizeof(bcastAddr)))<0)
		  {
		  	Logfout << GetNow() << "SendND error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
		  	break;
		  } 
		  Logfout << GetNow() << "Send ND to " << inet_ntoa(bcastAddr.sin_addr);
		  if (timeoutNum!=0) Logfout << " again";
		  Logfout << "[value:" << value << "]." << endl;
			//set receive timeout
			setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
			memset(recvBuf,'\0',ND_BUF_SIZE);// 还要用来接收
		  if((value=recvfrom(sock,recvBuf,ND_BUF_SIZE,0,(struct sockaddr*)&fromAddr,(socklen_t*)&from_len))<0)
			{
				timeoutNum++;
				// Logfout << "------------recv error" << endl;
				if (timeoutNum>=3)// 超时3次，直接挂掉
				{
					Logfout << GetNow() << "Recv ND reply from " << inet_ntoa(bcastAddr.sin_addr) << "[error:" << strerror(errno) << "(errno:" << errno <<  ")]." << endl;
					tempNDInfo.myIdent.level=-1;
					tempNDInfo.myIdent.position=-1;//表示超时没有收到ND
					m_globalRouting->NDRecvACK(tempNDInfo);
					break;
				}
				else continue;
			}
			memcpy(&tempNDInfo,recvBuf,sizeof(struct NDinfo));
			Logfout << GetNow() << "Recv ND reply from " << inet_ntoa(fromAddr.sin_addr) << "[value:" << value << "]." << endl;
			m_globalRouting->NDRecvACK(tempNDInfo);
			break;
		}
  }
  // Logfout << "---------SendNDThread over" << endl;
	close(sock);
	Logfout.close();
	// pthread_exit(0);
}

void
UDPClient::SendNDTo(struct sockaddr_in localAddr,struct sockaddr_in remoteAddr,struct NDinfo tempNDInfo)
{
	stringstream logFoutPath;
	logFoutPath.str("");
	logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
	ofstream Logfout(logFoutPath.str().c_str(),ios::app);

	struct threadparamA *tempThreadParam=(struct threadparamA *)malloc(sizeof(struct threadparamA));
	tempThreadParam->localAddr=localAddr;
	tempThreadParam->remoteAddr=remoteAddr;
	tempThreadParam->tempNDInfo=tempNDInfo;
  
	pthread_t NDThread;

	try
  {
    if (pthread_create(&NDThread,NULL,SendNDThread,(void*)tempThreadParam)!=0)
    {
    	Logfout << GetNow() << "Send ND failed" << endl;
    }
  }
  catch(...)
  {
  	Logfout << GetNow() << "Send ND failed" << endl;
    throw;
  }
  Logfout.close();
}

void 
UDPClient::SendPathInfoTo(struct sockaddr_in localAddr,struct sockaddr_in remoteAddr,struct pathinfo *tempPathInfo)
{
	stringstream logFoutPath;
	logFoutPath.str("");
	logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
	ofstream Logfout(logFoutPath.str().c_str(),ios::app);
  
	Logfout << GetNow() << "Send path [ ";
  for (int i=0;i<MAX_PATH_LEN;i++)
  {
  	if (tempPathInfo->pathNodeIdent[i].level!=-1 && tempPathInfo->pathNodeIdent[i].position!=-1)
  	{
  		Logfout << tempPathInfo->pathNodeIdent[i].level << "." << tempPathInfo->pathNodeIdent[i].position << "\t";
  	}
  }
  Logfout << "serverAddr:";
  for (int i=0;i<MAX_ADDR_NUM;i++)
  {
  	if (!strcmp(inet_ntoa(tempPathInfo->addrSet[i].addr.sin_addr),"255.255.255.255")) break;
  	Logfout << "(" << inet_ntoa(tempPathInfo->addrSet[i].addr.sin_addr) << ")";
  }
  Logfout << " ] to " << m_globalRouting->GetNeighborIdentByRemoteAddr(remoteAddr).level << "." << m_globalRouting->GetNeighborIdentByRemoteAddr(remoteAddr).position;
  Logfout << "(" << sizeof(*tempPathInfo) << "B)." << endl;

	int client_sockfd;
	struct sockaddr_in remote_addr; 
	memset(&remote_addr,0,sizeof(remote_addr));
	remote_addr.sin_family=AF_INET;
	remote_addr.sin_addr=remoteAddr.sin_addr;
	remote_addr.sin_port=htons(PT_PORT);

	if ((client_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{
		Logfout << GetNow() << "SendPathInfoTo sock failed" << endl;
		exit(1);
	}

	int value=1;
  if (setsockopt(client_sockfd,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "SendPathInfoTo set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(client_sockfd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "SendPathInfoTo set SO_REUSEADDR error" << endl;
    exit(0);
  }

	if ((bind(client_sockfd,(struct sockaddr*)&(localAddr),sizeof(localAddr))<0))
	{
		Logfout << GetNow() << "SendPathInfoTo bind failed"<<std::endl;
    exit(0);
	}
	char sendBuf[PT_BUF_SIZE];
	memcpy(sendBuf,tempPathInfo,sizeof(*tempPathInfo));

	if (sendto(client_sockfd,sendBuf,sizeof(*tempPathInfo),0,(struct sockaddr *)&remote_addr,sizeof(remote_addr))<0)
  {
  	Logfout << GetNow() << "SendPathInfoTo error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
  	exit(0);
  }
  // Logfout << GetNow() << "SendPathInfoTo over and size is " << sizeof(*tempPathInfo) << endl;
  Logfout.close();
}

void 
UDPClient::SendLinkInfo(struct sockaddr_in localAddr,string remoteAddress,struct MNinfo tempMNInfo)
{
	stringstream logFoutPath;
	logFoutPath.str("");
	logFoutPath << "/var/log/Primus-" << myIdent.level << "." << myIdent.position << ".log";
	ofstream Logfout(logFoutPath.str().c_str(),ios::app);

	int client_sockfd;
	struct sockaddr_in remote_addr; 
	memset(&remote_addr,0,sizeof(remote_addr));
	remote_addr.sin_family=AF_INET;
	remote_addr.sin_addr.s_addr=inet_addr(remoteAddress.c_str());
	remote_addr.sin_port=htons(UDP_TF_PORT);

	if ((client_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{
		Logfout << GetNow() << "SendLinkInfo sock failed" << endl;
		exit(1);
	}

	int value=1;
  if (setsockopt(client_sockfd,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "SendLinkInfo set SO_REUSEPORT error" << endl;
    exit(0);
  }

  if (setsockopt(client_sockfd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    Logfout << GetNow() << "SendLinkInfo set SO_REUSEADDR error" << endl;
    exit(0);
  }

	if ((bind(client_sockfd,(struct sockaddr*)&(localAddr),sizeof(localAddr))<0))
	{
		Logfout << GetNow() << "SendLinkInfo bind failed"<<std::endl;
    exit(0);
	}
	char sendBuf[MNINFO_BUF_SIZE];
	memcpy(sendBuf,&tempMNInfo,sizeof(tempMNInfo));

	if (sendto(client_sockfd,sendBuf,sizeof(tempMNInfo),0,(struct sockaddr *)&remote_addr,sizeof(remote_addr))<0)
  {
  	Logfout << GetNow() << "SendLinkInfo error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
  	exit(0);
  }
  Logfout.close();
}