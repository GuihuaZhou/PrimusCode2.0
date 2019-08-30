#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

int main()
{
	ifstream finA("/home/guolab/output/interfacesChangeTimeRecord.txt");
	string str;
	vector<double> interfaceTime;
    ofstream foutTest("/home/guolab/output/test.txt",ios::app);//test
    foutTest.setf(ios::fixed, ios::floatfield);
    foutTest.precision(6);//设置保留的小数点位数
    foutTest << "interfaceTime:" << endl;//test
	while (1)//读取出所有链路变化的时间
	{
		getline(finA,str);
		if (str[0]=='\0') break;
        interfaceTime.push_back(atof(str.c_str()));
        foutTest << atof(str.c_str()) << endl;//test
	}

    ifstream finB("/home/guolab/output/updateKernelTimeRecord.txt");
	vector<double> convergenceTime;
	double timeA,timeB;
	getline(finB,str);
	timeA=atof(str.c_str());
    foutTest << "\nupdateKernelTime:" << endl;
	while (1)//读取出所有内核路由表更新的时间
	{
		getline(finB,str);
		if (str[0]=='\0') break;
		timeB=atof(str.c_str());
		if (timeB-timeA>=10)
        {
            convergenceTime.push_back(timeA);
            foutTest << timeA << endl;//test
        } 
		timeA=timeB;
	}
    //test
    foutTest.close();

    ofstream fout("/home/guolab/output/convergenceTime.txt",ios::app);
    fout.setf(ios::fixed, ios::floatfield);
    fout.precision(6);//设置保留的小数点位数
    if (convergenceTime.size()==1)
    {
        for (int i=0;i<interfaceTime.size();i++) fout << "0" << "\n";//没有更新路由表
    } 
    else
    {
    	for (int i=0;i<interfaceTime.size();i++)//遍历所有的链路变化时间
    	{
    		bool flag=false;//判断每一个链路变化时间是否有对应的收敛时间
            //在两次链路变化时间的间隔中，选择最大的收敛时间
    		for (int j=0;j<convergenceTime.size();j++)
    		{
    			if (convergenceTime[j]>interfaceTime[i])//在convergenceTime中找到一个比interfaceTime大的时间戳
    			{
    				//printf("i=%d j=%d\n",i,j);
                    if (j==convergenceTime.size()-1)//如果convergenceTime的时间戳是最后一个，则直接是为该interfaceTime的收敛时间戳
                    {
                        fout << convergenceTime[j]-interfaceTime[i] << "\n";
                        //printf("%f ",convergenceTime[j]-interfaceTime[i]);
                        flag=true;
                        break;
                    } 
                    else if (j<convergenceTime.size()-1)//如果不是最后一个，则继续寻找更大的convergenceTime时间戳，但要求比下一个interfaceTime要小   
                    {
                        double lastConvergenceTime=convergenceTime[j];
                        if (i<interfaceTime.size()-1)
                        {
                            while (convergenceTime[j+1]<interfaceTime[i+1] && (j+1)<=convergenceTime.size()-1 && (i+1)<=interfaceTime.size()-1) lastConvergenceTime=convergenceTime[++j];
                            fout << lastConvergenceTime-interfaceTime[i] << "\n";
                            flag=true;
                            break;
                        }
                        else if (i==interfaceTime.size()-1)
                        {
                            fout << convergenceTime[convergenceTime.size()-1]-interfaceTime[i] << "\n";
                            flag=true;
                            break;
                        }
                    }
    			}
    		}
    		if (flag==false) fout << "0" << "\n";//若针对此次链路变化没有不要更新路由，则将对应位置的convergenceTime置为0
    		//printf("\n");
    	}
    }
    fout.close();
    //printf("\n");

    //test
    /*ofstream fout("/10.0.1.67/sambashare/output/totalConvergenceTime.txt",ios::app);
    fout << "hello!" << endl;*/

	return 0;
}