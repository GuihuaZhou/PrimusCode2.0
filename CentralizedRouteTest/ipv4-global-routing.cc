#include <time.h>  
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "node.h"

using namespace std;

/*Logfout.setf(ios::fixed, ios::floatfield);
Logfout.precision(9);//设置保留的小数点位数*/

Node *
Ipv4GlobalRouting::m_node=NULL;

void
Ipv4GlobalRouting::SetNode(Node *node)
{
  m_node=node;
}

void
Ipv4GlobalRouting::SetAttribute(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int defaultTimer,bool IsCenterRouting,bool randomEcmpRouting)
{
  m_Node.level=level; 
  m_Node.position=position;
  m_ToRNodes=ToRNodes;
  m_LeafNodes=LeafNodes;
  m_SpineNodes=SpineNodes;
  m_nPods=nPods;
  m_Pod=Pod;
  m_nMaster=nMaster;
  m_Links=Links;
  m_defaultTimer=defaultTimer;
  m_IsCenterRouting=IsCenterRouting;
  m_randomEcmpRouting=randomEcmpRouting;
}

ident 
Ipv4GlobalRouting::GetNextHopIdent(int level,int position,int interface,int * nextHopInterface)//interface从2开始,position从0开始
{
  ident nextHopIdent;
  interface-=m_nMaster;
  if (level==1)//若是第一层的端口，则返回它的下一跳，即第二层的下行链路端口的位置信息
  {
    nextHopIdent.level=2;
    nextHopIdent.position=position/m_ToRNodes*m_LeafNodes+interface-1;
    *nextHopInterface=position%m_ToRNodes+(m_SpineNodes/m_LeafNodes)+1+m_nMaster;
  }
  else if (level==2)
  {
    if (interface<((m_SpineNodes/m_LeafNodes)+1))//若是第二层的上行链路端口，则返回它的下一跳，即第三层的下行链路端口的位置信息
    {
      nextHopIdent.level=3;
      nextHopIdent.position=position%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+interface-1;
      *nextHopInterface=position/m_LeafNodes+1+m_nMaster;
    }
    else if (interface>=((m_SpineNodes/m_LeafNodes)+1))//若是第二层的下行链路端口，则返回它的下一跳，即第一层的上行链路端口的位置信息
    {
      nextHopIdent.level=1;
      nextHopIdent.position=position/m_LeafNodes*m_ToRNodes+interface-(m_SpineNodes/m_LeafNodes)-1;
      *nextHopInterface=position%m_LeafNodes+m_nMaster+1;
    }
  }
  else if (level==3)//若是第三层的下行链路端口，则返回它的下一跳，即第二层的上行链路端口的位置信息
  {
    nextHopIdent.level=2;
    nextHopIdent.position=position/(m_SpineNodes/m_LeafNodes)+(interface-1)*m_LeafNodes;
    *nextHopInterface=position%(m_SpineNodes/m_LeafNodes)+1+m_nMaster;
  }
  return nextHopIdent;
}

string 
Ipv4GlobalRouting::Allocate_ip(int level,int position,int interface)//position从0开始,interface从2开始,分配IP时positon注意加1,仅仅是针对实现时而言
{
  stringstream ip_address;
  if (level==3)
  {
    ip_address << "32." << position+1 << "." << interface-m_nMaster << "." << 0;
  }
  else if (level==2)
  {
    if (interface<=(m_SpineNodes/m_LeafNodes+1))
    {
      ip_address << "32." << position%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+interface-m_nMaster << "." << position/m_LeafNodes+1 << "." << 1;
    }
    else if (interface>(m_SpineNodes/m_LeafNodes+1))
    {
      ip_address << "21." << position+1 << "." << interface-(m_SpineNodes/m_LeafNodes)-m_nMaster << "." << 0;
    }
  }
  else if (level==1)
  {
    ip_address << "21." << position/m_ToRNodes*m_LeafNodes+interface-m_nMaster << "." << position%m_ToRNodes+1 << "." << 1;
  }
  return ip_address.str();
}

void 
Ipv4GlobalRouting::InitializeMasterLinkTable()
{
  //std::cout << "Ipv4GlobalRouting    Master InitializeMasterLinkTable and time:" << Simulator::Now().GetSeconds() << std::endl;
  ofstream Logfout("/var/log/center.log",ios::app);
  // Logfout << GetNow() << "InitializeMasterLinkTable m_ToRNodes is " << m_ToRNodes << " and m_SpineNodes is " << m_SpineNodes << " and m_nPods is " << m_nPods << std::endl;
  struct linktableentry linkTableEntry;
  for (int i=0;i<m_SpineNodes;i++)//先生成Spine Node直连的链路
  {
    linkTableEntry.high.level=3;
    linkTableEntry.high.position=i;
    linkTableEntry.isNewLink=false;//test
    linkTableEntry.flag=true;//test
    linkTableEntry.lastUpdateTime=GetSystemTime();//正常初始化应先设置为0，等待master收到Node确认该链路正式生效后才记录当前的时间戳，但是由于当前master不能收到全部的确认信息，所以初始化才获取当前时间戳    linkTableEntry.isStartThread=false;
    linkTableEntry.linkUpdateTimer=m_defaultTimer;
    linkTableEntry.lastUpdateFlag=true;
    linkTableEntry.temp=0;//test
    for (int j=0;j<m_nPods;j++)
    {
      linkTableEntry.low.level=2;
      linkTableEntry.low.position=j*m_LeafNodes+i/(m_SpineNodes/m_LeafNodes); 
      masterLinkTable.push_back(linkTableEntry);
    }
  }
  for (int i=0;i<m_LeafNodes*m_nPods;i++)
  {
    linkTableEntry.high.level=2;
    linkTableEntry.high.position=i;
    linkTableEntry.isNewLink=false;//test
    linkTableEntry.flag=true;//test
    linkTableEntry.lastUpdateTime=GetSystemTime();
    linkTableEntry.linkUpdateTimer=m_defaultTimer;
    linkTableEntry.isStartThread=false;
    linkTableEntry.lastUpdateFlag=true;
    linkTableEntry.temp=0;//test
    for (int j=0;j<m_ToRNodes;j++)
    {
      linkTableEntry.low.level=1;
      linkTableEntry.low.position=j+m_ToRNodes*(i/m_LeafNodes);   
      masterLinkTable.push_back(linkTableEntry);
    }
  }
  Logfout.close();
  PrintMasterLinkTable();//故障定位
  //std::cout << "Ipv4GlobalRouting    InitializeMasterLinkTable's size:" << masterLinkTable.size() << std::endl << std::endl;
}

void 
Ipv4GlobalRouting::GeneratePath(ident high,ident low,std::vector<struct pathtableentry> *pathEntryTable)//生成路径
{
  //cout << "Ipv4GlobalRouting::GeneratePath" << endl;
  if (m_Node.level==1)
  {
    if (high.position/m_LeafNodes+1==m_Pod)//在同一个Pod内
    {
      struct pathtableentry newPath;
      newPath.isAddRouteToKernel=false;
      newPath.nextHop[0]=m_Node;
      newPath.nextHop[1]=high;
      newPath.nextHop[2].level=0;
      newPath.nextHop[2].position=0;
      newPath.nextHop[3].level=0;
      newPath.nextHop[3].position=0;
      newPath.nextHop[4]=low;
      newPath.destInterfaceIndex=high.position%m_LeafNodes+1+m_nMaster;
      newPath.counter=0;
      newPath.recordWeight=NULL;
      newPath.next=NULL;
      pathEntryTable->push_back(newPath);
    } 
    else
    {
      for (int i=0;i<m_SpineNodes/m_LeafNodes;i++)//不在同一个pod内，则有m_SpineNodes/m_LeafNodes(4)条
      {
        struct pathtableentry newPath;
        newPath.isAddRouteToKernel=false;
        newPath.nextHop[0]=m_Node;
        newPath.nextHop[1].level=2;
        newPath.nextHop[1].position=m_Node.position/m_ToRNodes*m_LeafNodes+high.position%m_LeafNodes;
        newPath.nextHop[2].level=3;
        newPath.nextHop[2].position=high.position%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+i;
        newPath.nextHop[3]=high;
        newPath.nextHop[4]=low;
        newPath.destInterfaceIndex=high.position%m_LeafNodes+1+m_nMaster;
        newPath.counter=0;
        newPath.recordWeight=NULL;
        newPath.next=NULL;
        pathEntryTable->push_back(newPath);
      }
    } 
  }
  else if (m_Node.level==2)
  {
    if (high.position/m_LeafNodes+1==m_Pod)
    {
      struct pathtableentry newPath;
      newPath.isAddRouteToKernel=false;
      newPath.nextHop[0]=high;
      newPath.nextHop[1].level=0;
      newPath.nextHop[1].position=0;
      newPath.nextHop[2].level=0;
      newPath.nextHop[2].position=0;
      newPath.nextHop[3].level=0;
      newPath.nextHop[3].position=0;
      newPath.nextHop[4]=low;
      newPath.destInterfaceIndex=high.position%m_LeafNodes+1+m_nMaster;
      newPath.counter=0;
      newPath.recordWeight=NULL;
      newPath.next=NULL;
      pathEntryTable->push_back(newPath);
    } 
    else 
    {
      for (int i=0;i<m_SpineNodes/m_LeafNodes;i++)
      {
        struct pathtableentry newPath;
        newPath.isAddRouteToKernel=false;
        newPath.nextHop[0]=m_Node;
        newPath.nextHop[1].level=3;
        newPath.nextHop[1].position=high.position%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+i;
        newPath.nextHop[2]=high;
        newPath.nextHop[3].level=0;
        newPath.nextHop[3].position=0;
        newPath.nextHop[4]=low;
        newPath.destInterfaceIndex=high.position%m_LeafNodes+1+m_nMaster;
        newPath.counter=0;
        newPath.recordWeight=NULL;
        newPath.next=NULL;
        pathEntryTable->push_back(newPath);
      }
    }
  }
  else if (m_Node.level==3)
  {
    struct pathtableentry newPath;
    newPath.isAddRouteToKernel=false;
    newPath.nextHop[0]=m_Node;
    newPath.nextHop[1]=high;
    newPath.nextHop[2].level=0;
    newPath.nextHop[2].position=0;
    newPath.nextHop[3].level=0;
    newPath.nextHop[3].position=0;
    newPath.nextHop[4]=low;
    newPath.destInterfaceIndex=high.position%m_LeafNodes+1+m_nMaster;
    newPath.counter=0;
    newPath.recordWeight=NULL;
    newPath.next=NULL;
    pathEntryTable->push_back(newPath);
  }
  //std::cout << "GeneratePath over" << std::endl;
}

