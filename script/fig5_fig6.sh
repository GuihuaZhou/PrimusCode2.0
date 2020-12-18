#!/bin/bash
# Macro-benchmark: Impact on Applications
directory="/home/guolab"
counter=1 # 实验次数
meanA=96990000 # 数据平面期望
meanB=500000 # 控制平面故障持续时间
time=1000 # s
# # 重启
# pssh -i -h $directory/host/node.txt "echo 'hello';"
# pssh -i -h $directory/host/node.txt "reboot;"

########################### BGP ###############################
# 准备实验环境，确保不会启动重复的进程
# pssh -i -h $directory/host/node.txt "killall zebra; killall bgpd;"
# pssh -i -h $directory/host/client.txt "killall -9 tcp-client;"
# pssh -i -h $directory/host/server.txt "killall -9 tcp-server;"

# # 8条链路两端的网卡地址
# LeafNodeIP=("10.0.80.20" "10.0.80.21")
# SpineNodeIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
# Eth=(eth1 eth2 eth3 eth4)

# killall -9 linkChange
# rm $directory/ssh-time-bgp.log
# rm $directory/test-bgp.log
# pssh -i -h $directory/host/client.txt "rm "$directory"/output/*;"
# pssh -i -h $directory/host/server.txt "rm "$directory"/output/*;"
# pssh -i -h $directory/host/node.txt "sysctl net.ipv4.fib_multipath_hash_policy=1;"

# for ((i=1;i<=counter;i++))
# do
#     # # 确保所有网卡都是正常的
#     pssh -i -h $directory/host/node.txt "ifup -a;"
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
#     echo "**********************************************************************" >> $directory/test-bgp.log
#     echo "                            第"$i"次实验                               " >> $directory/test-bgp.log
#     # ssh root@10.0.80.40 "echo -e \"\n                            第 $i 次实验                               \n\" >> $directory/output/server-record.txt"
#     # echo "---------------------" >> $directory/ssh-time-bgp.log
#     pssh -i -h $directory/host/node.txt "rm $directory/bgpd.log;"
#     pssh -i -h $directory/host/node.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
#     pssh -i -h $directory/host/node.txt "chmod 777 -R $directory/bgpd.log;"
#     pssh -i -h $directory/host/client.txt "killall -9 tcp-client;"
#     pssh -i -h $directory/host/server.txt "killall -9 tcp-server;"
#     killall -9 linkChange

#     sleep 20
#     echo "等待改变链路......" >> $directory/test-bgp.log
#     echo "启动Server" >> $directory/test-bgp.log
#     # echo "启动Server"
#     pssh -i -h $directory/host/server.txt "$directory/tcp-server 3;" &
#     # echo "启动Client" 
#     echo "启动Client" >> $directory/test-bgp.log
#     echo "" >> $directory/test-bgp.log
#     sleep 5
#     pssh -i -h $directory/host/client.txt "$directory/tcp-client 192.168.1.2;" &
#     sleep 5
#     # echo "开始改变链路......"
#     echo "开始改变链路......" >> $directory/test-bgp.log
#     echo "" >> $directory/test-bgp.log

#     $directory/tool/linkChange $directory/test-bgp.log $directory/tool/linkInfo-1.txt 0 0 0.25 $meanA $meanB BGP & # 输出文件路径、输入文件路径、运行次数、down/up间隔、两个参数、期望、路由类型

#     sleep $time
#     echo "终止进程"
#     echo "终止进程" >> $directory/test-bgp.log
#     pssh -i -h $directory/host/server.txt "killall -9 tcp-server;"
#     pssh -i -h $directory/host/client.txt "killall -9 tcp-client;"
#     pssh -i -h $directory/host/node.txt "killall zebra; killall bgpd;"
#     killall -9 linkChange

#     echo "休眠20s......" >> $directory/test-bgp.log
#     sleep 20
#     {
#         echo "" >> $directory/test-bgp.log
#         echo "" >> $directory/test-bgp.log
#     }
# done
########################### BGP ###############################


########################### Primus ###############################

# 准备实验环境，确保不会启动重复的进程
$directory/script/stop.sh
pssh -i -h $directory/host/client.txt "killall -9 tcp-client;"
pssh -i -h $directory/host/server.txt "killall -9 tcp-server;"

# 8条链路两端的网卡地址
LeafNodeIP=("10.0.80.20" "10.0.80.21")
SpineNodeIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
Eth=(eth1 eth2 eth3 eth4)

rm $directory/test-primus.log
pssh -i -h $directory/host/client.txt "rm "$directory"/output/*;"
pssh -i -h $directory/host/server.txt "rm "$directory"/output/*;"
pssh -i -h $directory/host/node.txt "sysctl net.ipv4.fib_multipath_hash_policy=1;"

for ((i=1;i<=counter;i++))
do
    # # 确保所有网卡都是正常的
    pssh -i -h $directory/host/node.txt "ifup -a;"
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
    echo "**********************************************************************" >> $directory/test-primus.log
    echo "                            第"$i"次实验                               " >> $directory/test-primus.log
    $directory/script/start.sh
    killall -9 linkChange
    pssh -i -h $directory/host/client.txt "killall -9 tcp-client;"
    pssh -i -h $directory/host/server.txt "killall -9 tcp-server;"

    sleep 20
    echo "等待改变链路......" >> $directory/test-primus.log
    echo "启动Server" >> $directory/test-primus.log
    # echo "启动Server"
    pssh -i -h $directory/host/server.txt "$directory/tcp-server 3;" &
    # echo "启动Client" 
    echo "启动Client" >> $directory/test-primus.log
    echo "" >> $directory/test-primus.log
    sleep 5
    pssh -i -h $directory/host/client.txt ""$directory"/tcp-client 192.168.1.2;" &
    sleep 5
    echo "开始改变链路......"
    echo "开始改变链路......" >> $directory/test-primus.log
    echo "" >> $directory/test-primus.log

    $directory/tool/linkChange $directory/test-primus.log $directory/tool/linkInfo-2.txt 0 0 0.25 $meanA $meanB Primus &

    sleep $time
    echo "终止进程"
    echo "终止进程" >> $directory/test-primus.log
    pssh -i -h $directory/host/server.txt "killall -9 tcp-server;"
    pssh -i -h $directory/host/client.txt "killall -9 tcp-client;"
    $directory/script/stop.sh

    echo "休眠20s......" >> $directory/test-primus.log
    sleep 20
    {
        echo "" >> $directory/test-primus.log
        echo "" >> $directory/primus.log
    }
done

########################### Primus ###############################
