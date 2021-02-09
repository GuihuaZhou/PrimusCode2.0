########################## 参数部分 ##########################
rootDirectory="/home/user"
gitDirectory="/home/user/PrimusCode2.0"
spineNodes=8
leafNodes=2
torNodes=2
nPods=2
print_message=0
print_master_recv_all_LRs_time=0
print_node_modify_time=0
print_node_recv_RS_time=0
master_test=0
node_test=0
mgmt_interface="eth0"
killall -9 Primus
# 确保所有网卡都是正常的
# Master
ifconfig eth0 up
ifconfig eth1 up
ifconfig eth2 up
pssh -i -h $gitDirectory/host/master.txt "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# # Spine
# ssh root@10.0.80.30 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.31 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.32 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.33 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.34 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.35 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.36 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.37 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# Leaf
ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
ssh root@10.0.80.21 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.22 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.23 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ToR
ssh root@10.0.80.10 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.11 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# # 删除一些日志文件
rm /var/log/LinkTable*.txt;
rm /var/log/PathTable*.txt;
rm /var/log/NodeSockTable*.txt;
rm /var/log/ControllerSockTable*.txt;
rm /var/log/primusLog*.txt;
rm /var/log/CostTime*.txt;
rm /var/log/NeighborTable*.txt;
rm /var/log/Primus/Primus;
rm /var/log/switch.stdout;
rm /var/log/switch.stderr;
rm /var/log/PacketType*;
# master先编译
pssh -i -h $gitDirectory/host/master.txt "killall -9 Primus;rm /var/log/LinkTable*.txt;rm /var/log/PathTable*.txt;rm /var/log/NodeSockTable*.txt;rm /var/log/NeighborTable*.txt;
rm /var/log/ControllerSockTable*.txt;rm /var/log/primusLog*.txt;rm /var/log/CostTime*.txt;rm /var/log/switch.stdout;rm /var/log/switch.stderr;rm /var/log/PacketType*;"
pssh -i -h $gitDirectory/host/node.txt "killall -9 Primus;rm /var/log/LinkTable*.txt;rm /var/log/PathTable*.txt;rm /var/log/NodeSockTable*.txt;rm /var/log/NeighborTable*.txt;
rm /var/log/ControllerSockTable*.txt;rm /var/log/primusLog*.txt;rm /var/log/CostTime*.txt;rm /var/log/switch.stdout;rm /var/log/switch.stderr;rm /var/log/PacketType*;"
# 编译
rm $gitDirectory/Primus/Primus
cd $gitDirectory/Primus
g++ -std=c++0x init.cpp primus.cpp -o Primus -lpthread
cd ..
# # 传输
echo "transport"
pscp -h $gitDirectory/host/master.txt -l root $gitDirectory/Primus/Primus $rootDirectory/Primus
pscp -h $gitDirectory/host/node.txt -l root $gitDirectory/Primus/Primus $rootDirectory/Primus
# # 启动
tempCommand=''
# 1> 与>等价
# 输出log
tempCommand=$tempCommand" "$torNodes" "$leafNodes" "$spineNodes" "$nPods" "$print_message" "$print_master_recv_all_LRs_time" "$print_node_modify_time" "$print_node_recv_RS_time" "$master_test" "$node_test" $mgmt_interface 1>$rootDirectory/switch.stdout 2>$rootDirectory/switch.stderr"
# tempCommand=$tempCommand" "$torNodes" "$leafNodes" "$spineNodes" "$nPods
# 
command=''
command=$command"./Primus/Primus"$tempCommand
echo "Master"
$command &
sleep 3
command=''
command=$command"pssh -t 0 -i -h "$gitDirectory"/host/master.txt "$rootDirectory"/Primus"$tempCommand
$command &
echo "Node"
command=''
command=$command"pssh -t 0 -i -h "$gitDirectory"/host/node.txt "$rootDirectory"/Primus"$tempCommand
$command &