void 
Ipv4GlobalRouting::ModifyMappingTable(struct pathtableentry * newPath,int type)//初始化和更新映射表
{
  /*
  从newpath中截取某两个节点作为一条链路
  截取哪两个需要根据路径的实际情况决定
  */
  ident tempA,tempB;
  struct mappingtableentry mappingTableEntry;
  if (type==4)
  {
    tempA=newPath->nextHop[4];
    int i=4;
    while (1)
    {
      if (newPath->nextHop[--i].level!=0) break;
    }
    tempB=newPath->nextHop[i];
  }
  else if (type==3)
  {
    tempA=newPath->nextHop[2];
    tempB=newPath->nextHop[3];
  }
  else if (type==2)
  {
    tempA=newPath->nextHop[1];
    tempB=newPath->nextHop[2];
  }
  else if (type==1)
  {
    tempA=newPath->nextHop[0];
    tempB=newPath->nextHop[1];
  }
  if (tempA.level>tempB.level)
  {
    mappingTableEntry.high=tempA;
    mappingTableEntry.low=tempB;
  }
  else
  {
    mappingTableEntry.high=tempB;
    mappingTableEntry.low=tempA;
  }
  mappingTableEntry.address=newPath;
  mappingEntryTable.push_back(mappingTableEntry);
}


void 
Ipv4GlobalRouting::InitializePathEntryTable()
{
  /*头节点*/ 
  ofstream Logfout("/var/log/center.log",ios::app);
  // Logfout << GetNow() << "Initialize Path Table" << " m_ToRNodes is " << m_ToRNodes << " and m_SpineNodes is " << m_SpineNodes << " and m_nPods is " << m_nPods << std::endl;
  ident someIdent;
  someIdent.level=0;
  someIdent.position=0;
  headNode->isAddRouteToKernel=false;
  headNode->nextHop[0]=someIdent;
  headNode->nextHop[1]=someIdent;
  headNode->nextHop[2]=someIdent;
  headNode->nextHop[3]=someIdent;
  headNode->nextHop[4]=someIdent;
  headNode->destInterfaceIndex=0;
  headNode->counter=0;
  headNode->next=NULL;
  tailNode=headNode;
  //std::cout << "test 1" << std::endl;

  std::vector<struct pathtableentry> pathEntryTable;
  ident high,low;
  bool isTwo=false,isThree=false;//更新映射表的标志位
  high.level=2;
  low.level=1;

  if (m_Node.level==1)
  {
    for (int i=0;i<m_LeafNodes;i++)//根据路径表的数据结构，可以分为4大块，此时m_LeafNodes=4
    {
      //每个大块中包括了所有的pod，例如本例中每个ToRNode有m_LeafNodes个interface，这些interface分别出现在不同的大块中
      for (int j=1;j<=m_nPods;j++)//遍历所有的Pod
      {
        if (j==m_Pod) continue;//
        for (int k=(j-1)*m_ToRNodes;k<j*m_ToRNodes;k++)//每个pod中有m_ToRNodes个node
        {
          high.position=(j-1)*m_LeafNodes+i;
          low.position=k;
          GeneratePath(high,low,&pathEntryTable);//此时会有m_SpineNodes/m_LeafNodes(4)条路径，且这些路径是同一条路由
          for (int m=0;m<pathEntryTable.size();m++)
          {
            //将路径插入路径表中
            struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
            *newPath=pathEntryTable[m];
            tailNode->next=newPath;
            tailNode=newPath;
            headNode->counter++;
            // if (m==0) ModifyRoute(newPath,-1);//第一条路由添加
            // else newPath->isAddRouteToKernel=true;
            //判断是否在地址索引表中存在对应的索引
            if (newPath->nextHop[2].position%(m_SpineNodes/m_LeafNodes)==0 && newPath->nextHop[4].position%m_ToRNodes==0)
            {
              pathEntryAddressIndexThree.push_back(newPath);
              isThree=true;
              if ((m_Pod==1 && newPath->nextHop[4].position/m_ToRNodes==1) || (m_Pod!=1 && newPath->nextHop[4].position/m_ToRNodes==0))
              {
                pathEntryAddressIndexTwo.push_back(newPath);
                isTwo=true;
                pathEntryAddressIndexOne.push_back(newPath);
                ModifyMappingTable(newPath,1);
              }  
            }
            if (isTwo) ModifyMappingTable(newPath,2);
            if (isThree) ModifyMappingTable(newPath,3);
            if (m==0) ModifyMappingTable(newPath,4);
          }
          isTwo=false;
          isThree=false;
          pathEntryTable.clear();
        }
      }
      for (int k=(m_Pod-1)*m_ToRNodes;k<m_Pod*m_ToRNodes;k++)//到本pod内的路径放在每个大块的最后
      {
        if (k==m_Node.position) continue;
        high.position=(m_Pod-1)*m_LeafNodes+i;
        low.position=k;
        GeneratePath(high,low,&pathEntryTable);//此时只有一条路径
        struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
        *newPath=pathEntryTable[0];
        tailNode->next=newPath;
        tailNode=newPath;
        headNode->counter++;
        // ModifyRoute(newPath,-1);
        if ((m_Node.position%m_ToRNodes==0 && newPath->nextHop[4].position%m_ToRNodes==1) || (m_Node.position%m_ToRNodes!=0 && newPath->nextHop[4].position%m_ToRNodes==0))
        {
          pathEntryAddressIndexTwo.push_back(newPath);//记录相同的1.X-2.X前缀中，目的地址为同一Pod内的路径块首地址
          pathEntryAddressIndexThree.push_back(newPath);//
        }
        ModifyMappingTable(newPath,4);
        pathEntryTable.clear();
      }
    }
    //ToR还需要添加到PC的路由，暂时写法
    
    // stringstream cmd;
    // cmd << "sudo route add -net 192.168." << (m_Node.position+1) << ".0/24 gw 192.168." << (m_Node.position+1) << ".1 metric 20";
    // system(cmd.str().c_str());

    // if (m_Node.position==0)
    // {
    //   destAddress="192.168.1.0";
    //   system("sudo route add -net 192.168.1.0/24 gw 192.168.1.1");
    // }
    // else if (m_Node.position==1)
    // {
    //   destAddress="192.168.2.0";
    //   system("sudo route add -net 192.168.2.0/24 gw 192.168.2.1");
    // }
  }
  else if (m_Node.level==2)
  {
    //
    for (int j=1;j<=m_nPods;j++)
    {
      if (j==m_Pod) continue;
      for (int k=(j-1)*m_ToRNodes;k<j*m_ToRNodes;k++)//每个pod中有m_ToRNodes个node
      {
        high.position=(j-1)*m_LeafNodes+m_Node.position%m_LeafNodes;
        low.position=k;
        GeneratePath(high,low,&pathEntryTable);//此时会有m_SpineNodes/m_LeafNodes(4)条路径，但这些路径不是同一条路由
        //end
        for (int m=0;m<pathEntryTable.size();m++)
        {
          struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
          *newPath=pathEntryTable[m];
          tailNode->next=newPath;
          tailNode=newPath;
          headNode->counter++;
          // ModifyRoute(newPath,-1);
          //判断是否在地址索引表中存在对应的索引
          if (newPath->nextHop[1].position%(m_SpineNodes/m_LeafNodes)==0 && newPath->nextHop[4].position%m_ToRNodes==0) 
          {
            pathEntryAddressIndexThree.push_back(newPath); 
            isThree=true;
            if ((m_Pod==1 && newPath->nextHop[4].position/m_ToRNodes==1) || (m_Pod!=1 && newPath->nextHop[4].position/m_ToRNodes==0))
            {
              pathEntryAddressIndexTwo.push_back(newPath);
              isTwo=true;
            } 
          }
          if (isTwo) ModifyMappingTable(newPath,1);
          if (isThree) ModifyMappingTable(newPath,2);
          if (m==0) ModifyMappingTable(newPath,4);
        }
        isTwo=false;
        isThree=false;
        pathEntryTable.clear();
      }
    }
    for (int k=(m_Pod-1)*m_ToRNodes;k<m_Pod*m_ToRNodes;k++)//到本pod内的路径单独存放
    {
      high.position=m_Node.position;
      low.position=k;
      GeneratePath(high,low,&pathEntryTable);//一条路径
      struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
      *newPath=pathEntryTable[0];
      tailNode->next=newPath;
      tailNode=newPath;
      headNode->counter++;
      // ModifyRoute(newPath,-1);
      if (newPath->nextHop[4].position%m_ToRNodes==0)
      {
        pathEntryAddressIndexTwo.push_back(newPath);
        pathEntryAddressIndexThree.push_back(newPath);
      }
      ModifyMappingTable(newPath,4);
      pathEntryTable.clear(); 
    }
  }
  else if (m_Node.level==3)
  {
    for (int j=1;j<=m_nPods;j++)//遍历所有的pod
    {
      for (int k=(j-1)*m_ToRNodes;k<j*m_ToRNodes;k++)//每个pod中有m_ToRNodes个node
      {
        high.position=(j-1)*m_LeafNodes+m_Node.position/(m_SpineNodes/m_LeafNodes);
        low.position=k;
        GeneratePath(high,low,&pathEntryTable);//一条路径
        struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
        *newPath=pathEntryTable[0];
        tailNode->next=newPath;
        tailNode=newPath;
        headNode->counter++; 
        // ModifyRoute(newPath,-1);
        if (newPath->nextHop[4].position%m_ToRNodes==0) 
        {
          pathEntryAddressIndexThree.push_back(newPath);
          ModifyMappingTable(newPath,1); 
        }
        ModifyMappingTable(newPath,4);
        pathEntryTable.clear();
      }
    }
  }
  // PrintPathEntryTable();
  // Logfout << GetNow() << "Initialize Path Table Over" << std::endl;
  //PrintMappingTable();

  // 添加路由
  struct pathtableentry * path;
  struct pathtableentry * select;
  path=headNode->next;//path用来遍历路径表
  select=headNode->next;//标记具有相同目的地址的第一条路径的位置
  // test
  // stringstream filename;
  // filename << "/var/log/MappingTableIndex-output.txt";
  // ofstream fout(filename.str().c_str(),ios::app);
  // end
  if (m_Node.level==1)
  {
    for (;select->nextHop[1].position%m_LeafNodes==0 && select!=NULL;)
    {
      path=select;
      stringstream cmd;
      cmd.str("");
      struct routeweight * routeWeightTemp=(struct routeweight *)malloc(sizeof(struct routeweight));
      routeWeightTemp->weight[0]=0;
      routeWeightTemp->weight[1]=0;
      routeWeightTemp->weight[2]=0;
      routeWeightTemp->weight[3]=0;    
      cmd << "ip route add 192.168." << path->nextHop[4].position+1 << ".0/24 ";
      for (int i=0;i<m_LeafNodes;i++)//先遍历路由
      {
        // infocom test
        if (i>=m_LeafNodes-2) cmd << "nexthop via " << GetGateway(path) << " ";
        // end
        // cmd << "nexthop via " << GetGateway(path) << " ";
        if (path->nextHop[2].level!=0)//ToR的路径表，该路径目的地址不在本Pod内
        {
          for (int j=0;j<(m_SpineNodes/m_LeafNodes);j++)//每条路由4条路径，遍历每条路径，记录路由的权重
          {
            // infocom test
            if (i>=m_LeafNodes-2 && j>=m_SpineNodes/m_LeafNodes-2) if (path->counter==0) routeWeightTemp->weight[i]+=1;
            // end
            // if (path->counter==0) routeWeightTemp->weight[i]+=1;
            path->recordWeight=routeWeightTemp;
            if (j!=(m_SpineNodes/m_LeafNodes)-1) path=path->next;//此处可能要判断链表尾
          }
        }
        else 
        {
          // infocom test
          if (i>=m_SpineNodes/m_LeafNodes/2) if (path->counter==0) routeWeightTemp->weight[i]+=1;//每条路由一条路径
          // end
          // if (path->counter==0) routeWeightTemp->weight[i]+=1;//每条路由一条路径
          path->recordWeight=routeWeightTemp;
          // path=path->next;
        }
        // infocom test
        if (i>=m_LeafNodes-2) cmd << "weight " << routeWeightTemp->weight[i] << " ";
        // end
        // cmd << "weight " << routeWeightTemp->weight[i] << " ";
        // if (path->next==NULL) break;
        if (i==0) select=path->next;
        if (i!=m_LeafNodes-1) 
        {
          if (path->nextHop[2].level!=0) for (int k=0;k<headNode->counter/m_LeafNodes-3;k++) path=path->next;
          else for (int k=0;k<headNode->counter/m_LeafNodes;k++) path=path->next;
        }
      }
      Logfout << cmd.str() << std::endl;
      system(cmd.str().c_str());
    }
    stringstream cmd;
    cmd.str("");
    cmd << "route add -net 192.168." << m_Node.position+1 << ".0/24 gw 192.168." << m_Node.position+1 << ".2 metric 20";
    system(cmd.str().c_str());
  } 
  else if (m_Node.level==2)
  {
    for (;select!=NULL;)
    {
      path=select;
      stringstream cmd;
      cmd.str("");
      struct routeweight * routeWeightTemp=(struct routeweight *)malloc(sizeof(struct routeweight));
      routeWeightTemp->weight[0]=0;
      routeWeightTemp->weight[1]=0;
      routeWeightTemp->weight[2]=0;
      routeWeightTemp->weight[3]=0; 
      if (path->nextHop[2].level!=0)//该路径目的地址不在本Pod内
      {
        cmd << "ip route add 192.168." << path->nextHop[4].position+1 << ".0/24 ";
        for (int i=0;i<(m_SpineNodes/m_LeafNodes);i++)
        {
          // infocom test
          Logfout << "gateway is " << GetGateway(path) << endl;
          if (i>=(m_SpineNodes/m_LeafNodes/2))
          {
            Logfout << "i=" << i << endl;
            cmd << "nexthop via " << GetGateway(path) << " ";
            if (path->counter==0) routeWeightTemp->weight[i]+=1;//每条路由一条路径
          } 
          // end
          // cmd << "nexthop via " << GetGateway(path) <<" ";
          // if (path->counter==0) routeWeightTemp->weight[i]+=1;//每条路由一条路径
          path->recordWeight=routeWeightTemp;
          // infocom test
          if (i>=(m_SpineNodes/m_LeafNodes/2)) cmd << "weight " << routeWeightTemp->weight[i] << " ";
          // end
          // cmd << "weight " << routeWeightTemp->weight[i] << " ";
          if (i!=(m_SpineNodes/m_LeafNodes)-1) for (int k=0;k<(headNode->counter-m_ToRNodes)/(m_SpineNodes/m_LeafNodes)-1;k++) path=path->next;         
        }
        select=path->next;
      }
      else
      {
        cmd << "route add -net 192.168." << path->nextHop[4].position+1 << ".0/24 gw " << GetGateway(path) << " metric 20";
        select=path->next;
      }
      Logfout << cmd.str() << std::endl;
      system(cmd.str().c_str());
    }
  }
  else if (m_Node.level==3)
  {
    stringstream cmd;
    path=select;
    for (;path!=NULL;)
    {
      cmd.str("");
      cmd << "route add -net 192.168." << path->nextHop[4].position+1 << ".0/24 gw " << GetGateway(path) << " metric 20";
      path=path->next;
      system(cmd.str().c_str());
    }
  }

  //映射表排序
  struct mappingtableentry mappingTemp;
  for (int i=0;i<mappingEntryTable.size()-1;i++)
  {
    for (int j=0;j<mappingEntryTable.size()-1-i;j++)
    {
      if (mappingEntryTable[j+1].high.level<mappingEntryTable[j].high.level)//底层链路排在前
      {
        mappingTemp=mappingEntryTable[j+1];
        mappingEntryTable[j+1]=mappingEntryTable[j];
        mappingEntryTable[j]=mappingTemp;
      }
      else if (mappingEntryTable[j+1].high.level==mappingEntryTable[j].high.level)//若level相等
      {
        if (mappingEntryTable[j+1].high.position<mappingEntryTable[j].high.position)//position按升序排列
        {
          mappingTemp=mappingEntryTable[j+1];
          mappingEntryTable[j+1]=mappingEntryTable[j];
          mappingEntryTable[j]=mappingTemp;
        }
      }
    }
  }
  // 映射表索引，如果拓扑是规则的，则可直接求数组下标，而不需要索引
  // 映射表条目众多，将与同一个结点相关联的下行链路组织到一起，记录第一个的位置在索引表中
  struct mappingentryindex tempIndex;
  tempIndex.high.level=0;
  tempIndex.high.position=0;
  tempIndex.low.level=0;
  tempIndex.low.position=0;
  for (int i=0;i<mappingEntryTable.size();i++)
  {
    if (tempIndex.high.level!=mappingEntryTable[i].high.level || tempIndex.high.position!=mappingEntryTable[i].high.position)
    {
      tempIndex.high=mappingEntryTable[i].high;
      tempIndex.low=mappingEntryTable[i].low;
      tempIndex.index=i;
      mappingEntryIndex.push_back(tempIndex);
    }  
  }
  // test
  // stringstream filename;
  // filename << "/var/log/MappingTableIndex-output.txt";
  // ofstream fout(filename.str().c_str(),ios::app);
  // fout.setf(ios::fixed, ios::floatfield);
  // fout.precision(9);
  // for (int i=0;i<mappingEntryIndex.size();i++)
  // {
  //   fout << mappingEntryIndex[i].high.level << "." << mappingEntryIndex[i].high.position << " ";
  //   fout << mappingEntryIndex[i].low.level << "." << mappingEntryIndex[i].low.position << " ";
  //   fout << mappingEntryIndex[i].index << std::endl;
  // }
  // end
  PrintMappingTable();
  PrintPathEntryTable();
  Logfout.close();
}

