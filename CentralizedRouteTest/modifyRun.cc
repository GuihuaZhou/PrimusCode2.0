#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

struct ident
{
	int level;
	int position;
};

int main(int argc, char const *argv[])
{
    struct ident m_ident;
	//获得地址后8位
	stringstream suffix;
	char buff[3];
	FILE * fpA;
	if (fpA=popen("ifconfig eth0 | grep \"inet addr:\" | awk '{print $2}' | cut -c 13-","r"))//截取eth0 ip的后两位
	{
		fgets(buff,sizeof(buff),fpA);
	}
	suffix << buff;
	int ipSuffix=atoi(suffix.str().c_str());

	if (ipSuffix>=60 && ipSuffix<=75)//spine
	{
		m_ident.level=3;
    	m_ident.position=ipSuffix-60;
	}
	else if (ipSuffix>75 && ipSuffix<84)//leaf
	{
		m_ident.level=2;
    	m_ident.position=ipSuffix-76;
	}
	else if (ipSuffix==84)//tor
	{
		m_ident.level=1;
    	m_ident.position=0;
	}
	else if (ipSuffix==85)//tor
	{
		m_ident.level=1;
    	m_ident.position=2;
	}
	else if (ipSuffix==96)
	{
		m_ident.level=1;
    	m_ident.position=1;
	}
	else if (ipSuffix==97)
	{
		m_ident.level=1;
    	m_ident.position=3;
	}

	string masterAddress(argv[1]);
    int masterPort=atoi(argv[2]);
    int m_ToRNodes=atoi(argv[3]);
    int m_LeafNodes=atoi(argv[4]);
    int m_SpineNodes=atoi(argv[5]);
    int m_nPods=atoi(argv[6]);
    int m_defaultMasterTimer=atoi(argv[7]);
    int m_defaultKeepaliveTimer=atoi(argv[8]);

    ofstream fout("/usr/local/etc/center.conf",ios::app);
    fout << "masterAddress:" << masterAddress << endl;
    fout << "masterPort:" << masterPort << endl;
    fout << "level:" << m_ident.level << endl;
    fout << "position:" << m_ident.position << endl;
    fout << "defaultMasterTimer(us):" << m_defaultMasterTimer << endl;
    fout << "defaultKeepaliveTimer(s):" << m_defaultKeepaliveTimer << endl;
    fout << "ToRNodes:" << m_ToRNodes << endl;
    fout << "LeafNodes:" << m_LeafNodes << endl;
    fout << "SpineNodes:" << m_SpineNodes << endl;
    fout << "nPods:" << m_nPods << endl;

	// stringstream command;
	// command << "/home/guolab/CentralizedRouteTest/CentralizedRoute " << masterAddress << " " << masterPort << " " << m_ident.level << " " << m_ident.position << " " << m_ToRNodes << " " << m_LeafNodes << " " << m_SpineNodes << " " << m_nPods << " " << m_defaultMasterTimer << " " <<m_defaultKeepaliveTimer;
	// // cout << command.str() << endl;
	// sleep(1);
	// system(command.str().c_str());

	// struct ident m_ident;
	// //获得地址后8位
	// stringstream suffix;
	// char buff[3];
	// FILE * fpA;
	// if (fpA=popen("ifconfig eth0 | grep \"inet addr:\" | awk '{print $2}' | cut -c 17-","r"))//截取eth0 ip的后两位
	// {
	// 	fgets(buff,sizeof(buff),fpA);
	// }
	// suffix << buff;
	// int ipSuffix=atoi(suffix.str().c_str());

	// if (ipSuffix==1)//spine
	// {
	// 	m_ident.level=3;
 //    	m_ident.position=10;
	// }
	// else if (ipSuffix==2)//leaf
	// {
	// 	m_ident.level=3;
 //    	m_ident.position=11;
	// }
	// else if (ipSuffix==3)//leaf
	// {
	// 	m_ident.level=3;
 //    	m_ident.position=14;
	// }
	// else if (ipSuffix==4)//leaf
	// {
	// 	m_ident.level=3;
 //    	m_ident.position=15;
	// }
	// else if (ipSuffix==5)//tor
	// {
	// 	m_ident.level=2;
 //    	m_ident.position=2;
	// }
	// else if (ipSuffix==6)//tor
	// {
	// 	m_ident.level=2;
 //    	m_ident.position=3;
	// }
	// else if (ipSuffix==7)//tor
	// {
	// 	m_ident.level=2;
 //    	m_ident.position=6;
	// }
	// else if (ipSuffix==8)//tor
	// {
	// 	m_ident.level=2;
 //    	m_ident.position=7;
	// }
	// else if (ipSuffix==9)//tor
	// {
	// 	m_ident.level=1;
 //    	m_ident.position=0;
	// }
	// else if (ipSuffix==10)//tor
	// {
	// 	m_ident.level=1;
 //    	m_ident.position=1;
	// }
	// else if (ipSuffix==17)//tor
	// {
	// 	m_ident.level=1;
 //    	m_ident.position=2;
	// }
	// else if (ipSuffix==18)//tor
	// {
	// 	m_ident.level=1;
 //    	m_ident.position=3;
	// }

	// // const char* Address=argv[1];
	// string masterAddress(argv[1]);
 //    int masterPort=atoi(argv[2]);
 //    int m_ToRNodes=atoi(argv[3]);
 //    int m_LeafNodes=atoi(argv[4]);
 //    int m_SpineNodes=atoi(argv[5]);
 //    int m_nPods=atoi(argv[6]);
 //    int m_defaultTimer=atoi(argv[7]);

	// stringstream command;
	// command << "/home/guolab/CentralizedRouteTest/CentralizedRoute " << masterAddress << " " << masterPort << " " << m_ident.level << " " << m_ident.position << " " << m_ToRNodes << " " << m_LeafNodes << " " << m_SpineNodes << " " << m_nPods << " " << m_defaultTimer;
	// sleep(1);
	// system(command.str().c_str());
	return 0;
}