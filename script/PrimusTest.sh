###########################################################
# sudo pssh -i -h /home/guolab/host/ATChost.txt -t 0 "sysctl net.ipv4.fib_multipath_hash_policy=1;"
###########################################################

###################################################
# 1表示8个SpineNode的16条下行链路状态均为正常
SLLink=(1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1) # S:Spine L:Leaf
# 4个LeafNode的8条下行链路
LTLink=(1 1 1 1 1 1 1 1)

# 3层交换机的管理网口状态
SpineMGMT=(1 1 1 1 1 1 1 1)
LeafMGMT=(1 1 1 1)
ToRMGMT=(1 1 1 1)

SpineIP=("10.0.80.30" "10.0.80.31" "10.0.80.32" "10.0.80.33" "10.0.80.34" "10.0.80.35" "10.0.80.36" "10.0.80.37")
LeafIP=("10.0.80.20" "10.0.80.21" "10.0.80.22" "10.0.80.23")
ToRIP=("10.0.80.10" "10.0.80.11" "10.0.80.12" "10.0.80.13")

SpineEth=(eth0 eth1 eth2)
LeafEth=(eth0 eth5 eth6)

# function ExecuteCommand()
# {
#     exe_commandA=''
#     exe_commandB=''
#     if [ ${SLLink[$1]} -eq 1 ]
#     then
#         SLLink[$1]=0
#         Available=$(($Available-1))
#         exe_commandA=$exe_commandA"ifdown "
#         exe_commandB=$exe_commandB"ifdown eth1"
#     else
#         SLLink[$1]=1
#         Available=$(($Available+1))
#         exe_commandA=$exe_commandA"ifconfig "
#  up        exe_commandB=$exe_commandB"ifconfig eth1 up"
#     fi
#     IPA=''
#     IPB=''
#     IPA=${LeafNodeIP[$1 / 4]}
#     IPB=${SpineNodeIP[$1]}
#     ssh root@$IPA $exe_commandA${Eth[$1 % 4]}
#     ssh root@$IPB $exe_commandB
#     echo "root@$IPA $exe_commandA${Eth[$1 % 4]}" >> /home/guolab/PrimusTest.log
#     echo "root@$IPB $exe_commandB" >> /home/guolab/PrimusTest.log
#     return 0;
# }

# Spine
ssh root@10.0.80.30 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.31 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.32 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.33 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.34 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.35 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.36 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.37 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# Leaf
ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
ssh root@10.0.80.21 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
ssh root@10.0.80.22 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
ssh root@10.0.80.23 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
# ToR
ssh root@10.0.80.10 "ifconfig eth0 up;"
ssh root@10.0.80.11 "ifconfig eth0 up;"
ssh root@10.0.80.12 "ifconfig eth0 up;"
ssh root@10.0.80.13 "ifconfig eth0 up;"

# pssh -i -h /home/guolab/host/ATCServerhost.txt -t 0 "rm -rf /home/guolab/serverRecord*.txt;"
rm -rf /home/guolab/PrimusTest.log
rm -rf /home/guolab/MasterLinkTable.txt

