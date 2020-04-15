#!/bin/bash
# Macro-benchmark: Impact on Applications
counter=1 # 实验次数
mean=9699000 # 期望
time=100 # s
# # 重启
# pssh -i -h /home/guolab/host/ATChost.txt "echo 'hello';"
# pssh -i -h /home/guolab/host/ATChost.txt "reboot;"

########################### BGP ###############################
# 准备实验环境，确保不会启动重复的进程
# pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
# pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
# pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"

# # 8条链路两端的网卡地址
# LeafNodeIP=("10.0.80.20" "10.0.80.21")
# SpineNodeIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
# Eth=(eth1 eth2 eth3 eth4)

# killall -9 linkChange
# rm /home/guolab/ssh-time-bgp.log
# rm /home/guolab/ATCTest-bgp.log
# pssh -i -h /home/guolab/host/ATCClienthost.txt "rm /home/guolab/ATCOutput/*;"
# pssh -i -h /home/guolab/host/ATCServerhost.txt "rm /home/guolab/ATCOutput/*;"
# pssh -i -h /home/guolab/host/ATChost.txt "sysctl net.ipv4.fib_multipath_hash_policy=1;"

# for ((i=1;i<=counter;i++))
# do
#     # # 确保所有网卡都是正常的
#     pssh -i -h /home/guolab/host/ATChost.txt "ifup -a;"
#     # spine交换机的管理网口状态
#     SpineMGMT=(1 1 1 1 1 1 1 1)
#     # leaf交换机的管理网口状态
#     LeafMGMT=(1 1)
#     # 1表示8个SpineNode左侧的链路状态均为正常
#     SpineNode=(1 1 1 1 1 1 1 1)
#     # 表示8个SpineNode左侧的链路中可用的条数，避免为0
#     Available=8

#     random=7
#     index=7
#     temp=7
#     # 启动BGP
#     echo "**********************************************************************" >> /home/guolab/ATCTest-bgp.log
#     echo "                            第"$i"次实验                               " >> /home/guolab/ATCTest-bgp.log
#     # ssh root@10.0.80.40 "echo -e \"\n                            第 $i 次实验                               \n\" >> /home/guolab/ATCOutput/server-record.txt"
#     # echo "---------------------" >> /home/guolab/ssh-time-bgp.log
#     pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/bgpd.log;"
#     pssh -i -h /home/guolab/host/ATChost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
#     pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /home/guolab/bgpd.log;"
#     pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
#     pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"
#     killall -9 linkChange

#     sleep 20
#     echo "等待改变链路......" >> /home/guolab/ATCTest-bgp.log
#     echo "启动Server" >> /home/guolab/ATCTest-bgp.log
#     # echo "启动Server"
#     pssh -i -h /home/guolab/host/ATCServerhost.txt "/home/guolab/tcp-server 3;" &
#     # echo "启动Client" 
#     echo "启动Client" >> /home/guolab/ATCTest-bgp.log
#     echo "" >> /home/guolab/ATCTest-bgp.log
#     sleep 5
#     pssh -i -h /home/guolab/host/ATCClienthost.txt "/home/guolab/tcp-client 192.168.1.2;" &
#     sleep 5
#     # echo "开始改变链路......"
#     echo "开始改变链路......" >> /home/guolab/ATCTest-bgp.log
#     echo "" >> /home/guolab/ATCTest-bgp.log

#     /home/guolab/tool/linkChange /home/guolab/ATCTest-bgp.log /home/guolab/tool/linkInfo-1.txt 0 0 0.25 $mean BGP & # 输出文件路径、输入文件路径、运行次数、down/up间隔、两个参数、期望、路由类型

#     sleep $time
#     echo "终止进程"
#     echo "终止进程" >> /home/guolab/ATCTest-bgp.log
#     pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"
#     pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
#     pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
#     killall -9 linkChange

#     echo "休眠20s......" >> /home/guolab/ATCTest-bgp.log
#     sleep 20
#     {
#         echo "" >> /home/guolab/ATCTest-bgp.log
#         echo "" >> /home/guolab/ATCTest-bgp.log
#     }
# done
########################### BGP ###############################


########################### Primus ###############################

# 准备实验环境，确保不会启动重复的进程
/home/guolab/script/stop.sh
pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"

# 8条链路两端的网卡地址
LeafNodeIP=("10.0.80.20" "10.0.80.21")
SpineNodeIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
Eth=(eth1 eth2 eth3 eth4)

rm /home/guolab/ATCTest-primus.log
pssh -i -h /home/guolab/host/ATCClienthost.txt "rm /home/guolab/ATCOutput/*;"
pssh -i -h /home/guolab/host/ATCServerhost.txt "rm /home/guolab/ATCOutput/*;"
pssh -i -h /home/guolab/host/ATChost.txt "sysctl net.ipv4.fib_multipath_hash_policy=1;"

for ((i=1;i<=counter;i++))
do
    # # 确保所有网卡都是正常的
    pssh -i -h /home/guolab/host/ATChost.txt "ifup -a;"
    # spine交换机的管理网口状态
    SpineMGMT=(1 1 1 1 1 1 1 1)
    # leaf交换机的管理网口状态
    LeafMGMT=(1 1)
    # 1表示8个SpineNode左侧的链路状态均为正常
    SpineNode=(1 1 1 1 1 1 1 1)
    # 表示8个SpineNode左侧的链路中可用的条数，避免为0
    Available=8

    random=7
    index=7
    temp=7

    # 启动Primus
    echo "**********************************************************************" >> /home/guolab/ATCTest-primus.log
    echo "                            第"$i"次实验                               " >> /home/guolab/ATCTest-primus.log
    /home/guolab/script/start.sh
    killall -9 linkChange
    pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
    pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"

    sleep 20
    echo "等待改变链路......" >> /home/guolab/ATCTest-primus.log
    echo "启动Server" >> /home/guolab/ATCTest-primus.log
    # echo "启动Server"
    pssh -i -h /home/guolab/host/ATCServerhost.txt "/home/guolab/tcp-server 3;" &
    # echo "启动Client" 
    echo "启动Client" >> /home/guolab/ATCTest-primus.log
    echo "" >> /home/guolab/ATCTest-primus.log
    sleep 5
    pssh -i -h /home/guolab/host/ATCClienthost.txt "/home/guolab/tcp-client 192.168.1.2;" &
    sleep 5
    echo "开始改变链路......"
    echo "开始改变链路......" >> /home/guolab/ATCTest-primus.log
    echo "" >> /home/guolab/ATCTest-primus.log

    /home/guolab/tool/linkChange /home/guolab/ATCTest-primus.log /home/guolab/tool/linkInfo-1.txt 0 0 0.25 $mean Primus &

    sleep $time
    echo "终止进程"
    echo "终止进程" >> /home/guolab/ATCTest-primus.log
    pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"
    pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
    /home/guolab/script/stop.sh

    echo "休眠20s......" >> /home/guolab/ATCTest-primus.log
    sleep 20
    {
        echo "" >> /home/guolab/ATCTest-primus.log
        echo "" >> /home/guolab/ATCTest-primus.log
    }
done

########################### Primus ###############################
