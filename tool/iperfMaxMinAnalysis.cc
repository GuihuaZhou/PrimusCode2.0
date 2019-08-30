#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <math.h>

using namespace std;

int main(int argc, char const *argv[])
{
	string strA;
	double temp=0,couter=0;
	int index=0;
	vector<vector<double> > array(5000);
	for (int m=0;m<5000;m++)
	{
		array[m].resize(1200);
	}
	for (int m=0;m<5000;m++)
	{
		for (int n=0;n<1200;n++)
		{
			array[m][n]=-1;
		}
	}
	double interval=atof(argv[1]);
	double summary=0,average=0,standardDeviation=0;
	int i=0,len=0;
	ifstream finA("/home/guolab/output/serverRecord.txt",ios::app);
	ofstream foutA("/home/guolab/output/MaxMinAnalysis.txt",ios::app);
	ofstream foutB("/home/guolab/output/temp.txt",ios::app);
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(3);//设置保留的小数点位数
	while (getline(finA,strA)) if (strA[2]=='I') break;
	while (getline(finA,strA))
	{
		if (strA.length()==0) break;
		if (strA[2]=='I') break;
		if (strA[6]=='l') continue;
		if (strA[1]=='S')
		{
			getline(finA,strA);
			continue;
		} 
		len=0;
		int location=strA.find('/');
		for (i=location-7;i>0;i--) 
		{
			if (strA[i]==' ') break;
			len++;
		}
		temp=atof((strA.substr(++i,len)).c_str());
		if (strA[location-5]=='K') temp*=0.001;
		else if (strA[location-5]==' ') temp*=0.000001;
		index=(int)((atof((strA.substr(strA.find('-')+1,5)).c_str()))/interval-1);
		if (atof((strA.substr(6,4)).c_str())==0 && index>40) continue;
		// foutB << "index is " << index << " and temp is " << temp << endl;
		for (int n=0;n<1200;n++)
		{
			if (array[index][n]==-1) 
			{
				array[index][n]=temp;
				// foutB << "temp is " << array[index][n] << endl;
				break;
			}
		}
	}
	// sum
	if (strA[2]=='I')
	{
		do 
		{
			getline(finA,strA);
			getline(finA,strA);
			if (strA[0]=='S') break;
			len=0;
			int location=strA.find('/');
			for (i=location-7;i>0;i--) 
			{
				if (strA[i]==' ') break;
				len++;
			}
			temp=atof((strA.substr(++i,len)).c_str());
			if (strA[location-5]=='K') temp*=0.001;
			else if (strA[location-5]==' ') temp*=0.000001;
			foutB << strA.substr(1,3) << '\t' << temp << endl;
		}
		while(1);
	}
	for (int i=0;i<5000;i++)
	{
		if (array[i][0]==-1) break;
		summary=0;
		couter=0;
		for (int j=0;j<1200;j++)
		{
			if (array[i][j]==-1) break;
			summary+=array[i][j];
			// foutB << "throughtput is " << array[i][j] << "\t";
			couter++;
		}
		// foutA << (i+1)*interval << "\t" << summary << endl;
		average=summary/couter;
		summary=0;
		for (int j=0;j<1200;j++)
		{
			if (array[i][j]==-1) break;
			summary+=(array[i][j]-average)*(array[i][j]-average);
		}
		foutA << (i+1)*interval << "\t" << sqrt(summary/couter)/average << endl;
	}
	return 0;
}