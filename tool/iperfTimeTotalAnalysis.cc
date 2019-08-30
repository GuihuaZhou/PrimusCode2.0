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
	double totalTransmissionTime=0;
	int time=0;
	ifstream finA("/home/guolab/output/transmissionTime.txt",ios::app);
	ofstream foutA("/home/guolab/output/transmissionTime.txt",ios::app);
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(6);//设置保留的小数点位数
	for (int k = 0; k < 6; k++) getline(finA,str);
	while (getline(finA,str))
	{
		if (str[0]=='T') totalTransmissionTime+=atof((str.substr(21,7)).c_str());
		time++;
	}
	foutA << "Average transmission time is " << totalTransmissionTime/time << "s" << endl << endl;
	return 0;
}