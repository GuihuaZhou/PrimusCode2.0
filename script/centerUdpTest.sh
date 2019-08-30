# 准备实验环境，确保不会启动重复的进程
pssh -i -h /home/guolab/PCHost.txt "killall udp-client-my;"
pssh -i -H "root@10.0.8.86" "killall udp-server-my;"
killall -9 CentralizedRoute
pssh -i -h /home/guolab/host.txt "killall -9 CentralizedRoute;"

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

# 启动node
# rm -rf /home/guolab/output/centerTest.log
#pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/output/CenterOutput.txt;"
# pssh -i -H "root@10.0.8.86" -t 0 "chmod 777 -R /home/guolab/output;"

/home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 1 4 16 2 &
pssh -i -h /home/guolab/host.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun;" & # 启动Node上的集中式


# 启动PC
sleep 17
{
	echo "启动Server" >> /home/guolab/output/centerTest.log
    pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/udp-server-my 100 /home/guolab/output/udpResults.txt  /home/guolab/output/udpThroughput.txt" &
	echo "启动Client" >> /home/guolab/output/centerTest.log
	pssh -i -h /home/guolab/PCHost.txt -t 0 "/home/guolab/udp-client-my 192.168.1.2 100 100 /home/guolab/output/udpResult.txt;" &
	
}
sleep 2
{
    echo "等待改变链路......" >> /home/guolab/output/centerTest.log
}

index=0
echo "开始改变链路......" >> /home/guolab/output/centerTest.log
echo "" >> /home/guolab/output/centerTest.log
ExecuteCommand $index;
echo "当前链路状态 ${ToRNode[*]}" >> /home/guolab/output/centerTest.log
echo "可用链路数 $Available" >> /home/guolab/output/centerTest.log
echo "等待改变链路......" >> /home/guolab/output/centerTest.log
#sleep 3
#{
#	echo "开始改变链路......" >> /home/guolab/output/centerTest.log
#	ExecuteCommand $index;
#	echo " " >> /home/guolab/output/centerTest.log
#}


sleep 30
{
	echo "终止进程" >> /home/guolab/output/centerTest.log
	pssh -i -h /home/guolab/PCHost.txt "killall udp-client-my;"
	pssh -i -H "root@10.0.8.86" "killall udp-server-my;"
	killall -9 CentralizedRoute
	pssh -i -h /home/guolab/host.txt "killall -9 CentralizedRoute;"
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
echo "休眠20s......" >> /home/guolab/output/centerTest.log
sleep 20
{
    echo "" >> /home/guolab/output/centerTest.log
    echo "" >> /home/guolab/output/centerTest.log
}

# sleep 180
# {
# 	pssh -i -h /home/guolab/host.txt "killall -9 CentralizedRoute;"
# }
# killall -9 CentralizedRoute && /home/guolab/CentralizedRouteTest/modifyRun

# ps -e|grep CentralizedRout
# route -n

#
# auto eth5
# iface eth5 inet static
#         address 172.16.1.1
#         netmask 255.255.255.0
# auto eth6
# iface eth6 inet static
#         address 172.16.2.1
#         netmask 255.255.255.0
# auto eth7
# iface eth7 inet static
#         address 172.16.3.1
#         netmask 255.255.255.0
# auto eth8
# iface eth8 inet static
#         address 172.16.4.1
#         netmask 255.255.255.0
# auto eth9
# iface eth9 inet static
#         address 172.16.5.1
#         netmask 255.255.255.0
# auto eth10
# iface eth10 inet static
#         address 172.16.6.1
#         netmask 255.255.255.0
# auto eth11
# iface eth11 inet static
#         address 172.16.7.1
#         netmask 255.255.255.0
# auto eth12
# iface eth12 inet static
#         address 172.16.8.1
#         netmask 255.255.255.0
