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
	/*CDF*/
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
			foutA << stamp << "\t" << precentage << endl;
			if (precentage==1) break;
			stamp*=10;
		}
	}
	foutA << stamp << "\t" << "1" << endl;

  finA.close();
	foutA.close();

	return 0;
}