int 
Ipv4GlobalRouting::ModifyCounterAndWeight(struct pathtableentry * iter,int increment)
{
  ofstream Logfout("/var/log/center.log",ios::app);
  int oldCounter=iter->counter;
  iter->counter+=increment;
  // Logfout << "两个计数器为： " << oldCounter << " 和 " << iter->counter << std::endl;
  if (oldCounter && iter->counter) 
  {
    // Logfout << "返回值为0" << std::endl;
    return 0;//已经有链路坏了，再坏几条或者没有完全恢复
  }
  else//计数器从0变成1，或者从1变成0
  {
    if (iter->recordWeight!=NULL)
    {
      int index;
      if (m_Node.level==1) index=iter->nextHop[1].position%m_LeafNodes;
      else if (m_Node.level==2) index=iter->nextHop[1].position%(m_SpineNodes/m_LeafNodes);
      if (increment==-1)//路径恢复
      {
        iter->recordWeight->weight[index]++;
      }
      else if (increment==1)//路径故障
      {
        iter->recordWeight->weight[index]--;
      }
    }
    // Logfout << "返回值为1" << std::endl;
    return 1;
  } 
  Logfout.close();
}

void
Ipv4GlobalRouting::ModifyPathEntryTable(ident high,ident low,bool interfaceFlag)//
{
  ofstream Logfout("/var/log/center.log",ios::app);
  Logfout << GetNow() << "ModifyPathEntryTableFirst and interfaceFlag is " << interfaceFlag << std::endl;
  
  int increment;//路径的计数器：+1 or -1
  increment=(interfaceFlag==true)?-1:1;

  bool isFindMapping=false;
  struct pathtableentry *iter;//这一块路径的第一个地址

  for (int i=0;i<mappingEntryIndex.size();i++)//先在映射表的索引中找到大致范围
  {
    if (high.level==mappingEntryIndex[i].high.level && high.position==mappingEntryIndex[i].high.position)
    {
      for (int j=mappingEntryIndex[i].index;j<mappingEntryTable.size();j++)//在映射表中查找
      {
        if (low.level==mappingEntryTable[j].low.level && low.position==mappingEntryTable[j].low.position)
        {
          if (high.level==mappingEntryTable[j].high.level && high.position==mappingEntryTable[j].high.position)
          {
            iter=mappingEntryTable[j].address;
            isFindMapping=true;
            break;
          }
        }
      }
      break;
    }
  }
  if (isFindMapping==false) return;
  Logfout << GetNow() << "There is a mapping" << std::endl;
  
  if (m_Node.level==1)
  {
    // Logfout << GetNow() << "m_Node:" << m_Node.level << "." << m_Node.position << std::endl;
    if (high.level==2)//链路为2.X-1.X
    {
      if (low.position==m_Node.position)//直连链路
      {
        // Logfout << GetNow() << "2.X-1.X 直连链路 " << low.level << "." << low.position << "-" << high.level << "." << high.position << std::endl << std::endl;
        bool isNeedModify=false;
        struct pathtableentry *record;
        while (1)
        {
          isNeedModify=false;
          record=iter;
          if (iter->nextHop[2].level==0)//到本pod的路径
          {
            // iter->counter+=increment;
            // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
            if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);
            iter=iter->next;
            if (iter->nextHop[1].position!=iter->nextHop[1].position) break;
          }
          else if (iter->nextHop[2].level!=0)
          {
            for (int j=0;j<(m_SpineNodes/m_LeafNodes);j++)//每次修改m_SpineNodes/m_LeafNodes(4)条路径(目的地址相同)
            {
              // iter->counter+=increment;
              // if (IsNeedModifyRoute(iter->counter,increment)) isNeedModify=true;
              if (ModifyCounterAndWeight(iter,increment)) isNeedModify=true;
              iter=iter->next;
            }
            if (isNeedModify) ModifyRoute(record,increment);
          }
          if (iter==NULL || iter->nextHop[1].position!=high.position) break;
        }
      }
      else if (low.position/m_ToRNodes==m_Node.position/m_ToRNodes)//在同一个pod内,但不是直连
      {
        // Logfout << GetNow() << "2.X-1.X 在同一个pod内,但不是直连 " << low.level << "." << low.position << "-" << high.level << "." << high.position << std::endl;
        // iter->counter+=increment;
        // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
        if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);
      }
      else//不在同一个pod内
      {
        // Logfout << GetNow() << "2.X-1.X 不在同一个pod内 " << high.level << "." << high.position << "-" << low.level << "." << low.position << std::endl;
        bool isNeedModify=false;
        struct pathtableentry *record = iter;
        for (int j=0;j<(m_SpineNodes/m_LeafNodes);j++)//此时有m_SpineNodes/m_LeafNodes(4)条路径需要修改
        {
          // iter->counter+=increment;
          // if (IsNeedModifyRoute(iter->counter,increment)) isNeedModify=true;//要改
          if (ModifyCounterAndWeight(iter,increment)) isNeedModify=true;
          iter=iter->next;
        }
        if (isNeedModify) ModifyRoute(record,increment);
      }
    }
    else if (high.level==3)//链路为3.X-2.X
    {
      bool isEnd=false;
      bool isNeedModifyA=false;
      bool isNeedModifyB=true;
      struct pathtableentry * iterB;//其他相似（共用路由）路径
      //先查找这一块的第一条路径，存为iterB,因为不是双向链表
      if (low.position/m_LeafNodes+1==m_Pod)//2.X与源节点在同一个pod内
      {
        Logfout << GetNow() << "3.X-2.X 2.X与源节点在同一个pod内 " << low.level << "." << low.position << "-" << high.level << "." << high.position << std::endl;
        for (int j=0;j<pathEntryAddressIndexTwo.size();j++)
        {
          if ((pathEntryAddressIndexTwo[j])->nextHop[1].position==iter->nextHop[1].position)
          {
            iterB=pathEntryAddressIndexTwo[j];
            break;
          } 
        }
      }
      else//2.X与源节点不在同一个pod内
      {
        Logfout << GetNow() << "3.X-2.X 2.X与源节点不在同一个pod内 " << high.level << "." << high.position << "-" << low.level << "." << low.position << std::endl;
        for (int j=0;j<pathEntryAddressIndexThree.size();j++)
        {
          if ((pathEntryAddressIndexThree[j])->nextHop[3].position==iter->nextHop[3].position)
          {
            iterB=pathEntryAddressIndexThree[j];
            break;
          } 
        }
      }
      Logfout << GetNow() << "check index over" << std::endl;
      while (1)
      {
        // isNeedModifyA=false;
        // isNeedModifyB=true;
        // iter->counter+=increment;//先修改路径
        // if (IsNeedModifyRoute(iter->counter,increment)) isNeedModifyA=true;//是否从0变成1或者从1变成0
        // if (ModifyCounterAndWeight(iter,increment)) isNeedModifyA=true;
        // 查看其他m_SpineNodes/m_LeafNodes-1 (3)条路径
        // for (int k=0;k<(m_SpineNodes/m_LeafNodes);k++)
        // { 
        //   if (iterB==iter)
        //   {
        //     iterB=iterB->next;
        //     continue;
        //   } 
        //   if (iterB->counter==0) isNeedModifyB=false;//存在其他有效路径，即存在其他有效路由
        //   iterB=iterB->next;
        // }
        // if (isNeedModifyA==true && isNeedModifyB==true) ModifyRoute(iter,increment);
        // if (isNeedModifyA==true) ModifyRoute(iter,increment);
        // //终止条件
        // if (iterB==NULL) break;
        // else if (low.position/m_LeafNodes+1==m_Pod)//在同一个Pod内
        // {
        //   if (iterB->nextHop[2].level==0 || iterB->nextHop[1].position!=iter->nextHop[1].position)
        //   {
        //     isEnd=true;
        //     break;
        //   } 
        // }
        // else
        // {
        //   if (iterB->nextHop[3].level==0 || iterB->nextHop[3].position!=iter->nextHop[3].position)
        //   {
        //     isEnd=true;
        //     break;
        //   } 
        // }
        // if (isEnd) break; 
        // for (int k=0;k<(m_SpineNodes/m_LeafNodes);k++) iter=iter->next;//移动需要修改的路径
        // if (iter==NULL) break;

        if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);

        if (low.position/m_LeafNodes+1==m_Pod)//3.X-2.X在同一个Pod内
        {
          for (int k=0;k<(m_SpineNodes/m_LeafNodes);k++)
          {
            // 到链表尾或者链路的目的地址是在本pod内或者下一跳到了本pod内的其他leafnode
            if (iter->next==NULL || iter->next->nextHop[2].level==0 || iter->next->nextHop[1].position!=iter->nextHop[1].position)//
            {
              isEnd=true;
              break;
            }
            else iter=iter->next;
          }
        }
        else 
        {
          for (int k=0;k<(m_SpineNodes/m_LeafNodes);k++)
          {
            if (iter->next==NULL || iter->next->nextHop[3].level==0 || iter->next->nextHop[3].position!=iter->nextHop[3].position)//
            {
              isEnd=true;
              break;
            }
            else iter=iter->next;
          }
        }
        if (isEnd) break;
      }
    }
  }
  else if (m_Node.level==2)
  {
    //Logfout << GetNow() << "m_Node:" << m_Node.level << "." << m_Node.position << std::endl;
    if (high.level==2)
    { 
      if (high.position==m_Node.position)
      {
        //Logfout << GetNow() << "2.X-1.X 2.X与源节点在同一个pod内 " << high.level << "." << high.position << "-" << low.level << "." << low.position << std::endl;
        // iter->counter+=increment;
        // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
        if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);
      }
      else
      {
        bool isNeedModify=false;
        //Logfout << GetNow() << "2.X-1.X 2.X与源节点不在同一个pod内 " << high.level << "." << high.position << "-" << low.level << "." << low.position << std::endl;
        for (int j=0;j<(m_SpineNodes/m_LeafNodes);j++)//待路径全部修改完后统一修改路由
        {
          // iter->counter+=increment;
          // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
          if (ModifyCounterAndWeight(iter,increment)) isNeedModify=true;
          if (j!=m_SpineNodes/m_LeafNodes-1) iter=iter->next;
        }
        if (isNeedModify) ModifyRoute(iter,increment);
      }
    }
    else if (high.level==3)
    {
      //Logfout << GetNow() << "2.X-3.X " << std::endl;
      bool isEnd=false;
      while (1)
      {
        // iter->counter+=increment;
        // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
        if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);
        for (int j=0;j<(m_SpineNodes/m_LeafNodes);j++)
        {
          iter=iter->next;
          //终止条件
          if (iter->nextHop[1].level==0) 
          {
            isEnd=true;
            break;
          }
        } 
        if (isEnd) break;
      }
    }
  }
  else if (m_Node.level==3)
  {
    // Logfout << GetNow() << "m_Node:" << m_Node.level << "." << m_Node.position << std::endl;
    //Logfout << GetNow() << "Link is " << high.level << "." << high.position << "-" << low.level << "." << low.position << std::endl;
    if (high.level==3)
    {
      struct pathtableentry *record = iter;
      while (1)
      {
        // iter->counter+=increment;
        // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
        if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);
        //终止条件
        iter=iter->next;
        if (iter==NULL || iter->nextHop[1].position!=record->nextHop[1].position) break;
      }
    }
    else if (high.level==2)
    {
      // iter->counter+=increment;
      // if (IsNeedModifyRoute(iter->counter,increment)) ModifyRoute(iter,increment);
      if (ModifyCounterAndWeight(iter,increment)) ModifyRoute(iter,increment);
    }
  }
  //std::cout << std::endl << std::endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::ModifyNodeConfiguration(int newNPods)
{
  int oldNPods=m_nPods;
  m_nPods=newNPods;
  if (m_Node.level==0)//master需要更新linktable
  {
    for (int i=0;i<m_SpineNodes;i++)
    {
      for (int j=oldNPods+1;j<=m_nPods;j++) ModifyLinkTableBySpineNode(i,j);
    }
    PrintMasterLinkTable();
  }
  else //node需要更新路径表
  {
    ident high,low;
    high.level=2;
    low.level=1;
    for (int i=oldNPods;i<m_nPods;i++)//遍历新添加的每一个pod
    {
      if (m_Node.level==1)
      {
        for (int j=i*m_LeafNodes;j<(i+1)*m_LeafNodes;j++)//遍历pod中合适的leafnode,此时每个pod中的leafnode都需要遍历
        {
          high.position=j;
          for (int k=i*m_ToRNodes;k<(i+1)*m_ToRNodes;k++)//每个leafnode又会连接到torndoes个tornode，则此时有torndoes条新的2.X-1.X链路
          {
            low.position=k;
            AddNodeProcess(high,low);
          }
        }
      }
      else if (m_Node.level==2)
      {
        high.position=i*m_LeafNodes+m_Node.position%m_LeafNodes;//此时每个pod中只有一个leafnode需要
        for (int k=i*m_ToRNodes;k<(i+1)*m_ToRNodes;k++)//每个leafnode又会连接到torndoes个tornode，则此时有torndoes条新的2.X-1.X链路
        {
          low.position=k;
          AddNodeProcess(high,low);
        }
      }
      else if (m_Node.level==3)
      {
        high.position=i*m_LeafNodes+m_Node.position/(m_SpineNodes/m_LeafNodes);//此时每个pod中只有一个leafnode需要
        for (int k=i*m_ToRNodes;k<(i+1)*m_ToRNodes;k++)//每个leafnode又会连接到torndoes个tornode，则此时有torndoes条新的2.X-1.X链路
        {
          low.position=k;
          AddNodeProcess(high,low);
        }
      }
    }
    //PrintPathEntryTable();
    //PrintMappingTable();
  }
}

