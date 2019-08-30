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
	double lastThroughtputRatio=0;
	int len=0,i;
	ifstream finA("/home/guolab/output/serverRecord.txt",ios::app);
	ofstream foutA("/home/guolab/output/throughputRatio.txt",ios::app);
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(6);//设置保留的小数点位数
	for (int k = 0; k < 6; k++) getline(finA,str);
	getline(finA,str);
	len=0;
	for (i=str.length()-11;str[i]!=' ';i--) len++;
	i+=1;
	lastThroughtputRatio=atof((str.substr(i,len)).c_str());
	while (getline(finA,str))
	{
		foutA << lastThroughtputRatio << endl;
		len=0;
		for (i=str.length()-11;str[i]!=' ';i--) len++;
		i+=1;
		// foutA << time*0.5 << "\t" << atof((str.substr(i,len)).c_str()) << endl;
		// foutA << atof((str.substr(i,len)).c_str()) << endl;
		lastThroughtputRatio=atof((str.substr(i,len)).c_str());
	}
	// foutA << "average throughputRatio is " << lastThroughtputRatio << "Mbps" << endl << endl;
	foutA << endl;
	return 0;
}