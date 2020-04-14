#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

int main(int argc, char const *argv[])
{
	/*tcp timeout CDF*/
	vector<double> dataSetA;
	ifstream finA(argv[1],ios::app);

  string str;
	while (getline(finA,str))
	{
    dataSetA.push_back(atof(str.c_str()));
	}
	sort(dataSetA.begin(),dataSetA.end());

	ofstream foutA(argv[2],ios::trunc);
	foutA.setf(ios::fixed, ios::floatfield);
  foutA.precision(6);//设置保留的小数点位数

	int stamp=atoi(argv[3]);// 戳
	double precentage=0;

	for (double i=0;i<dataSetA.size();i++)
	{
		if (dataSetA[i]<=stamp);
		else 
		{
			precentage=i/dataSetA.size();// 获得百分比
			stamp*=10;
			if (precentage==1) break;
			foutA << stamp << "\t" << precentage << endl;
		}
	}
	foutA << stamp << "\t" << "1" << endl;

  finA.close();
	foutA.close();

	/*ssh stamp CDF*/
	// vector<double> dataSetA;
	// ifstream finA(argv[1],ios::app);

	// string str;
	// double stampA=0,stampB=0,stampC=0;
	// while (getline(finA,str))
	// {
	// 	if (str[0]=='-') continue;
	// 	stampA=atof(str.c_str());
	// 	str="";
	// 	if (getline(finA,str))
	// 	{
	// 		if (str[0]=='-') continue;
	// 		stampB=atof(str.c_str());
	// 		dataSetA.push_back((stampB-stampA)*1000);
	// 	}
	// 	else break;
	// 	str="";
	// 	if (getline(finA,str))
	// 	{
	// 		if (str[0]=='-') continue;
	// 		stampC=atof(str.c_str());
	// 		dataSetA.push_back((stampC-stampB)*1000);
	// 	}
	// 	else break;
	// 	str="";
	// }
	// sort(dataSetA.begin(),dataSetA.end());

	// ofstream foutA(argv[2],ios::trunc);
	// foutA.setf(ios::fixed, ios::floatfield);
 //  foutA.precision(6);//设置保留的小数点位数

	// int stamp=0;// 戳，单位毫秒
	// double precentage=0;

	// for (double i=0;i<dataSetA.size();i++)
	// {
	// 	if (dataSetA[i]<=stamp);
	// 	else 
	// 	{
	// 		precentage=i/dataSetA.size();// 获得百分比
	// 		stamp+=100;
	// 		if (precentage==1) break;
	//    foutA << stamp << "\t" << precentage << endl;
	// 	}
	// }
	// foutA << stamp << "\t" << "1" << endl;

	return 0;
}