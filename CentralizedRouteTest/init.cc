#include <iostream>
#include "node.h"

#define MAX_ROUTE 1000
using namespace std;

string masterAddress = "10.0.1.67";
int masterPort = 6666;
int level = 2;
int position = 1;

int nPods=2;//Pod总数
int addPods=0;
int SpineNodes=16;//顶层交换机数
int LeafNodes=4;//单个pod里2层交换机数
int ToRNodes=1;//每个pod里的ToR交换机数
int AllNodes=26;//节点总数
int Links=832;//链路总数
int nMaster=1;
int pod=0;
int defaultMasterTimer=10000;//us
int defaultKeepaliveTimer=6;
/*NodeContainer SpineNode,LeafNode,ToRNode,MasterNode;*/

int main(int argc,char **argv)
{
  ofstream Logfout("/home/guolab/output/center.log",ios::app);
  ifstream fin("/usr/local/etc/center.conf",ios::app);
  string config;
  int begin,end;

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  masterAddress=config.substr(begin,end);

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  masterPort=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  level=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  position=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  defaultMasterTimer=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  defaultKeepaliveTimer=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  ToRNodes=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  LeafNodes=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  SpineNodes=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  nPods=atoi(config.substr(begin,end).c_str());

  // test
  Logfout << "level:" << level << endl << "position:" << position << endl << "masterAddress:" << masterAddress << endl << "masterPort:" << masterPort << endl << "defaultMasterTimer:" << defaultMasterTimer << endl << "defaultKeepaliveTimer:" << defaultKeepaliveTimer << endl << endl;
  // end

  int Links=(SpineNodes+LeafNodes*ToRNodes)*nPods;
  if (level==3 || level==0) pod=0;
  else if (level==2) pod=position/LeafNodes+1;
  else if (level==1) pod=position/ToRNodes+1;
  Node *node = new Node(level,position,ToRNodes,LeafNodes,SpineNodes,nPods,pod,nMaster,Links,defaultMasterTimer,defaultKeepaliveTimer,true,true);
  node->SetRole(masterAddress,masterPort);
  return 0;
}