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
	double time;
	ifstream finA("/home/guolab/output/serverRecord.txt",ios::app);
	ofstream foutA("/home/guolab/output/transmissionTime.txt",ios::app);
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(1);//设置保留的小数点位数
	for (int k = 0; k < 6; k++) getline(finA,strA);
	while (getline(finA,strA))
	{
		if (strA.length()==0) break;
		else strB=strA;
	}
	time=atof((strB.substr(strB.find('-')+1,4)).c_str());
	// foutA << "Transmission time is " << (time-2)*0.5 << "s"<< endl;
	foutA << time << endl;
	return 0;
}