void 
Ipv4GlobalRouting::ModifyLinkTableBySpineNode(int i,int j)//spinenode i 到pod j 的tornode的所有链路
{
  struct linktableentry linkTableEntry;
  linkTableEntry.high.level=3;
  linkTableEntry.low.position=i;
  linkTableEntry.high.level=2;
  linkTableEntry.low.position=(j-1)*m_LeafNodes+i/(m_SpineNodes/m_LeafNodes);
  //linkTableEntry.isNewLink=true;
  //linkTableEntry.flag=false;
  linkTableEntry.isNewLink=false;//test
  linkTableEntry.flag=true;//test
  masterLinkTable.push_back(linkTableEntry);
  if (i%(m_SpineNodes/m_LeafNodes)==0)
  {
    linkTableEntry.high.level=2;
    linkTableEntry.high.position=(j-1)*m_LeafNodes+i/(m_SpineNodes/m_LeafNodes);
    linkTableEntry.low.level=1;
    for (int k=(j-1)*m_ToRNodes;k<j*m_ToRNodes;k++) 
    {
      linkTableEntry.low.position=k;
      //linkTableEntry.isNewLink=true;
      //linkTableEntry.flag=false;
      linkTableEntry.isNewLink=false;//test
      linkTableEntry.flag=true;//test
      masterLinkTable.push_back(linkTableEntry);
    }
  }
}

