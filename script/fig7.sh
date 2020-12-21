#!/bin/bash
# 说明10.0.80.40和10.0.80.50是server和client的IP
directory="/home/guolab"
# 利用udp吞吐率来测试收敛时间
pssh -i -h $directory/host/node.txt "sysctl net.ipv4.fib_multipath_hash_policy=1;"
checkInterval=30000 #ms
sendInterval=1000
packetNum=43
# # ########################### Primus data link###############################
echo "start primus"
$directory/script/start.sh
#
ssh root@10.0.80.40 "killall udp-server;rm /home/guolab/udpServerRecord.txt;"
ssh root@10.0.80.50 "killall udp-client;rm /home/guolab/udpClientRecord.txt;"
sleep 10
echo "start udp server"
ssh root@10.0.80.40 "/home/guolab/udp-server "$checkInterval" /home/guolab/udpServerRecord.txt;" &
sleep 2
echo "start udp client"
ssh root@10.0.80.50 "/home/guolab/udp-client 192.168.1.2 "$sendInterval" "$packetNum" /home/guolab/udpClientRecord.txt;" &
sleep 2
ifconfig eth0 down;# 模拟leader down
ssh root@10.0.1.68 "ifconfig eth0 down;" # 模拟slave down
ssh root@10.0.80.21 "ifconfig eth0 down;" # 模拟control link down
# 2.1--1.1 down
echo "start change link"
ssh root@10.0.80.21 "ifconfig eth5 down;"
sleep 20
echo "stop process"
ssh root@10.0.80.50 "killall udp-client;"
ssh root@10.0.80.40 "killall udp-server;"
$directory/script/stop.sh
scp root@10.0.80.40:/home/guolab/udpServerRecord.txt /home/guolab/udpServerRecord-primus.txt
# ########################### Primus data link###############################