#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

int main(int argc, char const *argv[])
{
	string strA,strB;
	double timeA,timeB,time;
	ifstream finA("/home/guolab/output/serverRecord.txt",ios::app);
	ofstream foutA("/home/guolab/output/udpInterval.txt",ios::app);
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(6);//设置保留的小数点位数
	getline(finA,strA);
	timeA=atof((strA.substr(5,strA.length()-5)).c_str());
	time=timeA;
	// foutA << timeA << endl;
	while (getline(finA,strA))
	{
		if (strA.length()==0) break;
		else 
		{
			timeB=atof((strA.substr(5,strA.length()-5)).c_str());
			// foutA << timeB << endl;
			foutA << timeB-time << "\t" << (timeB-timeA)*1000 << endl;
			timeA=timeB;
		}
	}
	return 0;
}