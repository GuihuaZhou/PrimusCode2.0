#include "defs.h"
#include <stddef.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <pthread.h>


// void MasterLinkThread(int i);
class Node;
class Ipv4GlobalRouting
{
public:
  struct linktableentry//master链路表条目
  {
    ident high;
    ident low;
    bool isNewLink;
    bool flag;
    double lastUpdateTime;//记录该条链路上一次更新的时间戳
    int linkUpdateTimer;//默认10ms内同一条链路的其他update信息不会被广播出去
    bool isStartThread;
    bool lastUpdateFlag;
    int temp;//判断是否快速恢复
  };

  struct routeweight
  {
    int weight[4];
  };

  struct pathtableentry//路径表条目
  {
    bool isAddRouteToKernel;
    ident nextHop[5];//此处应该重新考虑，万一拓扑有四层呢
    int destInterfaceIndex;
    int counter;
    struct routeweight * recordWeight;
    struct pathtableentry * next;
  };

  struct mappingtableentry//映射表条目
  {
    ident high;
    ident low;
    struct pathtableentry * address;
  };

  struct mappingentryindex//映射表索引
  {
    ident high;
    ident low;
    int index;
  };

  struct threadparam
  {
    Ipv4GlobalRouting* m_ipv4GlobalRouting;
    int index;
  };

  void SetNode(Node *node);
  void SetAttribute(int level,int position,int ToRNodes,int LeafNodes,int SpineNodes,int nPods,int Pod,int nMaster,int Links,int linkUpdateTimer,bool IsCenterRouting,bool randomEcmpRouting);//设置属性，充当conf
  void GeneratePath(ident high,ident low,std::vector<struct pathtableentry> *pathEntryTable);
  ident GetNextHopIdent(int level,int position,int interface,int * nextHopInterface);// 
  string GetGateway(struct pathtableentry * iter);
  string Allocate_ip(int level,int position,int interface);//根据端口信息计算IP

  void InitializeMasterLinkTable();//初始化Master的链路表
  void ModifyMasterLinkTable(ident sourceident,ident destident,bool * isNeedToNotice,bool interfaceFlag);//更新Master的链路表
  void ModifyMappingTable(struct pathtableentry * newPath,int type);
  static void* MasterLinkThread(void* threadparam);
  // void MasterLinkThread(int i);

  void InitializePathEntryTable();//初始化路径表
  void ModifyPathEntryTable(ident high,ident low,bool interfaceFlag);//更新路径表
  int ModifyCounterAndWeight(struct pathtableentry * iter,int increment);//修改路径的计数器和路由的权重，返回值表示是否需要修改路由
  void ModifyRoute(struct pathtableentry * iter,int increment);

  void ModifyNodeConfiguration(int newNPods);//更新configuration中的pod个数
  void ModifyLinkTableBySpineNode(int i,int j);//

  int IsNeedModifyRoute(int counter,int increment);//判断是否需要更新路由
  //void ModifyRoute(string destAddress,string gateway,string subMask,int cmd,int metric);//更新路由 
  void InsertPathEntryTable(struct pathtableentry * begin,struct pathtableentry * end,struct pathtableentry * newPathBegin,struct pathtableentry * newPathEnd);//将路径插入路径表
  // void AddRoute(struct pathtableentry * iter);//添加路由
  // void AddHostRouteTo(string destination,string nextaddress);
  void AddNodeProcess(ident high,ident low);//新加node的处理函数 
  
  void PrintMasterLinkTable();
  void PrintMappingTable();
  void PrintPathEntryTable();
  void RecordTime();
  void PrintPathEntry(struct pathtableentry * iter);

  string GetNow();
  double GetSystemTime();

  double Test(bool isMaster);////Master查找和修改链路表的时间测试，测试Node查找映射表、查找和修改路径表的时间

private:
  int m_ToRNodes;//第一层每个Pod结点的数量
  int m_LeafNodes;//第二层每个Pod结点的数量
  int m_SpineNodes;//第三层结点的数量
  int m_nPods;//Pod的数量
  int m_Pod;//是第几个Pod
  int m_nMaster;//master数量
  int m_Links=10;//Link总数
  int m_defaultTimer;//us
  bool m_IsCenterRouting=false;
  bool m_randomEcmpRouting = false;
  double checkTime=0.000000001;
  double modifyTime=0.000000001;
  ident m_Node;
  std::vector<struct linktableentry> masterLinkTable;
  std::vector<struct mappingtableentry> mappingEntryTable;
  struct pathtableentry * headNode= (struct pathtableentry *)malloc(sizeof(struct pathtableentry));//头节点中的counter用作链表长度计数器
  struct pathtableentry * tailNode;
  // 路径表索引
  std::vector<struct pathtableentry *> pathEntryAddressIndexOne;
  std::vector<struct pathtableentry *> pathEntryAddressIndexTwo;
  std::vector<struct pathtableentry *> pathEntryAddressIndexThree;
  // 映射表索引
  std::vector<struct mappingentryindex> mappingEntryIndex;
  // 路由权重
  // std::vector<struct routeweight *> routeWeight;
  double firstRecordTime;
  double secondRecordTime;
  static Node *m_node;//与Ipv4-global-routing聚合的Node对象

  pthread_t masterLink_thread;
};