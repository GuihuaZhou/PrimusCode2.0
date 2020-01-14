#!/bin/bash
# Macro-benchmark: Impact on Applications
counter=1 # 实验次数
# # 重启
# pssh -i -h /home/guolab/host/ATChost.txt "echo 'hello';"
# pssh -i -h /home/guolab/host/ATChost.txt "reboot;"

########################### BGP ###############################
# # 准备实验环境，确保不会启动重复的进程
# pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
# pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
# pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"

# # 8条链路两端的网卡地址
# LeafNodeIP=("10.0.80.20" "10.0.80.21")
# SpineNodeIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
# Eth=(eth1 eth2 eth3 eth4)

# killall -9 linkChange
# rm /home/guolab/output/ssh-time-bgp.log
# rm /home/guolab/output/ATCTest-bgp.log
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
#     echo "**********************************************************************" >> /home/guolab/output/ATCTest-bgp.log
#     echo "                            第"$i"次实验                               " >> /home/guolab/output/ATCTest-bgp.log
#     # ssh root@10.0.80.40 "echo -e \"\n                            第 $i 次实验                               \n\" >> /home/guolab/ATCOutput/server-record.txt"
#     # echo "---------------------" >> /home/guolab/output/ssh-time-bgp.log
#     pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/bgpd.log;"
#     pssh -i -h /home/guolab/host/ATChost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
#     pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /home/guolab/bgpd.log;"
#     pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
#     pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"
#     killall -9 linkChange

#     sleep 20
#     echo "等待改变链路......" >> /home/guolab/output/ATCTest-bgp.log
#     echo "启动Server" >> /home/guolab/output/ATCTest-bgp.log
#     # echo "启动Server"
#     pssh -i -h /home/guolab/host/ATCServerhost.txt "/home/guolab/tcp-server 3;" &
#     # echo "启动Client" 
#     echo "启动Client" >> /home/guolab/output/ATCTest-bgp.log
#     echo "" >> /home/guolab/output/ATCTest-bgp.log
#     sleep 5
#     pssh -i -h /home/guolab/host/ATCClienthost.txt "/home/guolab/tcp-client 192.168.1.2;" &
#     sleep 5
#     # echo "开始改变链路......"
#     echo "开始改变链路......" >> /home/guolab/output/ATCTest-bgp.log
#     echo "" >> /home/guolab/output/ATCTest-bgp.log
    
#     # for ((j=1;j<=20;j++))
#     # do
#     #     # echo "第$j次改变链路状态"
#     #     echo "第 $j 次改变链路状态" >> /home/guolab/output/ATCTest-bgp.log
#     #     # 产生一个随机数
#     #     random=$(($RANDOM+0))
#     #     # echo "随机数" $random
#     #     # 随机数对8取余，余数表示的是将要修改的SpineNode左侧下行链路
#     #     SpineIndex=$(($random % ${#SpineNodeIP[@]}))
#     #     # echo "SpineIndex" $SpineIndex
#     #     if ([ $Available -eq 1 ] && [ ${SpineNode[$SpineIndex]} -eq 1 ])
#     #     then
#     #         echo "不改变链路状态......"  >> /home/guolab/output/ATCTest-bgp.log
#     #         # echo "不改变链路状态......"
#     #         # echo "---------------------" >> /home/guolab/output/ssh-time-bgp.log
#     #     else
#     #         exe_commandA=''
#     #         exe_commandB=''
#     #         if [ ${SpineNode[$SpineIndex]} -eq 1 ]
#     #         then
#     #             SpineNode[$SpineIndex]=0
#     #             Available=$(($Available-1))
#     #             exe_commandA=$exe_commandA"ifconfig eth1 down"
#     #             exe_commandB=$exe_commandB"ifconfig "${Eth[$SpineIndex % ${#Eth[@]} ]}" down"
#     #         else
#     #             SpineNode[$SpineIndex]=1
#     #             Available=$(($Available+1))
#     #             exe_commandA=$exe_commandA"ifconfig eth1 up"
#     #             exe_commandB=$exe_commandB"ifconfig "${Eth[$SpineIndex % ${#Eth[@]} ]}" up"
#     #         fi
#     #         IPA=''
#     #         IPB=''
#     #         IPA=${SpineNodeIP[$SpineIndex]}
#     #         IPB=${LeafNodeIP[$SpineIndex / 4]}
#     #         # date +%s.%3N >> /home/guolab/output/ssh-time-bgp.log
#     #         ssh root@$IPA $exe_commandA
#     #         # date +%s.%3N >> /home/guolab/output/ssh-time-bgp.log
#     #         ssh root@$IPB $exe_commandB
#     #         # date +%s.%3N >> /home/guolab/output/ssh-time-bgp.log
#     #         echo "ssh root@$IPA \"$exe_commandA\"" >> /home/guolab/output/ATCTest-bgp.log
#     #         echo "ssh root@$IPB \"$exe_commandB\"" >> /home/guolab/output/ATCTest-bgp.log
#     #         # echo "ssh root@$IPA \"$exe_commandA\""
#     #         # echo "ssh root@$IPB \"$exe_commandB\"" 
#     #         return 0;
#     #     fi
#     #     # ssh root@10.0.80.40 "echo -e \"\n第 $j 次改变链路状态\n\" >> /home/guolab/ATCOutput/server-record.txt"
#     #     echo "当前链路状态 ${SpineNode[*]}" >> /home/guolab/output/ATCTest-bgp.log
#     #     echo "可用链路数 $Available" >> /home/guolab/output/ATCTest-bgp.log
#     #     echo "等待改变链路......" >> /home/guolab/output/ATCTest-bgp.log
#     #     echo " " >> /home/guolab/output/ATCTest-bgp.log
#     #     # echo " "
#     #     # echo "SpineNode 左侧链路: ${SpineNode[*]}" 
#     #     # echo "可用链路数 $Available"
#     #     # echo " "
#     #     sleep 5
#     # done

