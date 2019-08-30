#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

int main(int argc, char const *argv[])
{
	string str;
	int lengthA,lengthB,lengthC;
	vector<double> dataSetA;
	vector<double> dataSetB;
	vector<double> dataSetC;
	ifstream finA("/home/guolab/output/BGPOutput.txt",ios::app);
	ifstream finB("/home/guolab/output/CenterOutput.txt",ios::app);
	ifstream finC("/home/guolab/output/NodeTime.txt",ios::app);
	//ifstream finC("/home/guolab/output/NodeTime.txt",ios::app);
	//ofstream fout("/home/guolab/output/Data.csv",ios::app);
	//ofstream fout("/home/guolab/output/NodeTimeData.csv",ios::app);
	ofstream fout("/home/guolab/output/DataTest.csv",ios::app);

	fout.setf(ios::fixed, ios::floatfield);
    fout.precision(3);//设置保留的小数点位数
	while (getline(finA,str))
	{
		//fout << str << ",";
        dataSetA.push_back(atof(str.c_str()));
	}
	lengthA=dataSetA.size();
	sort(dataSetA.begin(),dataSetA.end());
	//fout << endl;
	while (getline(finB,str))
	{
		//fout << str << ",";
		dataSetB.push_back(atof(str.c_str()));
	}
	lengthB=dataSetB.size();
	sort(dataSetB.begin(),dataSetB.end());

	while (getline(finC,str))
	{
		//fout << str << ",";
		dataSetC.push_back(atof(str.c_str()));
	}
	lengthC=dataSetC.size();
	sort(dataSetC.begin(),dataSetC.end());

	for (int i=0;i<99;i++)
	{
		fout << dataSetA[i*150] << "," << 0.01*(i+1) << std::endl;
		if ((i+2)*150>=dataSetA.size())
		{
			fout << dataSetA[lengthA-1] << "," << 1 << std::endl << std::endl;
			break;
		}
	}
	for (int i=0;i<99;i++)
	{
		fout << dataSetB[i*150] << "," << 0.01*(i+1) << std::endl;
		if ((i+2)*150>=dataSetB.size())
		{
			fout << dataSetB[lengthB-1] << "," << 1 << std::endl << std::endl;
			break;
		}
	}
	fout.setf(ios::fixed, ios::floatfield);
    fout.precision(9);//设置保留的小数点位数
	for (int i=0;i<99;i++)
	{
		fout << dataSetC[i*150] << "," << 0.01*(i+1) << std::endl;
		if ((i+2)*150>=dataSetC.size())
		{
			fout << dataSetC[lengthA-1] << "," << 1 << std::endl << std::endl;
			break;
		}
	}
	/*for (int i=0;i<dataSetA.size();i++)
	{
		fout << dataSetA[i] << std::endl;
	}*/

	/*while (getline(finC,str))
	{
		fout << str << ",";
	}*/
	/*close(finA);
	close(finB);
	close(fout);*/
	return 0;
}