void* 
Ipv4GlobalRouting::MasterLinkThread(void* threadParam)
{
  ofstream Logfout("/var/log/center.log",ios::app);
  Logfout.setf(ios::fixed, ios::floatfield);
  Logfout.precision(9);//设置保留的小数点位数
  threadparam *param=(threadparam *)threadParam;
  Ipv4GlobalRouting* pThis=param->m_ipv4GlobalRouting;
  int index=param->index;
  double threadStartTime=pThis->GetSystemTime();
  Logfout << pThis->GetNow() << "定时器开始计时，系统时间为 " << pThis->GetSystemTime() << "......" << std::endl;
  while (1)
  {
    usleep(pThis->m_defaultTimer);
    if ((pThis->GetSystemTime()-threadStartTime)*1000000>=pThis->masterLinkTable[index].linkUpdateTimer)
    {
      break;
    } 
  }
  Logfout << std::endl << pThis->GetNow() << "定时器超时，开始处理，系统时间为 " << pThis->GetSystemTime() << "......" << std::endl;
  if (pThis->masterLinkTable[index].lastUpdateFlag!=pThis->masterLinkTable[index].flag)//两个状态不同则处理
  {
    Logfout << pThis->GetNow() << "两个状态不同" << std::endl;
    m_node->SendMessageToNode(pThis->masterLinkTable[index].high,pThis->masterLinkTable[index].low,pThis->masterLinkTable[index].lastUpdateFlag);
    pThis->masterLinkTable[index].flag=pThis->masterLinkTable[index].lastUpdateFlag;
    pThis->masterLinkTable[index].lastUpdateFlag=NULL;
    pThis->masterLinkTable[index].lastUpdateTime=pThis->GetSystemTime();
    pThis->masterLinkTable[index].isStartThread=false;
    pThis->masterLinkTable[index].linkUpdateTimer=pThis->m_defaultTimer;
    pThis->masterLinkTable[index].temp=0;
    pThis->PrintMasterLinkTable();//故障定位
  }
  Logfout << std::endl;
  Logfout.close();
}

void
Ipv4GlobalRouting::ModifyMasterLinkTable(ident high,ident low,bool * isNeedToNotice,bool interfaceFlag)
{//此处可优化，
  ofstream Logfout("/var/log/center.log",ios::app);
  Logfout.setf(ios::fixed, ios::floatfield);
  Logfout.precision(6);//设置保留的小数点位数
  int i=0;
  if (high.level==3) i=m_nPods*high.position;
  else if (high.level==2) i=m_ToRNodes*high.position+m_SpineNodes*m_nPods;
  // Logfout << GetNow() << "index is " << i << " high.level is " << high.level << " high.position is " << high.position << " m_ToRNodes is " << m_ToRNodes << " and m_SpineNodes is " << m_SpineNodes << " and m_nPods is " << m_nPods << std::endl;
  while (1)
  {
    if (masterLinkTable[i].high.level==high.level && masterLinkTable[i].high.position==high.position && masterLinkTable[i].low.level==low.level && masterLinkTable[i].low.position==low.position)
    {
      // Logfout << GetNow() << "find link" << std::endl;
      if (masterLinkTable[i].flag)//master的链路表中该条链路此时正常工作
      {
        // Logfout << GetNow() << "Link is normal---" << endl;
        if (interfaceFlag) *isNeedToNotice=false;//Node信息表示该条链路正常，则不做反应
        else //Node信息表示该条链路故障
        {
          //两次链路变化的时间间隔不管是否大于定时器，都要广播该信息
          *isNeedToNotice=true;
          masterLinkTable[i].lastUpdateTime=GetSystemTime(); 
          masterLinkTable[i].flag=false;
          masterLinkTable[i].temp++;//test
        }
      }
      else//master的链路表中该条链路此时异常
      {
        //正常会有两个结点上报同一条链路，所以必须进行判断，因为此时是不需要double timer的，这部分代码留以后写
        if (interfaceFlag && masterLinkTable[i].isNewLink==true) //若是master刚初始化的链路，则Node信息验证该条链路已经正常工作
        {
          Logfout << GetNow() << "A new link---" << endl;
          *isNeedToNotice=false;//不广播
          masterLinkTable[i].flag=true;
          masterLinkTable[i].isNewLink=false;
          masterLinkTable[i].lastUpdateTime=GetSystemTime();
        }
        else if (interfaceFlag && masterLinkTable[i].temp==1)
        {
          *isNeedToNotice=true;
           masterLinkTable[i].flag=true;
           masterLinkTable[i].lastUpdateTime=GetSystemTime();
           masterLinkTable[i].temp++;//test
        }
        else 
        {
          double intervalTime=(GetSystemTime()-masterLinkTable[i].lastUpdateTime)*1000000;
          Logfout << GetNow() << "intervalTime is " << intervalTime << std::endl;
          if (intervalTime>m_defaultTimer)//两次链路变化的时间间隔大于定时器，广播该信息
          {
            *isNeedToNotice=true;
            masterLinkTable[i].flag=true;
          }
          else//如果间隔小于定时器，则不修改Master中该条链路的状态，不广播，但要记录Node上传的最新的链路信息，待定时器超时后处理
          {
            Logfout << GetNow() << "间隔小于定时器" << std::endl;
            *isNeedToNotice=false;
            masterLinkTable[i].lastUpdateFlag=interfaceFlag;//此处要判断
            //创建线程，定时器超时后处理
            if (masterLinkTable[i].isStartThread==false)
            {
              // struct threadparam param;
              Logfout << GetNow() << "Create thread" << std::endl;
              threadparam *param=new threadparam();
              param->m_ipv4GlobalRouting=this;
              param->index=i;
              // Logfout << GetNow() << "param.delay is " << (masterLinkTable[i].linkUpdateTimer-(GetSystemTime()-masterLinkTable[i].lastUpdateTime)*1000000) << " param.index is " << param->index << std::endl;
              // if(pthread_create(&masterLink_thread,NULL,MasterLinkThread,(void *)&param)<0)
              if(pthread_create(&masterLink_thread,NULL,MasterLinkThread,param)<0)
              {
                Logfout << GetNow() << "could not create thread" << std::endl;
                return;
              }
              masterLinkTable[i].isStartThread=true;
              // if (masterLinkTable[i].linkUpdateTimer<2000000) masterLinkTable[i].linkUpdateTimer+=100000;
            }
            else 
            {
              // if (masterLinkTable[i].linkUpdateTimer<2000000) masterLinkTable[i].linkUpdateTimer+=100000;//累加一次定时器
              masterLinkTable[i].linkUpdateTimer+=m_defaultTimer;
              Logfout << GetNow() << "定时器double" << std::endl;
            }
          }
          masterLinkTable[i].lastUpdateTime=GetSystemTime();//此处要判断
        }
        // if (interfaceFlag)//Node上报信息表示该条链路已经恢复
        // {
        //   if (masterLinkTable[i].isNewLink==true) //若是master刚初始化的链路，则Node信息验证该条链路已经正常工作
        //   {
        //     Logfout << GetNow() << "A new link---" << endl;
        //     *isNeedToNotice=false;//不广播
        //     masterLinkTable[i].flag=true;
        //     masterLinkTable[i].isNewLink=false;
        //     masterLinkTable[i].lastUpdateTime=GetSystemTime();
        //   }
        //   else 
        //   {
        //     double intervalTime=(GetSystemTime()-masterLinkTable[i].lastUpdateTime)*1000000;
        //     Logfout << GetNow() << "intervalTime is " << intervalTime << std::endl;
        //     if (intervalTime>masterLinkTable[i].linkUpdateTimer)//两次链路变化的时间间隔大于定时器，广播该信息
        //     {
        //       *isNeedToNotice=true;
        //       masterLinkTable[i].flag=true;
        //       masterLinkTable[i].lastUpdateTime=GetSystemTime();
        //     }
        //     else//如果间隔小于定时器，则不修改Master中该条链路的状态，不广播，但要记录Node上传的最新的链路信息，待定时器超时后处理
        //     {
        //       Logfout << GetNow() << "间隔小于定时器" << std::endl;
        //       *isNeedToNotice=false;
        //       masterLinkTable[i].lastUpdateFlag=interfaceFlag;
        //       //创建线程，定时器超时后处理
        //       if (masterLinkTable[i].isStartThread==false)
        //       {
        //         // struct threadparam param;
        //         Logfout << GetNow() << "Create thread" << std::endl;
        //         threadparam *param=new threadparam();
        //         param->m_ipv4GlobalRouting=this;
        //         param->index=i;
        //         // Logfout << GetNow() << "param.delay is " << (masterLinkTable[i].linkUpdateTimer-(GetSystemTime()-masterLinkTable[i].lastUpdateTime)*1000000) << " param.index is " << param->index << std::endl;
        //         // if(pthread_create(&masterLink_thread,NULL,MasterLinkThread,(void *)&param)<0)
        //         if(pthread_create(&masterLink_thread,NULL,MasterLinkThread,param)<0)
        //         {
        //           Logfout << GetNow() << "could not create thread" << std::endl;
        //           return;
        //         }
        //         masterLinkTable[i].isStartThread=true;
        //       }
        //       else 
        //       {
        //         masterLinkTable[i].linkUpdateTimer+=m_defaultTimer;//累加一次定时器
        //         Logfout << GetNow() << "定时器double" << std::endl;
        //       }
        //     }
        //   }
        // }
        // else //正常是会有两个结点上报同一条链路，但是此时是不需要double timer的，这部分代码留以后写
        // {
        //   masterLinkTable[i].linkUpdateTimer+=m_defaultTimer;//累加一次定时器
        //   Logfout << GetNow() << "定时器double" << std::endl;
        //   *isNeedToNotice=false;
        // }
      }
      break;
    }
    i++;
    if (high.level==3 && i>=m_nPods*(high.position+1)) break;
    else if (high.level==2 && i>=(high.position+1)*m_ToRNodes+m_SpineNodes*m_nPods) break;
  }
  Logfout.close();
  if (*isNeedToNotice) PrintMasterLinkTable();//故障定位
  //std::cout << "Ipv4GlobalRouting    ModifyMasterLinkTable isNewLink:" << isNewLink << " isNeedToNotice:" << *isNeedToNotice << " isFind:" << isFind << std::endl;
}

