#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

void functionA(double interval)//单个时间片内的吞吐率
{
	/*boss zhou*/
	string str;
	double timeA,timeB,timeC,throughputRatio=0;
	long long readBytes=0,temp=0;
	ifstream finA("/home/guolab/output/serverRecord.txt",ios::app);
	ofstream foutA("/home/guolab/output/throughputRatio.txt",ios::trunc);
	getline(finA,str);
	readBytes+=atoi((str.substr(0,str.find('\t'))).c_str());
	timeA=atof((str.substr(str.find('\t')+1,str.length()-str.find('\t'))).c_str());	
	timeC=timeA;
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(6);//设置保留的小数点位数
	while (getline(finA,str))
	{
		readBytes+=atoi((str.substr(0,str.find('\t'))).c_str());
		timeB=atof((str.substr(str.find('\t')+1,str.length()-str.find('\t'))).c_str());
		if (timeB-timeA>interval)//interval ms计算一次吞吐率
		{
			throughputRatio=readBytes*8/((timeB-timeA)*1000000);
			if (throughputRatio>1024) foutA << timeB-timeC << "\t" << 1024.000000 << endl;
			else foutA << timeB-timeC << "\t" << throughputRatio << endl;
			timeA=timeB;
			readBytes=0;
		}
	}
}

void functionB(double interval)//每隔一定的时间求一次总吞吐率
{
	/*boss wang */
	string str;
	double timeA,timeB,timeC,throughputRatio=0;
	long long readBytes=0,temp=0;
	ifstream finA("/home/guolab/output/serverRecord.txt",ios::app);
	ofstream foutA("/home/guolab/output/throughputRatio.txt",ios::trunc);
	getline(finA,str);
	readBytes+=atoi((str.substr(0,str.find('\t'))).c_str());
	timeA=atof((str.substr(str.find('\t')+1,str.length()-str.find('\t'))).c_str());	
	timeC=timeA;
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(6);//设置保留的小数点位数
	while (getline(finA,str))
	{
		readBytes+=atoi((str.substr(0,str.find('\t'))).c_str());
		timeB=atof((str.substr(str.find('\t')+1,str.length()-str.find('\t'))).c_str());
		if (timeB-timeA>interval)//interval ms计算一次吞吐率
		{
			throughputRatio=readBytes*8/((timeB-timeC)*1000000);
			foutA << timeB-timeC << "\t" << throughputRatio << endl;
			timeA=timeB;
		}
	}
}

int main(int argc, char const *argv[])
{
	// string str;
	// double timeA,timeB,timeC;
	// vector<double> dataSetA;
	// ifstream finA("/home/guolab/output/udpThroughput.txt",ios::app);
	// ofstream foutA("/home/guolab/output/udpThroughputAnalysis.txt",ios::trunc);
	// getline(finA,str);
	// timeA=atof((str.substr(0,str.find(' '))).c_str());
	// while (getline(finA,str))
	// {
	// 	timeB=atof((str.substr(0,str.find(' '))).c_str());
	// 	timeC=timeB-timeA;
	// 	timeA=timeB;
	// 	dataSetA.push_back(timeC);
	// }
	// sort(dataSetA.begin(),dataSetA.end());
	// for (int i=0;i<dataSetA.size();i++)
	// {
	// 	foutA << dataSetA[i] << endl;
	// }
	// string str;
	// double timeA,timeB,timeC,throughputRatio=0,interval;
	// interval=atof(argv[1]);
	// long long readBytes=0,temp=0;
	// vector<double> dataSetA;
	// ifstream finA("/home/guolab/output/server-record.txt",ios::app);
	// ofstream foutA("/home/guolab/output/throughputRatio.txt",ios::trunc);
	// ofstream foutB("/home/guolab/output/udpThroughputAnalysis.txt",ios::trunc);
	// getline(finA,str);
	// readBytes+=atoi((str.substr(0,str.find('\t'))).c_str());
	// timeA=atof((str.substr(str.find('\t')+1,str.length()-str.find('\t'))).c_str());	
	// timeC=timeA;
	// foutA.setf(ios::fixed, ios::floatfield);
	// foutA.precision(6);//设置保留的小数点位数
	// foutB.setf(ios::fixed, ios::floatfield);
	// foutB.precision(6);//设置保留的小数点位数
	// while (getline(finA,str))
	// {
	// 	readBytes+=atoi((str.substr(0,str.find('\t'))).c_str());
	// 	timeB=atof((str.substr(str.find('\t')+1,str.length()-str.find('\t'))).c_str());
	// 	if (timeB-timeA>interval)//interval ms计算一次吞吐率
	// 	{
	// 		throughputRatio=readBytes*8/((timeB-timeC)*1000000);
	// 		dataSetA.push_back(throughputRatio);
	// 		foutB << timeB-timeC << endl;
	// 		timeA=timeB;
	// 	}
	// }
	// for (int i=0;i<dataSetA.size();i++)
	// {
	// 	foutA << dataSetA[i] << endl;
	// }

	if (atoi(argv[1])==1) functionA(atof(argv[2]));
	else if (atoi(argv[1])==2) functionB(atof(argv[2]));
	return 0;
}