for ((i=1;i<=1;i++))
do
	# # # iperf3 tcp
	echo "**********************************************************************" >> /home/guolab/PrimusTest.log
    echo "                            第"$i"次实验                               " >> /home/guolab/PrimusTest.log
	echo "启动Primus." 
	/home/guolab/script/start.sh

	# echo ""
	# echo "Tcp"
	# pssh -i -h /home/guolab/host/ATCServerhost.txt -t 0 "killall -9 iperf3;"
	# pssh -i -h /home/guolab/host/ATCClienthost.txt -t 0 "killall -9 iperf3;"
	# sleep 5

	# echo ""
	# echo "启动iperf-server."
	# pssh -i -h /home/guolab/host/ATCServerhost.txt -t 0 "sudo iperf3 -s -p 2048 -i 0.5 > /home/guolab/serverRecord-p2048.txt;" &
	# pssh -i -h /home/guolab/host/ATCServerhost.txt -t 0 "sudo iperf3 -s -p 2049 -i 0.5 > /home/guolab/serverRecord-p2049.txt;" &
	# sleep 5

	# echo ""
	# echo "启动iperf-client."
	# pssh -i -H "root@10.0.80.50" -t 0 "sudo iperf3 -c 192.168.1.2 -P 10 -p 2048 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.50" -t 0 "sudo iperf3 -c 192.168.1.2 -P 10 -p 2049 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.51" -t 0 "sudo iperf3 -c 192.168.1.3 -P 10 -p 2048 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.51" -t 0 "sudo iperf3 -c 192.168.1.3 -P 10 -p 2049 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.52" -t 0 "sudo iperf3 -c 192.168.1.4 -P 10 -p 2048 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.52" -t 0 "sudo iperf3 -c 192.168.1.4 -P 10 -p 2049 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.53" -t 0 "sudo iperf3 -c 192.168.1.5 -P 10 -p 2048 -t 40 -w 200K;" &
	# pssh -i -H "root@10.0.80.53" -t 0 "sudo iperf3 -c 192.168.1.5 -P 10 -p 2049 -t 40 -w 200K;" &

	sleep 20
    {
        echo "等待改变链路......" >> /home/guolab/PrimusTest.log
    }

    echo "开始改变链路......" >> /home/guolab/PrimusTest.log
    echo "" >> /home/guolab/PrimusTest.log

    for ((j=1;j<=36;j++))
    do
        echo "第$j次改变链路状态" >> /home/guolab/PrimusTest.log
        # 产生一个随机数
        random=$(($RANDOM+0))
        # echo "随机数" $random
        # 随机数对3取余，余数表示的是将要修改SpineNode或LeafNode或ToR的网口状态
        level=$((($random%3)+1))
        exe_command=''
        if ([ $level -eq 1 ]) # 改变某个ToR的网口状态
        then
            ToRIndex=$(($random%4)) # 选择要改变的ToR
            IP=${ToRIP[$ToRIndex]} # 获得ToR的IP
            if ([ ${ToRMGMT[$ToRIndex]} -eq 1 ]) # 当前网口状态是up
            then 
                ToRMGMT[$ToRIndex]=0
                exe_command=$exe_command"ifconfig eth0 down"
                ssh root@$IP $exe_command
                echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
            else
                ToRMGMT[$ToRIndex]=1
                exe_command=$exe_command"ifconfig eth0 up"
                ssh root@$IP $exe_command
                echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
            fi
        elif ([ $level -eq 2 ]) # 改变某个Leaf的网口状态
        then
            LeafIndex=$(($random%4)) # 选择要改变的Leaf
            EthIndex=$(($random%3)) # 选择要改变的网口
            IP=${LeafIP[$LeafIndex]} # 获得Leaf的IP
            if ([ ${LeafEth[$EthIndex]} == 'eth0' ]) # 改变eth0的状态
            then
                if ([ ${LeafMGMT[$LeafIndex]} -eq 1 ]) # 当前网口状态是up
                then 
                    LeafMGMT[$LeafIndex]=0
                    exe_command=$exe_command"ifconfig eth0 down"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                else
                    LeafMGMT[$LeafIndex]=1
                    exe_command=$exe_command"ifconfig eth0 up"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                fi
            else # 改变其他网口的状态
                exe_command=$exe_command"ifconfig "${LeafEth[$EthIndex]}" "
                if ([ ${LTLink[$LeafIndex*2+EthIndex-1]} -eq 1 ])
                then
                    LTLink[$LeafIndex*2+EthIndex-1]=0
                    exe_command=$exe_command"down"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                else
                    LTLink[$LeafIndex*2+EthIndex-1]=1
                    exe_command=$exe_command"up"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                fi
            fi
        else # 改变某个Spine的网口状态
            SpineIndex=$(($random%8)) # 选择要改变的Spine
            EthIndex=$(($random%3)) # 选择要改变的网口
            IP=${SpineIP[$SpineIndex]} # 获得Spine的IP
            if ([ ${SpineEth[$EthIndex]} == 'eth0' ]) # 改变eth0的状态
            then
                if ([ ${SpineMGMT[$SpineIndex]} -eq 1 ]) # 当前网口状态是up
                then 
                    SpineMGMT[$SpineIndex]=0
                    exe_command=$exe_command"ifconfig eth0 down"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                else
                    SpineMGMT[$SpineIndex]=1
                    exe_command=$exe_command"ifconfig eth0 up"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                fi
            else # 改变其他网口的状态
                exe_command=$exe_command"ifconfig "${SpineEth[$EthIndex]}" "
                if ([ ${SLLink[$SpineIndex*2+EthIndex-1]} -eq 1 ])
                then
                    SLLink[$SpineIndex*2+EthIndex-1]=0
                    exe_command=$exe_command"down"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                else
                    SLLink[$SpineIndex*2+EthIndex-1]=1
                    exe_command=$exe_command"up"
                    ssh root@$IP $exe_command
                    echo "ssh root@$IP $exe_command" >> /home/guolab/PrimusTest.log
                fi
            fi
        fi
        echo "当前链路状态 " >> /home/guolab/PrimusTest.log
        echo "SpineNode 下行链路: ${SLLink[*]}" >> /home/guolab/PrimusTest.log
        echo "SpineNode 直连链路: ${SpineMGMT[*]}" >> /home/guolab/PrimusTest.log
        echo "LeafNode  下行链路: ${LTLink[*]}" >> /home/guolab/PrimusTest.log
        echo "LeafNode  直连链路: ${LeafMGMT[*]}" >> /home/guolab/PrimusTest.log
        echo "ToRNode   直连链路: ${ToRMGMT[*]}" >> /home/guolab/PrimusTest.log
        echo "等待改变链路......" >> /home/guolab/PrimusTest.log
        echo " " >> /home/guolab/PrimusTest.log
        sleep 10
        cp /var/log/MasterLinkTable-0.0.txt /home/guolab/MasterLinkTable.txt
    done  
	# sleep 20
	# echo ""
	# echo "关闭iperf."
	# pssh -i -h /home/guolab/host/ATCServerhost.txt -t 0 "killall -9 iperf3;"
	# pssh -i -h /home/guolab/host/ATCClienthost.txt -t 0 "killall -9 iperf3;"

    # 确保所有网卡都已恢复
    # Spine
    ssh root@10.0.80.30 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.31 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.32 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.33 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.34 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.35 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.36 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    ssh root@10.0.80.37 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
    # Leaf
    ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
    ssh root@10.0.80.21 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
    ssh root@10.0.80.22 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
    ssh root@10.0.80.23 "ifconfig eth0 up;ifconfig eth5 up;ifconfig eth6 up;"
    # ToR
    ssh root@10.0.80.10 "ifconfig eth0 up;"
    ssh root@10.0.80.11 "ifconfig eth0 up;"
    ssh root@10.0.80.12 "ifconfig eth0 up;"
    ssh root@10.0.80.13 "ifconfig eth0 up;"

	sleep 10
	echo ""
	echo "关闭Primus."
	/home/guolab/script/stop.sh

	echo "休眠20s......" >> /home/guolab/PrimusTest.log
    sleep 20
    {
        echo "" >> /home/guolab/PrimusTest.log
        echo "" >> /home/guolab/PrimusTest.log
    }