int 
Ipv4GlobalRouting::IsNeedModifyRoute(int counter,int increment)//判断路径变化是否需要修改路由，计数器是否从0变成1或者从1变成0
{
  //std::cout << "Ipv4GlobalRouting    IsNeedModifyRoute" << std::endl;
  int oldCounter=counter-increment;
  if (oldCounter && counter) return 0;
  else return 1;
}

string 
Ipv4GlobalRouting::GetGateway(struct pathtableentry * iter)
{
  string gateway;
  if (iter->nextHop[0].level==1) gateway=Allocate_ip((*iter).nextHop[1].level,(*iter).nextHop[1].position,m_Node.position%m_ToRNodes+m_nMaster+1+m_SpineNodes/m_LeafNodes);
  else if (iter->nextHop[0].level==2) 
  {
    if (iter->nextHop[1].level==0) gateway=Allocate_ip((*iter).nextHop[4].level,(*iter).nextHop[4].position,(*iter).destInterfaceIndex);
    else if (iter->nextHop[1].level==3) gateway=Allocate_ip((*iter).nextHop[1].level,(*iter).nextHop[1].position,m_Node.position/m_LeafNodes+1+m_nMaster);
  }
  else if (iter->nextHop[0].level==3) gateway=Allocate_ip((*iter).nextHop[1].level,(*iter).nextHop[1].position,m_Node.position%4+1+m_nMaster);
  return gateway;
}

// void
// Ipv4GlobalRouting::ModifyRoute(string destAddress,string gateway,string subMask,int cmd,int metric)//cmd==1表示add，cmd==0表示del，目前只能添加net 
// Ipv4GlobalRouting::ModifyRoute(struct pathtableentry * iter,int increment)//修改路由(临时)      
// { 
//   //临时
//   string destAddress,gateway,subMask;
//   int cmd,metric;
//   if (iter->nextHop[4].position==0) destAddress="192.168.1.0";
//   else if (iter->nextHop[4].position==1) destAddress="192.168.2.0";
//   gateway=GetGateway(iter);
//   subMask = "255.255.255.0";
//   //链路恢复，如果要改路由，则是添加路由，标志位0表示正常
//   if (increment==-1) cmd=1;
//   else if (increment==1) cmd=0;
//   metric=0;
//   //end
  
//   ofstream Logfout("/var/log/center.log",ios::app);
  
//   // create the control socket.
//   int fd=socket(AF_INET,SOCK_DGRAM,0);
//   if (fd<0) Logfout << GetNow() << "Creating the control socket failed" << std::endl;
 
//   //test
//   Logfout << GetNow() << "The route is " << destAddress << " " << gateway << " " << subMask << " cmd is " << cmd << std::endl;
//   //end 
  
//   struct rtentry route;
//   memset(&route,0x00,sizeof(route));

//   route.rt_flags=RTF_UP;

//   struct sockaddr_in *addr=(struct sockaddr_in *)&route.rt_gateway;

//   addr->sin_family=AF_INET;
//   addr->sin_addr.s_addr=inet_addr(gateway.c_str());
//   route.rt_flags|=RTF_GATEWAY;

//   addr=(struct sockaddr_in*)&route.rt_dst;
//   addr->sin_family=AF_INET;
//   addr->sin_addr.s_addr=inet_addr(destAddress.c_str());

//   addr=(struct sockaddr_in*)&route.rt_genmask;
//   addr->sin_family=AF_INET;
//   addr->sin_addr.s_addr=inet_addr(subMask.c_str());

//   route.rt_metric=metric;
 
//   if (cmd==1)
//   {
//     if (!ioctl(fd,SIOCADDRT,&route))
//     {
//       Logfout << GetNow() << "Add Route success" << std::endl;
//       close(fd);
//     } 
//   } 
//   else if (cmd==0)
//   {
//     if (!ioctl(fd,SIOCDELRT,&route))
//     {
//       Logfout << GetNow() << "Del Route success" << std::endl;
//       close(fd);
//     } 
//   } 
//   close(fd);
//   Logfout.close();
// }

void
Ipv4GlobalRouting::ModifyRoute(struct pathtableentry * iter,int increment)//修改路由(临时)
{
  ofstream Logfout("/var/log/center.log",ios::app);
  // Logfout << "ModifyRoute" << std::endl;
  // 根据目的地址来删除
  stringstream cmd;
  cmd.str("");
  if (iter->recordWeight==NULL)//对应路由不带权重
  {
    if (increment==-1)//add route
    {
      cmd << "route add -net 192.168." << iter->nextHop[4].position+1 << ".0/24 gw " << GetGateway(iter) << " metric 20";
    }
    else 
    {
      cmd << "route del -net 192.168." << iter->nextHop[4].position+1 << ".0/24 gw " << GetGateway(iter) << " metric 20";
    }
    system(cmd.str().c_str());
  }
  else//路由带权重
  {
    cmd << "route del -net 192.168." << iter->nextHop[4].position+1 << ".0/24";
    system(cmd.str().c_str());
    cmd.str("");

    int temp,readyRouteNum=0;
    if (m_Node.level==1) temp=m_LeafNodes;
    else if (m_Node.level==2) temp=m_SpineNodes/m_LeafNodes;
    int nextHopInterface=m_SpineNodes/m_LeafNodes+1;
    ident node;
    node.level=0;
    node.position=0;
    for (int i=0;i<temp;i++)//统计可用路由条数
    {
      if (iter->recordWeight->weight[i]!=0) readyRouteNum++;
    }
    if (readyRouteNum==1)//只剩一条可用路由，不带权重
    {
      for (int i=0;i<temp;i++)
      {
        if (iter->recordWeight->weight[i]!=0)
        {
          node=GetNextHopIdent(m_Node.level,m_Node.position,i+1+m_nMaster,&nextHopInterface);
          cmd << "route add -net 192.168." << iter->nextHop[4].position+1 << ".0/24 gw " << Allocate_ip(node.level,node.position,nextHopInterface) << " metric 20";
        } 
      }
      system(cmd.str().c_str());
    }
    else if (readyRouteNum==0);//全挂了，不处理
    else
    {
      cmd << "ip route add 192.168." << iter->nextHop[4].position+1 << ".0/24 ";
      for (int i=0;i<temp;i++)
      {
        if (iter->recordWeight->weight[i]!=0)
        {
          node=GetNextHopIdent(m_Node.level,m_Node.position,i+1+m_nMaster,&nextHopInterface);
          cmd << "nexthop via " << Allocate_ip(node.level,node.position,nextHopInterface) << " weight " << iter->recordWeight->weight[i] << " ";
        }
      }
      system(cmd.str().c_str());
    } 
    // Logfout << "可用路由条数：" << readyRouteNum << std::endl;
  }
  Logfout << GetNow() << "------" << cmd.str() << std::endl;
  Logfout.close();
}

void 
Ipv4GlobalRouting::InsertPathEntryTable(struct pathtableentry * begin,struct pathtableentry * end,struct pathtableentry * newPathBegin,struct pathtableentry * newPathEnd)
{
  //std::cout << "Ipv4GlobalRouting    InsertPathEntryTable start" << std::endl;
  struct pathtableentry * iter=(struct pathtableentry *)malloc(sizeof(struct pathtableentry));
  iter=begin;
  for (;iter->next!=end;) iter=iter->next;
  if (newPathEnd==NULL) newPathBegin->next=iter->next;
  else newPathEnd->next=iter->next;
  iter->next=newPathBegin;
  if (newPathEnd!=NULL) headNode->counter+=m_SpineNodes/m_LeafNodes;//某些情况下会添加m_SpineNodes/m_LeafNodes(4)条路径
  else headNode->counter++;
}

