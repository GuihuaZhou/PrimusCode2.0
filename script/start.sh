##########################虚拟机测试部分##########################
spineNodes=8
leafNodes=2
torNodes=1
nPods=2
killall -9 Primus
killall -9 linkChange
pssh -i -h /home/guolab/host/ATChost.txt "killall -9 Primus;killall -9 linkChange;"
pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;killall -9 linkChange;"
# 确保所有网卡都是正常的
# Master
ifconfig eth0 up
ifconfig eth1 up
ifconfig eth2 up
pssh -i -h /home/guolab/host/master.txt "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
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
ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
ssh root@10.0.80.21 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
ssh root@10.0.80.22 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
ssh root@10.0.80.23 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ToR
ssh root@10.0.80.10 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.11 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# # 删除一些日志文件
rm /home/guolab/LinkTable*.txt;
rm /home/guolab/PathTable*.txt;
rm /home/guolab/NodeSockTable*.txt;
rm /home/guolab/ControllerSockTable*.txt;
rm /home/guolab/PrimusLog*.txt;
rm /home/guolab/CostTime*.txt;
rm /home/guolab/NeighborTable*.txt;
rm /home/guolab/Primus/Primus;
rm /home/guolab/switch.stdout;
rm /home/guolab/switch.stderr;
rm /home/guolab/PacketType*;
# master先编译
pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;rm /home/guolab/LinkTable*.txt;
rm /home/guolab/PathTable*.txt;rm /home/guolab/NodeSockTable*.txt;rm /home/guolab/NeighborTable*.txt;
rm /home/guolab/ControllerSockTable*.txt;rm /home/guolab/PrimusLog*.txt;
rm /home/guolab/CostTime*.txt;rm /home/guolab/Primus;rm /home/guolab/switch.stdout;rm /home/guolab/switch.stderr;
rm /home/guolab/PacketType*;"
pssh -i -h /home/guolab/host/ATChost.txt "killall -9 Primus;rm /home/guolab/LinkTable*.txt;
rm /home/guolab/PathTable*.txt;rm /home/guolab/NodeSockTable*.txt;rm /home/guolab/NeighborTable*.txt;
rm /home/guolab/ControllerSockTable*.txt;rm /home/guolab/PrimusLog*.txt;
rm /home/guolab/CostTime*.txt;rm /home/guolab/Primus;rm /home/guolab/switch.stdout;rm /home/guolab/switch.stderr;
rm /home/guolab/PacketType*;"
# 编译
rm /home/guolab/Primus/Primus
cd /home/guolab/Primus
# g++ -std=c++0x init.cpp primus.cpp -o Primus -lpthread
g++ -std=c++0x init.cpp primus.cpp Graph.cpp YenTopKShortestPathsAlg.cpp DijkstraShortestPathAlg.cpp -o Primus -lpthread
cd ..
# # 传输
echo "transport"
pscp -h /home/guolab/host/master.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus
pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus
# # 启动
tempCommand=''
# 1> 与>等价
# 输出log
tempCommand=$tempCommand" "$torNodes" "$leafNodes" "$spineNodes" "$nPods" 1>/home/guolab/switch.stdout 2>/home/guolab/switch.stderr"
# tempCommand=$tempCommand" "$torNodes" "$leafNodes" "$spineNodes" "$nPods
# 
command=''
command=$command"/home/guolab/Primus/Primus"$tempCommand
echo "Master"
$command &
sleep 3
command=''
command=$command"pssh -t 0 -i -h /home/guolab/host/master.txt /home/guolab/Primus"$tempCommand
$command &
echo "Node"
command=''
command=$command"pssh -t 0 -i -h /home/guolab/host/ATChost.txt /home/guolab/Primus"$tempCommand
$command &
########################## ##########################
# ./configure --enable-vtysh --enable-user=root --enable-group=root --enable-multipath=64

# taskset -c 0 ./kshortestpath 1 0 16 100 4 100 10000 1>/home/tencent/switch.stdout 2>/home/tencent/switch.stderr &