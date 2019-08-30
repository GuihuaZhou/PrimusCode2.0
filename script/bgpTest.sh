#!/bin/bash
# 准备实验环境，确保不会启动重复的进程
pssh -i -h /home/guolab/host.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/PCHost.txt "killall client;"
pssh -i -H "root@10.0.8.86" "killall master;"
# 确保所有网卡都是正常的
# Spine
ssh root@10.0.8.60 "ifup eth1;ifup eth2;"
ssh root@10.0.8.61 "ifup eth1;ifup eth2;"
ssh root@10.0.8.62 "ifup eth1;ifup eth2;"
ssh root@10.0.8.63 "ifup eth1;ifup eth2;"
ssh root@10.0.8.64 "ifup eth1;ifup eth2;"
ssh root@10.0.8.65 "ifup eth1;ifup eth2;"
ssh root@10.0.8.66 "ifup eth1;ifup eth2;"
ssh root@10.0.8.67 "ifup eth1;ifup eth2;"
# Leaf
ssh root@10.0.8.76 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;"
ssh root@10.0.8.77 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;"

# 1表示8个SpineNode左侧的链路状态均为正常
SpineNode=(1 1 1 1 1 1 1 1)
# 表示8个SpineNode左侧的链路中可用的条数，避免为0
Available=8
# 8条链路两端的网卡地址
LeafNodeIP=("10.0.8.76" "10.0.8.77")
SpineNodeIP=("10.0.8.60" "10.0.8.61" "10.0.8.62" "10.0.8.63" "10.0.8.64" "10.0.8.65" "10.0.8.66" "10.0.8.67")
Eth=(eth1 eth2 eth3 eth4)

random=7
index=7
temp=7

function ExecuteCommand()
{
    exe_commandA=''
    exe_commandB=''
    if [ ${SpineNode[$1]} -eq 1 ]
    then
        SpineNode[$1]=0
        Available=$(($Available-1))
        exe_commandA=$exe_commandA"ifdown "
        exe_commandB=$exe_commandB"ifdown eth1"
    else
        SpineNode[$1]=1
        Available=$(($Available+1))
        exe_commandA=$exe_commandA"ifup "
        exe_commandB=$exe_commandB"ifup eth1"
    fi
    IPA=''
    IPB=''
    IPA=${LeafNodeIP[$1 / 4]}
    IPB=${SpineNodeIP[$1]}
    ssh root@$IPA $exe_commandA${Eth[$1 % 4]}
    ssh root@$IPB $exe_commandB
    echo "root@$IPA $exe_commandA${Eth[$1 % 4]}" >> /home/guolab/output/bgpTest.log
    echo "root@$IPB $exe_commandB" >> /home/guolab/output/bgpTest.log
    return 0;
}

#pssh -i -h /home/guolab/host.txt -t 0 "rm /home/guolab/output/bgpd.log;"
pssh -i -h /home/guolab/host.txt "/home/guolab/modifyZebraConf;"
# 修改BGP配置文件，adv interval
pssh -i -h /home/guolab/host.txt "/home/guolab/modifyBGPConf 1 2 4 16 2;"
rm -rf /home/guolab/output/bgpTest.log
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/output/*;"
pssh -i -H "root@10.0.8.86" -t 0 "chmod 777 -R /home/guolab/output/;"

for ((i=1;i<=100;i++))
do
    # 启动BGP
    echo "**********************************************************************" >> /home/guolab/output/bgpTest.log
    echo "                            第"$i"次实验                               " >> /home/guolab/output/bgpTest.log
    pssh -i -h /home/guolab/host.txt "rm /home/guolab/output/*;"
    pssh -i -h /home/guolab/host.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
    pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/output/;"

    # 启动PC
    sleep 27
    {
        echo "启动Client" >> /home/guolab/output/bgpTest.log
        # pssh -i -h PCHost.txt "killall client;"
        pssh -i -h PCHost.txt -t 0 "/home/guolab/client 6666;" &
        sleep 3
        {
            echo "启动Server" >> /home/guolab/output/bgpTest.log
            echo "" >> /home/guolab/output/bgpTest.log
            # pssh -i -H "root@10.0.8.86" "killall master;"
            # exe_command=''
            # exe_command=$exe_command"/home/guolab/master 3 172.16.1.2 172.16.2.2 172.16.3.2 6666 150 /home/guolab/output/BGP/BGPOutput-"
            # exe_command=$exe_command$i
            # exe_command=$exe_command".txt;"
            pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/master 3 172.16.1.2 172.16.2.2 172.16.3.2 6666 150 /home/guolab/output/BGPOutput.txt;" &
            # pssh -i -H "root@10.0.8.86" -t 0 $exe_command &
        }
    }

    sleep 10
    {
        echo "等待改变链路......" >> /home/guolab/output/bgpTest.log
    }

    echo "开始改变链路......" >> /home/guolab/output/bgpTest.log
    echo "" >> /home/guolab/output/bgpTest.log

    for ((j=1;j<=20;j++))
    do
        echo "第$j次改变链路状态" >> /home/guolab/output/bgpTest.log
        # 产生一个随机数
        random=$(($RANDOM+0))
        # echo "随机数" $random
        # 随机数对8取余，余数表示的是将要修改的SpineNode左侧下行链路
        index=$(($random % 8))
        # echo "index" $index
        temp=${SpineNode[$index]}
        if ([ $Available -eq 1 ] && [ $temp -eq 1 ])
        then
            echo "不改变链路状态......"  >> /home/guolab/output/bgpTest.log
        else
            ExecuteCommand $index;
        fi
        echo "当前链路状态 ${SpineNode[*]}" >> /home/guolab/output/bgpTest.log
        echo "可用链路数 $Available" >> /home/guolab/output/bgpTest.log
        echo "等待改变链路......" >> /home/guolab/output/bgpTest.log
        sleep 5
        {
            echo " " >> /home/guolab/output/bgpTest.log
        }
    done  

    sleep 20
    {
        echo "终止进程" >> /home/guolab/output/bgpTest.log
        pssh -i -h /home/guolab/host.txt "killall zebra; killall bgpd;"
        pssh -i -h /home/guolab/PCHost.txt "killall client;"
        pssh -i -H "root@10.0.8.86" "killall master;"
        # 确保所有网卡都已恢复
        # Spine
        ssh root@10.0.8.60 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.61 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.62 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.63 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.64 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.65 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.66 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.67 "ifup eth1;ifup eth2;"
        # Leaf
        ssh root@10.0.8.76 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;"
        ssh root@10.0.8.77 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;"
    }
    echo "休眠20s......" >> /home/guolab/output/bgpTest.log
    sleep 20
    {
        echo "" >> /home/guolab/output/bgpTest.log
        echo "" >> /home/guolab/output/bgpTest.log
    }
done