void 
Ipv4GlobalRouting::AddNodeProcess(ident high,ident low)
{
  bool isFind=false;
  std::vector<struct pathtableentry> pathEntryTable;
  if (m_Node.level==1 || m_Node.level==2)
  {
    if (high.position/m_LeafNodes+1==m_Pod)//在同一个Pod内
    {
      GeneratePath(high,low,&pathEntryTable);
      struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
      *newPath=pathEntryTable[0];
      //插入路径表
      if (m_Node.level==1)
      {
        if (newPath->nextHop[1].position%m_LeafNodes==m_SpineNodes/m_LeafNodes-1) InsertPathEntryTable(pathEntryAddressIndexTwo[(newPath->nextHop[1].position%m_LeafNodes)*2+1],NULL,newPath,NULL);
        else InsertPathEntryTable(pathEntryAddressIndexTwo[(newPath->nextHop[1].position%m_LeafNodes)*2+1],pathEntryAddressIndexTwo[(newPath->nextHop[1].position%m_LeafNodes)*2+2],newPath,NULL);
      } 
      else if (m_Node.level==2) InsertPathEntryTable(pathEntryAddressIndexTwo[1],NULL,newPath,NULL);
      ModifyMappingTable(newPath,4);
      ModifyRoute(newPath,-1);
      newPath->isAddRouteToKernel=true;
    } 
    else
    {
      int i,pendingNode;
      GeneratePath(high,low,&pathEntryTable);
      struct pathtableentry * newPathBegin;
      struct pathtableentry * newPathEnd;
      //判断索引表中是否存在对应的索引,此处只对pathEntryAddressThree检索，实际上所有的索引表都需要遍历，比如leafnode从4增加到5时，就需要更新pathEntryAddressTwo和pathEntryAddressOne
      if (m_Node.level==1) pendingNode=3;
      else if (m_Node.level==2) pendingNode=2;
      for (i=0;i<pathEntryAddressIndexThree.size();i++)//检查是否有到目的地址pod的索引
      {
        if (pathEntryTable[0].nextHop[pendingNode].level==pathEntryAddressIndexThree[i]->nextHop[pendingNode].level && pathEntryTable[0].nextHop[pendingNode].position==pathEntryAddressIndexThree[i]->nextHop[pendingNode].position)
        {
          isFind=true;
          break;
        }
      }
      for (int j=0;j<pathEntryTable.size();j++)//把产生的路径连接起来
      {
        struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
        *newPath=pathEntryTable[j];
        if (j==0) ModifyMappingTable(newPath,4);
        if (!isFind) ModifyMappingTable(newPath,pendingNode);//若没有找到对应的索引，则说明第3和第4跳组成的路径是没有保存在映射表中的
        if (j==0)
        {
          newPathBegin=newPath;
          newPathEnd=newPath;
          continue;
        } 
        newPathEnd->next=newPath;
        newPathEnd=newPath;
        if (j==pathEntryTable.size()-1) break;
      }
      if (isFind)
      {
        if (i==pathEntryAddressIndexThree.size()) InsertPathEntryTable(pathEntryAddressIndexThree[i],NULL,newPathBegin,newPathEnd);
        else InsertPathEntryTable(pathEntryAddressIndexThree[i],pathEntryAddressIndexThree[i+1],newPathBegin,newPathEnd);
      }
      else if (!isFind)
      {
        std::vector<struct pathtableentry *>::iterator iter=pathEntryAddressIndexThree.end()-1;//从末尾开始遍历
        if (m_Node.level==1)
        {
          for (;iter>=pathEntryAddressIndexThree.begin();iter--)//寻找合适的位置插入路径
          {
            if ((*iter)->nextHop[1].position==newPathBegin->nextHop[1].position && (*iter)->nextHop[1].level==newPathBegin->nextHop[1].level)
            {
              if ((*iter)->nextHop[2].position==newPathBegin->nextHop[2].position && (*iter)->nextHop[2].level==newPathBegin->nextHop[2].level)
              {
                InsertPathEntryTable(*iter,*(iter+1),newPathBegin,newPathEnd);
                pathEntryAddressIndexThree.insert(iter+1,newPathBegin);//更新pathEntryAddressThree
                break;
              }
            }
          }
        }
        else if (m_Node.level==2)
        {
          for (;iter>=pathEntryAddressIndexThree.begin();iter--)
          {
            if ((*iter)->nextHop[1].position==newPathBegin->nextHop[1].position && (*iter)->nextHop[1].level==newPathBegin->nextHop[1].level)
            {
              InsertPathEntryTable(*iter,*(iter+1),newPathBegin,newPathEnd);
              pathEntryAddressIndexThree.insert(iter+1,newPathBegin);//更新pathEntryAddressThree
              break;
            }
          }
        }
      }
      if (m_Node.level==1) 
      {
        ModifyRoute(newPathBegin,-1);
        for (;newPathBegin!=newPathEnd;)//虽然公用路由，但仍需要将其他的路径的isAddRouteToKernel标志位修改
        {
          newPathBegin->isAddRouteToKernel=true;
          newPathBegin=newPathBegin->next;
        }
        newPathBegin->isAddRouteToKernel=true;
      }
      else if (m_Node.level==2)//此时路径不共用路由
      {
        for (;newPathBegin!=newPathEnd;)
        {
          ModifyRoute(newPathBegin,-1);
          newPathBegin->isAddRouteToKernel=true;
          newPathBegin=newPathBegin->next;
        }
        ModifyRoute(newPathBegin,-1);
        newPathBegin->isAddRouteToKernel=true;
      }
    }
    pathEntryTable.clear(); 
  }
  else if (m_Node.level==3)
  {
    GeneratePath(high,low,&pathEntryTable);
    struct pathtableentry * newPath= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
    *newPath=pathEntryTable[0];
    ModifyMappingTable(newPath,4);
    std::vector<struct pathtableentry *>::iterator iter=pathEntryAddressIndexThree.end()-1;
    for (;iter>=pathEntryAddressIndexThree.begin();iter--)
    {
      if ((*iter)->nextHop[1].position==newPath->nextHop[1].position && (*iter)->nextHop[1].level==newPath->nextHop[1].level)
      {
        isFind=true;
        if (iter==pathEntryAddressIndexThree.end()-1) InsertPathEntryTable(*iter,NULL,newPath,NULL);
        else InsertPathEntryTable(*iter,*(iter+1),newPath,NULL);
        break;
      }
    }
    if (!isFind) 
    {
      ModifyMappingTable(newPath,1);
      InsertPathEntryTable(pathEntryAddressIndexThree[pathEntryAddressIndexThree.size()-1],NULL,newPath,NULL);
      pathEntryAddressIndexThree.insert(pathEntryAddressIndexThree.end(),newPath);
    }
    ModifyRoute(newPath,-1);
    newPath->isAddRouteToKernel=true;
  }
}

void 
Ipv4GlobalRouting::PrintMasterLinkTable()
{
  stringstream filename;
  filename << "/var/log/MasterLinkTable-output.txt";
  ofstream fout(filename.str().c_str(),ios::trunc);
  fout.setf(ios::fixed, ios::floatfield);
  fout.precision(9);//设置保留的小数点位数
  //fout << "Node" << m_Node.level << "." << m_Node.position << "'s linktable size is " << masterLinkTable.size() << " time is " << Simulator::Now().GetSeconds() << std::endl;
  // fout << "Node" << m_Node.level << "." << m_Node.position << "'s linktable size is " << masterLinkTable.size() << " time is " << GetNow() << std::endl;
  int counter=0;
  std::vector<struct linktableentry>::iterator iter=masterLinkTable.begin();
  // while (iter!=masterLinkTable.end())
  // {
  //   fout << (*iter).high.level << "." << (*iter).high.position << "   ";
  //   fout << (*iter).low.level << "." << (*iter).low.position << "   ";
  //   fout << (*iter).isNewLink << "   " << (*iter).flag << "    " << counter++ << std::endl;
  //   iter++;
  // }
  // 远程可视化界面用
  while (iter!=masterLinkTable.end())
  {
    fout << (*iter).high.level << "." << (*iter).high.position << "--";
    fout << (*iter).low.level << "." << (*iter).low.position << " ";
    fout << (*iter).flag << std::endl;
    iter++;
  }
  fout << std::endl;
  fout.close();
}

void 
Ipv4GlobalRouting::PrintMappingTable()
{
  stringstream filename;
  // filename << "/var/log/MappingTable-output.txt";
  filename << "/home/guolab/output/MappingTable-output.txt";
  ofstream fout(filename.str().c_str(),ios::app);
  fout.setf(ios::fixed, ios::floatfield);
  fout.precision(9);//设置保留的小数点位数
  /*fout << "Node" << m_Node.level << "." << m_Node.position << "'s mappingtable size is " << mappingEntryTable.size() << " time is " << Simulator::Now().GetSeconds() << std::endl;*/
  vector<struct mappingtableentry>::iterator iter=mappingEntryTable.begin();
  stringstream source,destination;
  for (;iter<mappingEntryTable.end();iter++)
  {
    source.str("");
    destination.str("");
    source << (*iter).high.level << "." << (*iter).high.position;
    destination << (*iter).low.level << "." << (*iter).low.position;
    fout << std::setiosflags (std::ios::left) << std::setw (10) << source.str() << std::setiosflags (std::ios::left) << std::setw (10) << destination.str() << std::endl;
    //fout << ((*iter).address)->nextHop[0].level << "." << ((*iter).address)->nextHop[0].position << "\t" << ((*iter).address)->nextHop[1].level << "." << ((*iter).address)->nextHop[1].position << "\t";
    //fout << ((*iter).address)->nextHop[2].level << "." << ((*iter).address)->nextHop[2].position << "\t" << ((*iter).address)->nextHop[3].level << "." << ((*iter).address)->nextHop[3].position << "\t";
    //fout << ((*iter).address)->nextHop[4].level << "." << ((*iter).address)->nextHop[4].position << "\t" << ((*iter).address)->destInterfaceIndex << "\t" << ((*iter).address)->counter << "\t";
  }
  fout << std::endl;
  fout.close();
}
void 
Ipv4GlobalRouting::PrintPathEntryTable()
{
  stringstream filename;
  // filename << "/var/log/PathEntryTable-output.txt";
  filename << "/home/guolab/output/PathEntryTable-output.txt";
  ofstream fout(filename.str().c_str(),ios::app);
  fout.setf(ios::fixed, ios::floatfield);
  fout.precision(9);//设置保留的小数点位数
  /*fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathentrytable size is " << headNode->counter << " time is " << Simulator::Now().GetSeconds() << std::endl;*/
  fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathentrytable size is " << headNode->counter << " time is " << GetNow() << std::endl;
  int counter=0;
  struct pathtableentry * iter= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));
  iter=headNode->next;
  while (iter!=NULL)
  {
    fout << (*iter).nextHop[0].level << "." << (*iter).nextHop[0].position << "   ";
    fout << (*iter).nextHop[1].level << "." << (*iter).nextHop[1].position << "   ";
    fout << (*iter).nextHop[2].level << "." << (*iter).nextHop[2].position << "   ";
    fout << (*iter).nextHop[3].level << "." << (*iter).nextHop[3].position << "   ";
    fout << (*iter).nextHop[4].level << "." << (*iter).nextHop[4].position << "   ";
    fout << (*iter).destInterfaceIndex << "   " << (*iter).counter << "   " << (*iter).isAddRouteToKernel << "   " << counter++ << std::endl;
    iter=iter->next;
  }
  fout << std::endl;

  fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathEntryAddressIndexOne size is " << pathEntryAddressIndexOne.size() << " time is " << GetNow() << std::endl;
  counter=0;
  for (int i=0;i<pathEntryAddressIndexOne.size();i++)
  {
    fout << (pathEntryAddressIndexOne[i])->nextHop[0].level << "." << (pathEntryAddressIndexOne[i])->nextHop[0].position << "   ";
    fout << (pathEntryAddressIndexOne[i])->nextHop[1].level << "." << (pathEntryAddressIndexOne[i])->nextHop[1].position << "   ";
    fout << (pathEntryAddressIndexOne[i])->nextHop[2].level << "." << (pathEntryAddressIndexOne[i])->nextHop[2].position << "   ";
    fout << (pathEntryAddressIndexOne[i])->nextHop[3].level << "." << (pathEntryAddressIndexOne[i])->nextHop[3].position << "   ";
    fout << (pathEntryAddressIndexOne[i])->nextHop[4].level << "." << (pathEntryAddressIndexOne[i])->nextHop[4].position << "   ";
    fout << (pathEntryAddressIndexOne[i])->destInterfaceIndex << "   " << (pathEntryAddressIndexOne[i])->counter << "   " << (pathEntryAddressIndexOne[i])->isAddRouteToKernel << "   " << counter++ << std::endl;
  }
  fout << std::endl;
  /*fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathEntryAddressIndexTwo size is " << pathEntryAddressIndexTwo.size() << " time is " << Simulator::Now().GetSeconds() << std::endl;*/
  fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathEntryAddressIndexTwo size is " << pathEntryAddressIndexTwo.size() << " time is " << GetNow() << std::endl;
  counter=0;
  for (int i=0;i<pathEntryAddressIndexTwo.size();i++)
  {
    fout << (pathEntryAddressIndexTwo[i])->nextHop[0].level << "." << (pathEntryAddressIndexTwo[i])->nextHop[0].position << "   ";
    fout << (pathEntryAddressIndexTwo[i])->nextHop[1].level << "." << (pathEntryAddressIndexTwo[i])->nextHop[1].position << "   ";
    fout << (pathEntryAddressIndexTwo[i])->nextHop[2].level << "." << (pathEntryAddressIndexTwo[i])->nextHop[2].position << "   ";
    fout << (pathEntryAddressIndexTwo[i])->nextHop[3].level << "." << (pathEntryAddressIndexTwo[i])->nextHop[3].position << "   ";
    fout << (pathEntryAddressIndexTwo[i])->nextHop[4].level << "." << (pathEntryAddressIndexTwo[i])->nextHop[4].position << "   ";
    fout << (pathEntryAddressIndexTwo[i])->destInterfaceIndex << "   " << (pathEntryAddressIndexTwo[i])->counter << "   " << (pathEntryAddressIndexTwo[i])->isAddRouteToKernel << "   " << counter++ << std::endl;
  }
  fout << std::endl;
  /*fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathEntryAddressIndexThree size is " << pathEntryAddressIndexThree.size() << " time is " << Simulator::Now().GetSeconds() << std::endl;*/
  fout << "Node" << m_Node.level << "." << m_Node.position << "'s pathEntryAddressIndexThree size is " << pathEntryAddressIndexThree.size() << " time is " << GetNow() << std::endl;
  counter=0;
  for (int i=0;i<pathEntryAddressIndexThree.size();i++)
  {
    fout << (pathEntryAddressIndexThree[i])->nextHop[0].level << "." << (pathEntryAddressIndexThree[i])->nextHop[0].position << "   ";
    fout << (pathEntryAddressIndexThree[i])->nextHop[1].level << "." << (pathEntryAddressIndexThree[i])->nextHop[1].position << "   ";
    fout << (pathEntryAddressIndexThree[i])->nextHop[2].level << "." << (pathEntryAddressIndexThree[i])->nextHop[2].position << "   ";
    fout << (pathEntryAddressIndexThree[i])->nextHop[3].level << "." << (pathEntryAddressIndexThree[i])->nextHop[3].position << "   ";
    fout << (pathEntryAddressIndexThree[i])->nextHop[4].level << "." << (pathEntryAddressIndexThree[i])->nextHop[4].position << "   ";
    fout << (pathEntryAddressIndexThree[i])->destInterfaceIndex << "   " << (pathEntryAddressIndexThree[i])->counter << "   " << (pathEntryAddressIndexThree[i])->isAddRouteToKernel << "   " << counter++ << std::endl;
  }
  fout << std::endl << std::endl;
  fout.close();
}