#     /home/guolab/script/linkChange /home/guolab/output/ATCTest-bgp.log /home/guolab/tool/linkInfo.txt 10 0 0 0.25 BGP # 输出文件路径、输入文件路径、运行次数、down/up间隔、两个参数、路由类型

#     # ssh root@10.0.80.20 "ifconfig eth5 down"
#     # ssh root@10.0.80.10 "ifconfig eth1 down"
#     # # 
#     # sleep 5
#     # ssh root@10.0.80.21 "ifconfig eth5 down"
#     # ssh root@10.0.80.10 "ifconfig eth2 down"
#     # ssh root@10.0.80.20 "ifconfig eth5 up"
#     # ssh root@10.0.80.10 "ifconfig eth1 up"
#     # # 
#     # sleep 5
#     # ssh root@10.0.80.21 "ifconfig eth5 up"
#     # ssh root@10.0.80.10 "ifconfig eth2 up"
#     # ssh root@10.0.80.20 "ifconfig eth5 down"
#     # ssh root@10.0.80.10 "ifconfig eth1 down"
#     # sleep 10

#     echo "终止进程"
#     echo "终止进程" >> /home/guolab/output/ATCTest-bgp.log
#     pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"
#     pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
#     pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
#     killall -9 linkChange

#     echo "休眠20s......" >> /home/guolab/output/ATCTest-bgp.log
#     sleep 20
#     {
#         echo "" >> /home/guolab/output/ATCTest-bgp.log
#         echo "" >> /home/guolab/output/ATCTest-bgp.log
#     }
# done
########################### BGP ###############################


########################### Primus ###############################

# # 准备实验环境，确保不会启动重复的进程
/home/guolab/script/stop.sh
pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"

# 8条链路两端的网卡地址
LeafNodeIP=("10.0.80.20" "10.0.80.21")
SpineNodeIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
Eth=(eth1 eth2 eth3 eth4)