done
echo "终止进程" >> /home/guolab/PrimusTest.log

############################################################
# # # iperf3 udp
# echo "Udp"
# pssh -i -H "root@10.0.8.86" -t 0 "killall -9 iperf3;rm -rf /home/guolab/output/*;"
# pssh -i -H "root@10.0.8.98" -t 0 "killall -9 iperf3;rm -rf /home/guolab/output/*;"
# pssh -i -H "root@10.0.8.87" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.89" -t 0 "killall -9 iperf3;"
# sleep 5
# echo "启动iperf-server."
# pssh -i -H "root@10.0.8.86" -t 0 "iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# pssh -i -H "root@10.0.8.98" -t 0 "iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# sleep 5
# echo "启动iperf-client."
# pssh -i -H "root@10.0.8.87" -t 0 "iperf3 -u -c 192.168.1.2 -i 0.5 -P 10 -t 50 -w 200K;" &
# pssh -i -H "root@10.0.8.89" -t 0 "iperf3 -u -c 192.168.1.3 -i 0.5 -P 10 -t 50 -w 200K;" &
# sleep 60
# echo "停止iperf."
# pssh -i -H "root@10.0.8.86" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.98" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.87" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.89" -t 0 "killall -9 iperf3;"

# screen 

# sudo su
# 123.com

# screen -rd

# vi /etc/sysctl.conf

# sysctl -p

# # spine
# net.ipv4.conf.eth1.rp_filter=0
# net.ipv4.conf.eth2.rp_filter=0
# net.ipv4.conf.lo.rp_filter=0
# net.ipv4.conf.all.rp_filter=0
# net.ipv4.conf.default.rp_filter=0

# # leaf
# net.ipv4.conf.eth1.rp_filter=0
# net.ipv4.conf.eth2.rp_filter=0
# net.ipv4.conf.eth3.rp_filter=0
# net.ipv4.conf.eth4.rp_filter=0
# net.ipv4.conf.eth5.rp_filter=0
# net.ipv4.conf.all.rp_filter=0
# net.ipv4.conf.default.rp_filter=0

# # tor
# net.ipv4.conf.eth1.rp_filter=0
# net.ipv4.conf.eth2.rp_filter=0
# net.ipv4.conf.eth3.rp_filter=0
# net.ipv4.conf.all.rp_filter=0
# net.ipv4.conf.default.rp_filter=0

# # server/client
# net.ipv4.conf.eth1.rp_filter=0
# net.ipv4.conf.all.rp_filter=0
# net.ipv4.conf.default.rp_filter=0

# # Spine
# ssh root@10.0.80.30 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.31 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.32 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.33 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.34 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.35 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.36 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.37 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# # # Leaf
# ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.21 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.22 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.23 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# # # ToR
# ssh root@10.0.80.10 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.12 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"