void 
Ipv4GlobalRouting::RecordTime()
{
  stringstream filename;
  //filename << "/home/guolab/ns-allinone-3.28/ns-3.28/output/TimeRecord-" << m_Node.level << "." << m_Node.position << ".txt";
  filename << "/var/log/CentrializedUpdateKernelTimeRecord.txt";//testbed
  ofstream fout(filename.str().c_str(),ios::app);
  fout.setf(ios::fixed, ios::floatfield);
  fout.precision(9);//设置保留的小数点位数
  /*fout << Simulator::Now().GetSeconds() << std::endl;*/
  struct timeval tv;
  gettimeofday(&tv,NULL);

  fout << tv.tv_sec+tv.tv_usec*0.000001 << std::endl;
  fout.close();
}

void 
Ipv4GlobalRouting::PrintPathEntry(struct pathtableentry * iter)
{
  /*int counter=0;
  std::cout << GetNow();
  std::cout << (*iter).nextHop[0].level << "." << (*iter).nextHop[0].position << "   ";
  std::cout << (*iter).nextHop[1].level << "." << (*iter).nextHop[1].position << "   ";
  std::cout << (*iter).nextHop[2].level << "." << (*iter).nextHop[2].position << "   ";
  std::cout << (*iter).nextHop[3].level << "." << (*iter).nextHop[3].position << "   ";
  std::cout << (*iter).nextHop[4].level << "." << (*iter).nextHop[4].position << "   ";
  std::cout << (*iter).destInterfaceIndex << "   " << (*iter).counter << "   " << (*iter).isAddRouteToKernel << "   " << counter++ << std::endl;*/
  ofstream Logfout("/var/log/center.log",ios::app);
  Logfout << GetNow();
  Logfout << (*iter).nextHop[0].level << "." << (*iter).nextHop[0].position << "   ";
  Logfout << (*iter).nextHop[1].level << "." << (*iter).nextHop[1].position << "   ";
  Logfout << (*iter).nextHop[2].level << "." << (*iter).nextHop[2].position << "   ";
  Logfout << (*iter).nextHop[3].level << "." << (*iter).nextHop[3].position << "   ";
  Logfout << (*iter).nextHop[4].level << "." << (*iter).nextHop[4].position << "   ";
  Logfout << "   counter:" << (*iter).counter << std::endl;//test
  Logfout.close();
}

string
Ipv4GlobalRouting::GetNow(){
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );
  /*cout << tt << endl;*/

  /*printf("[%d-%02d-%02d %02d:%02d:%02d]",
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

double
Ipv4GlobalRouting::GetSystemTime()
{
  //add by me
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC,&tv);
  return tv.tv_sec+tv.tv_nsec*0.000000001;
  //end
}

double
Ipv4GlobalRouting::Test(bool isMaster)////Master查找和修改链路表的时间测试
{
  ofstream timeFout("/var/log/Test.txt",ios::app);
  ofstream exampleFout("/var/log/Example.txt",ios::app);
  ofstream nodeFout("/var/log/NodeTime.txt",ios::app);
  timeFout.setf(ios::fixed, ios::floatfield);
  timeFout.precision(9);//设置保留的小数点位数
  exampleFout.setf(ios::fixed, ios::floatfield);
  exampleFout.precision(9);//设置保留的小数点位数
  nodeFout.setf(ios::fixed, ios::floatfield);
  nodeFout.precision(9);//设置保留的小数点位数

  struct timeval tv;
  double timeA,timeB,timeC,timeD,randNumTime;
  static double totalTime=0;

  //循环一万次
  int sandNum,interface,interfaceFlag,nextInterface=0;
  ident sourceident,destident;
  srand((unsigned)time(NULL));

  gettimeofday(&tv,NULL);
  timeA=tv.tv_sec+tv.tv_usec*0.000001;

  //产生一条随机的链路
  for (int i=0;i<10000;i++)
  {
    int sandNum=rand();
    sourceident.level=sandNum%3+1;//确定链路的一端位于第几层
    //确定这一端的位置和端口
    if (sourceident.level==1)
    {
      sourceident.position=sandNum%10000;
      interface=sandNum%4+2;
    }
    else if (sourceident.level==2)
    {
      sourceident.position=sandNum%400;
      interface=sandNum%104+2;
    }
    else if (sourceident.level==3)
    {
      sourceident.position=sandNum%16;
      interface=sandNum%100+2;
    }
    //选择一个随机端口
    destident=GetNextHopIdent(sourceident.level,sourceident.position,interface,&nextInterface);
    interfaceFlag=sandNum%2;
  }

  gettimeofday(&tv,NULL);
  timeB=tv.tv_sec+tv.tv_usec*0.000001;

  //timeFout << "产生随机数时间为：" << timeB << "-" << timeA << "=" << timeB-timeA << "s" << std::endl;
  randNumTime=timeB-timeA;

  //第二部分，调用ModifyMasterLinkTable
  gettimeofday(&tv,NULL);
  timeA=tv.tv_sec+tv.tv_usec*0.000001;

  bool isNeedToNotice=false;

  for (int i=0;i<10000;i++)
  {
    int sandNum=rand();
    sourceident.level=sandNum%3+1;//确定链路的一端位于第几层
    //确定这一端的位置和端口
    if (sourceident.level==1)
    {
      sourceident.position=sandNum%10000;
      interface=sandNum%4+2;
    }
    else if (sourceident.level==2)
    {
      sourceident.position=sandNum%400;
      interface=sandNum%104+2;
    }
    else if (sourceident.level==3)
    {
      sourceident.position=sandNum%16;
      interface=sandNum%100+2;
    }
    destident=GetNextHopIdent(sourceident.level,sourceident.position,interface,&nextInterface);
    //选择一个随机端口
    interfaceFlag=sandNum%2;
    //exampleFout << sourceident.level << "." << sourceident.position << "-" << destident.level << "." << destident.position << "  " << interfaceFlag << std::endl;
    if (isMaster)
    {
      //调用ModifyMasterLinkTable，测试Master查找和修改链路表的时间
      ModifyMasterLinkTable(sourceident,destident,&isNeedToNotice,interfaceFlag);
    }
    else
    {
      //调用LinkAssociatePath，测试Node查找映射表、查找和修改路径表的时间
      gettimeofday(&tv,NULL);
      timeC=tv.tv_sec+tv.tv_usec*0.000001;

      //LinkAssociatePath(sourceident,destident,interfaceFlag);

      gettimeofday(&tv,NULL);
      timeD=tv.tv_sec+tv.tv_usec*0.000001;

      nodeFout << timeD-timeC << std::endl;
      totalTime+=timeD-timeC;
    }
  }

  gettimeofday(&tv,NULL);
  timeB=tv.tv_sec+tv.tv_usec*0.000001;

  //timeFout << (timeB-timeA-randNumTime)/10000 << "s" << std::endl;
  if (isMaster)
  {
    return (timeB-timeA-randNumTime)/10000;
  }
  else
  {
    return totalTime/10000;
  }
}