rm /home/guolab/output/ATCTest-primus.log
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
    echo "**********************************************************************" >> /home/guolab/output/ATCTest-primus.log
    echo "                            第"$i"次实验                               " >> /home/guolab/output/ATCTest-primus.log
    # ssh root@10.0.80.40 "echo -e \"\n                            第 $i 次实验                               \n\" >> /home/guolab/ATCOutput/server-record.txt"
    /home/guolab/script/start.sh
    killall -9 linkChange
    pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
    pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"

    sleep 20
    echo "等待改变链路......" >> /home/guolab/output/ATCTest-primus.log
    echo "启动Server" >> /home/guolab/output/ATCTest-primus.log
    # echo "启动Server"
    pssh -i -h /home/guolab/host/ATCServerhost.txt "/home/guolab/tcp-server 3;" &
    # echo "启动Client" 
    echo "启动Client" >> /home/guolab/output/ATCTest-primus.log
    echo "" >> /home/guolab/output/ATCTest-primus.log
    sleep 5
    pssh -i -h /home/guolab/host/ATCClienthost.txt "/home/guolab/tcp-client 192.168.1.2;" &
    sleep 5
    echo "开始改变链路......"
    echo "开始改变链路......" >> /home/guolab/output/ATCTest-primus.log
    echo "" >> /home/guolab/output/ATCTest-primus.log
    
    # for ((j=1;j<=20;j++))
    # do
    #     # echo "第 $j 次改变链路状态"
    #     echo "第 $j 次改变链路状态" >> /home/guolab/output/ATCTest-primus.log
    #     # 产生一个随机数
    #     random=$(($RANDOM+0)) # echo "随机数" $random
    #     type=$(($random % 4)) # 对4取余，如果为0，则修改控制平面链路，如果不为0，则修改数据平面链路
    #     # 修改数据平面链路的概率是控制平面的k-1倍
    #     if ([ $type -eq 0 ]) # 修改控制平面链路
    #     then
    #         # 随机数对2取余，余数表示的是将要修改SpineNode或LeafNode的管理网口状态
    #         random=$(($RANDOM+0)) # 产生一个新的随机数
    #         level=$(($random%2))
    #         exe_command=''
    #         random=$(($RANDOM+0)) # 产生一个新的随机数
    #         if ([ $level -eq 0 ]) # 改变某个Leaf的网口状态
    #         then
    #             LeafIndex=$(($random%${#LeafNodeIP[@]})) # 选择要改变的Leaf
    #             IP=${LeafNodeIP[$LeafIndex]} # 获得Leaf的IP
    #             if ([ ${LeafMGMT[$LeafIndex]} -eq 1 ]) # 当前网口状态是up
    #             then 
    #                 LeafMGMT[$LeafIndex]=0
    #                 exe_command=$exe_command"ifconfig eth0 down"
    #                 ssh root@$IP $exe_command
    #                 echo "ssh root@$IP \"$exe_command\"" >> /home/guolab/output/ATCTest-primus.log
    #                 echo "ssh root@$IP \"$exe_command\""
    #             else
    #                 LeafMGMT[$LeafIndex]=1
    #                 exe_command=$exe_command"ifconfig eth0 up"
    #                 ssh root@$IP $exe_command
    #                 echo "ssh root@$IP \"$exe_command\"" >> /home/guolab/output/ATCTest-primus.log
    #                 echo "ssh root@$IP \"$exe_command\""
    #             fi
    #         else # 改变某个Spine的网口状态
    #             SpineIndex=$(($random%${#SpineNodeIP[@]})) # 选择要改变的Spine
    #             IP=${SpineNodeIP[$SpineIndex]} # 获得Spine的IP
    #             if ([ ${SpineMGMT[$SpineIndex]} -eq 1 ]) # 当前网口状态是up
    #             then 
    #                 SpineMGMT[$SpineIndex]=0
    #                 exe_command=$exe_command"ifconfig eth0 down"
    #                 ssh root@$IP $exe_command
    #                 echo "ssh root@$IP \"$exe_command\"" >> /home/guolab/output/ATCTest-primus.log
    #                 echo "ssh root@$IP \"$exe_command\""
    #             else
    #                 SpineMGMT[$SpineIndex]=1
    #                 exe_command=$exe_command"ifconfig eth0 up"
    #                 ssh root@$IP $exe_command 
    #                 echo "ssh root@$IP \"$exe_command\"" >> /home/guolab/output/ATCTest-primus.log
    #                 echo "ssh root@$IP \"$exe_command\""
    #             fi               
    #         fi
    #     else # 改变数据平面的链路状态
    #         random=$(($RANDOM+0)) # 产生一个新的随机数
    #         SpineIndex=$(($random % ${#SpineNodeIP[@]})) # 随机数对8取余，余数表示的是将要修改的SpineNode左侧下行链路
    #         if ([ $Available -eq 1 ] && [ ${SpineNode[$SpineIndex]} -eq 1 ])
    #         then
    #             echo "不改变链路状态......"  >> /home/guolab/output/ATCTest-primus.log
    #         else
    #             exe_commandA=''
    #             # exe_commandB=''
    #             if [ ${SpineNode[$SpineIndex]} -eq 1 ]
    #             then
    #                 SpineNode[$SpineIndex]=0
    #                 Available=$(($Available-1))
    #                 exe_commandA=$exe_commandA"ifconfig eth1 down"
    #                 # exe_commandB=$exe_commandB"ifconfig "${Eth[$SpineIndex % ${#Eth[@]} ]}" down"
    #             else
    #                 SpineNode[$SpineIndex]=1
    #                 Available=$(($Available+1))
    #                 exe_commandA=$exe_commandA"ifconfig eth1 up"
    #                 # exe_commandB=$exe_commandB"ifconfig "${Eth[$SpineIndex % ${#Eth[@]} ]}" up"
    #             fi
    #             IPA=''
    #             IPB=''
    #             IPA=${SpineNodeIP[$SpineIndex]}
    #             # IPB=${LeafNodeIP[$SpineIndex / 4]}
    #             ssh root@$IPA $exe_commandA
    #             # ssh root@$IPB $exe_commandB
    #             echo "ssh root@$IPA \"$exe_commandA\"" >> /home/guolab/output/ATCTest-primus.log
    #             # echo "ssh root@$IPB \"$exe_commandB\"" >> /home/guolab/output/ATCTest-primus.log
    #             echo "ssh root@$IPA \"$exe_commandA\""
    #             # echo "ssh root@$IPB \"$exe_commandB\""
    #         fi
    #     fi
    #     # ssh root@10.0.80.40 "echo -e \"\n第 $j 次改变链路状态\n\" >> /home/guolab/ATCOutput/server-record.txt"
    #     echo "SpineNode 可用左侧链路数: $Available" >> /home/guolab/output/ATCTest-primus.log
    #     echo "SpineNode 左侧链路: ${SpineNode[*]}" >> /home/guolab/output/ATCTest-primus.log
    #     echo "SpineNode 直连链路: ${SpineMGMT[*]}" >> /home/guolab/output/ATCTest-primus.log
    #     echo "LeafNode  直连链路: ${LeafMGMT[*]}" >> /home/guolab/output/ATCTest-primus.log
    #     echo "等待改变链路......" >> /home/guolab/output/ATCTest-primus.log
    #     echo " " >> /home/guolab/output/ATCTest-primus.log
    #     echo "SpineNode 可用左侧链路数: $Available"
    #     echo "SpineNode 左侧链路: ${SpineNode[*]}" 
    #     echo "SpineNode 直连链路: ${SpineMGMT[*]}"
    #     echo "LeafNode  直连链路: ${LeafMGMT[*]}"
    #     echo "等待改变链路......"
    #     echo " "
    #     sleep 5
    # done  

    /home/guolab/script/linkChange /home/guolab/output/ATCTest-primus.log /home/guolab/tool/linkInfo.txt 10 0 0 0.25 Primus
    # ssh root@10.0.80.20 "ifconfig eth5 down"
    # ssh root@10.0.80.10 "ifconfig eth1 down"
    # # 
    # sleep 5
    # ssh root@10.0.80.21 "ifconfig eth5 down"
    # ssh root@10.0.80.10 "ifconfig eth2 down"
    # ssh root@10.0.80.20 "ifconfig eth5 up"
    # ssh root@10.0.80.10 "ifconfig eth1 up"
    # # 
    # sleep 5
    # ssh root@10.0.80.21 "ifconfig eth5 up"
    # ssh root@10.0.80.10 "ifconfig eth2 up"
    # ssh root@10.0.80.20 "ifconfig eth5 down"
    # ssh root@10.0.80.10 "ifconfig eth1 down"

    echo "终止进程"
    echo "终止进程" >> /home/guolab/output/ATCTest-primus.log
    pssh -i -h /home/guolab/host/ATCServerhost.txt "killall -9 tcp-server;"
    pssh -i -h /home/guolab/host/ATCClienthost.txt "killall -9 tcp-client;"
    /home/guolab/script/stop.sh

    echo "休眠20s......" >> /home/guolab/output/ATCTest-primus.log
    sleep 20
    {
        echo "" >> /home/guolab/output/ATCTest-primus.log
        echo "" >> /home/guolab/output/ATCTest-primus.log
    }
done

########################### Primus ###############################
