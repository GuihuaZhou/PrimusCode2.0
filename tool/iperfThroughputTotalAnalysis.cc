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
	double throughputRatioA=0,throughputRatioB=0,timeA=0,timeB=0,throughputRatio=0,time=0;
	int len=0,i;
	ifstream finA("/home/guolab/output/throughputRatio.txt",ios::app);
	// ifstream finB("/home/guolab/output/throughputRatioB.txt",ios::app);
	ofstream foutA("/home/guolab/output/throughputAvarageRatio.txt",ios::app);
	foutA.setf(ios::fixed, ios::floatfield);
	foutA.precision(6);//设置保留的小数点位数
	// while (getline(finA,str))
	// {
	// 	if (str[0]==' ') continue;
	// 	len=0;
	// 	for (i=str.length()-11;str[i]!=' ';i--) len++;
	// 	i+=1;
	// 	throughputRatioA+=atof((str.substr(i,len)).c_str());
	// 	// foutA << time*0.5 << "\t" << atof((str.substr(i,len)).c_str()) << endl;
	// 	timeA++;
	// }
	// while (getline(finB,str))
	// {
	// 	if (str[0]==' ') continue;
	// 	len=0;
	// 	for (i=str.length()-11;str[i]!=' ';i--) len++;
	// 	i+=1;
	// 	throughputRatioB+=atof((str.substr(i,len)).c_str());
	// 	// foutA << time*0.5 << "\t" << atof((str.substr(i,len)).c_str()) << endl;
	// 	timeB++;
	// }
	// while (1)
	// {
	// 	if (!getline(finA,strA)) break;
	// 	if (!getline(finB,strB)) break;
	// 	if (strA.length()==0) continue;
	// 	if (strB.length()==0) continue;
	// 	// foutA << strA << " and " << strB << endl;
	// 	// len=0;
	// 	// for (i=strA.length()-11;strA[i]!=' ';i--) len++;
	// 	// i+=1;
	// 	// throughputRatio+=atof((strA.substr(i,len)).c_str());
	// 	// len=0;
	// 	// for (i=strB.length()-11;strB[i]!=' ';i--) len++;
	// 	// i+=1;
	// 	// throughputRatio+=atof((strB.substr(i,len)).c_str());
	// 	throughputRatio+=atof((strA.substr(strA.find('\t')+1,strA.length()-strA.find('\t'))).c_str());
	// 	throughputRatio+=atof((strB.substr(strB.find('\t')+1,strB.length()-strB.find('\t'))).c_str());
	// 	time++;
	// }
	while (getline(finA,strA))
	{
		if (strA[0]=='a')
		{
			throughputRatioA+=atof((strA.substr(27,10)).c_str());
			// foutA << atof((strA.substr(27,10)).c_str()) << endl;
			time++;
		}
	}
	foutA << throughputRatioA/time << endl;
	return 0;
}