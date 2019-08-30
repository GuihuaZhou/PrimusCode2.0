#!/bin/bash
# 准备实验环境，确保不会启动重复的进程
pssh -i -h host.txt "sudo killall zebra; killall bgpd;"
pssh -i -h /home/guolab/PCHost.txt "killall udp-client-my;"
pssh -i -H "root@10.0.8.86" "killall udp-server-my;"

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
ssh root@10.0.8.76 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
ssh root@10.0.8.77 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
ssh root@10.0.8.80 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
ssh root@10.0.8.81 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"

#ToR
ssh root@10.0.8.84 "ifup eth1;ifup eth2;"
ssh root@10.0.8.85 "ifup eth1;ifup eth2;"

# 1表示8个SpineNode左侧的链路状态均为正常
# SpineNode=(1 1 1 1 1 1 1 1)
# 1表示ToRNode1.1的两条链路状态均为正常
ToRNode=(1 1)
# 表示8个SpineNode左侧的链路中可用的条数，避免为0
# Available=8
# 表示ToRNode1.1的链路中可用的条数，避免为0
Available=2
# 8条链路两端的网卡地址
ToRNodeIP=("10.0.8.84" "10.0.8.85")
LeafNodeIP=("10.0.8.76" "10.0.8.77" "10.0.8.80" "10.0.8.81")
SpineNodeIP=("10.0.8.60" "10.0.8.61" "10.0.8.62" "10.0.8.63" "10.0.8.64" "10.0.8.65" "10.0.8.66" "10.0.8.67")
Eth=(eth1 eth2 eth3 eth4)

random=7
index=7
temp=7

function ExecuteCommand()
{
    exe_commandA=''
    exe_commandB=''
    if [ ${ToRNode[$1]} -eq 1 ]
    then
        ToRNode[$1]=0
        Available=$(($Available-1))
        exe_commandA=$exe_commandA"ifdown "
        exe_commandB=$exe_commandB"ifdown eth5"
    else
        ToRNode[$1]=1
        Available=$(($Available+1))
        exe_commandA=$exe_commandA"ifup "
        exe_commandB=$exe_commandB"ifup eth5"
    fi
    IPA=''
    IPB=''
    IPA=${ToRNodeIP[0]}
    IPB=${LeafNodeIP[$1]}
    ssh root@$IPA $exe_commandA${Eth[$1 % 4]}
    ssh root@$IPB $exe_commandB
    echo "root@$IPA $exe_commandA${Eth[$1 % 4]}" >> /home/guolab/output/bgpTest.log
    echo "root@$IPB $exe_commandB" >> /home/guolab/output/bgpTest.log
    return 0;
}

#pssh -i -h /home/guolab/host.txt -t 0 "rm /home/guolab/output/bgpd.log;"
pssh -i -h /home/guolab/host.txt "/home/guolab/modifyZebraConf;"
# 修改BGP配置文件，adv interval
pssh -i -h /home/guolab/host.txt "/home/guolab/modifyBGPConf 1;"
rm -rf /home/guolab/output/bgpTest.log
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/output/*;"
pssh -i -H "root@10.0.8.86" -t 0 "chmod 777 -R /home/guolab/output/;"

# 启动BGP
# echo "**********************************************************************" >> /home/guolab/output/bgpTest.log
# echo "                            第"$i"次实验                               " >> /home/guolab/output/bgpTest.log
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/output/*;"
# pssh -i -h /home/guolab/host.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
# pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/output/;"




for ((i=1;i<=10;i++))
do
    # 启动BGP
    echo "**********************************************************************" >> /home/guolab/output/bgpTest.log
    echo "                            第"$i"次实验                               " >> /home/guolab/output/bgpTest.log
    pssh -i -h /home/guolab/host.txt "rm /home/guolab/output/*;"
    pssh -i -h /home/guolab/host.txt "sudo zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
    pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/output/;"

    # 启动PC
    sleep 17
    {
        echo "启动Client" >> /home/guolab/output/bgpTest.log
        # pssh -i -h PCHost.txt "killall client;"
        pssh -i -h /home/guolab/PCHost.txt -t 0 "/home/guolab/udp-client-my 192.168.1.2 100 2000 /home/guolab/output/udpResult.txt;" &
        echo "启动Server" >> /home/guolab/output/bgpTest.log
        echo "" >> /home/guolab/output/bgpTest.log
        # pssh -i -H "root@10.0.8.86" "killall master;"
        # exe_command=''
        # exe_command=$exe_command"/home/guolab/master 3 172.16.1.2 172.16.2.2 172.16.3.2 6666 150 /home/guolab/output/BGP/BGPOutput-"
        # exe_command=$exe_command$i
        # exe_command=$exe_command".txt;"
        pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/udp-server-my 2000 /home/guolab/output/udpResults.txt  /home/guolab/output/udpThroughput.txt" &
        # pssh -i -H "root@10.0.8.86" -t 0 $exe_command &
    }

    sleep 12
    {
        echo "等待改变链路......" >> /home/guolab/output/bgpTest.log
    }

    index=1
    echo "开始改变链路......" >> /home/guolab/output/bgpTest.log
    echo "" >> /home/guolab/output/bgpTest.log
    ExecuteCommand $index;
    echo "当前链路状态 ${ToRNode[*]}" >> /home/guolab/output/bgpTest.log
    echo "可用链路数 $Available" >> /home/guolab/output/bgpTest.log

    #sleep 3
    #{
    #    echo "开始改变链路......" >> /home/guolab/output/centerTest.log
    #    ExecuteCommand $index;
    #    echo " " >> /home/guolab/output/centerTest.log
    #}

    sleep 30
    {
        echo "终止进程" >> /home/guolab/output/bgpTest.log
        pssh -i -h /home/guolab/host.txt "sudo killall zebra; killall bgpd;"
        pssh -i -h /home/guolab/PCHost.txt "killall udp-client-my;"
        pssh -i -H "root@10.0.8.86" "killall udp-server-my;"
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
        ssh root@10.0.8.76 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
        ssh root@10.0.8.77 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
        ssh root@10.0.8.80 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
        ssh root@10.0.8.81 "ifup eth1;ifup eth2;ifup eth3;ifup eth4;ifup eth5;"
        # ToR
        ssh root@10.0.8.84 "ifup eth1;ifup eth2;"
        ssh root@10.0.8.85 "ifup eth1;ifup eth2;"

    }
    echo "休眠20s......" >> /home/guolab/output/bgpTest.log
    sleep 20
    {
        echo "" >> /home/guolab/output/bgpTest.log
        echo "" >> /home/guolab/output/bgpTest.log
    }
done
