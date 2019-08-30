#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

int main(int argc, char const *argv[])
{
	vector<vector<double> > convergenceTimeSet(20);
    for (int i=0;i<20;i++) convergenceTimeSet[i].resize(20,0); 
    vector<int> packetNumSet;

	string str,strA,strB;
	stringstream fileA,fileB;
	int modifyEthTimes,hostNum;//记录更改eth的次数和host数
	hostNum=0;
	ifstream fin("/home/guolab/host.txt",ios::app);
	ofstream foutA("/home/guolab/output/convergenceTimeTotalAnalysis.txt",ios::app);//收敛时间存储文件
	ofstream foutB("/home/guolab/output/packetRecord.txt",ios::app);//通信数据包个数存储文件
	while (getline(fin,str))
	{
		fileA.str("");
		fileB.str("");
		fileA << "/home/guolab/output/" << str.substr(5,9) << "/convergenceTime.txt";
		fileB << "/home/guolab/output/" << str.substr(5,9) << "/packetRecord.txt";
		ifstream finA(fileA.str().c_str(),ios::app);
		ifstream finB(fileB.str().c_str(),ios::app);
		modifyEthTimes=0;
		foutA << "root@" << str.substr(5,9) << ":\t";
		foutB << "root@" << str.substr(5,9) << ":\t";
        while (getline(finA,strA))
        {
        	convergenceTimeSet[hostNum][modifyEthTimes]=atof(strA.c_str());
        	foutA << convergenceTimeSet[hostNum][modifyEthTimes++] << "\t";
        } 
        hostNum++;
        foutA << "\n";

        while (getline(finB,strB))
        {
            packetNumSet.push_back(atoi(strB.c_str()));
            foutB << atoi(strB.c_str()) << "\n";
        }
	}

	vector<double> convergenceTime;
	double maxValue;
	for (int j=0;j<modifyEthTimes;j++)//求最大得收敛时间
	{
		maxValue=0;
		for (int i=0;i<hostNum;i++) if (convergenceTimeSet[i][j]>maxValue) maxValue=convergenceTimeSet[i][j];
		convergenceTime.push_back(maxValue);
	}

    maxValue=0;
	for (int i=0;i<hostNum;i++)
	{
		if (packetNumSet[i]>maxValue) maxValue=packetNumSet[i];
	}

	//test
	/*cout << "hostNum:" << hostNum << " modifyEthTimes:" << modifyEthTimes << endl;
	cout << "convergenceTime:" << endl;*/
	foutA << endl << "TotalAnalysis:" << "\t";
	for (int i=0;i<convergenceTime.size();i++) foutA << convergenceTime[i] << "\t";
	foutA << endl << endl;
    foutB << endl << "PacketNum:" << "\t" << maxValue << endl << endl;
	return 0;
}