#!/bin/bash
pssh -i -h /home/guolab/host/ATChost.txt "sysctl net.ipv4.fib_multipath_hash_policy=0;"
checkInterval=30000 #ms
sendInterval=1000
packetNum=43
echo "start primus"
/home/guolab/script/start.sh
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
echo "start change link"
ssh root@10.0.80.20 "ifconfig eth0 down;ifconfig eth1 down;ifconfig eth2 down;ifconfig eth3 down;ifconfig eth4 down;ifconfig eth5 down;"
ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# killall -9 Primus
# ifconfig eth0 down
# ifconfig eth1 down
# ifconfig eth2 down
# ssh root@10.0.80.20 "ifconfig eth5 down;"
# ssh root@10.0.80.10 "ifconfig eth1 down;"
# sleep 2
# ssh root@10.0.80.20 "ifconfig eth5 up;"
# # ssh root@10.0.80.10 "ifconfig eth1 up;"
# ssh root@10.0.80.21 "ifconfig eth5 down;"
# # ssh root@10.0.80.10 "ifconfig eth2 down;"
sleep 10
echo "stop process"
ssh root@10.0.80.50 "killall udp-client;"
ssh root@10.0.80.40 "killall udp-server;"
/home/guolab/script/stop.sh
rm /home/guolab/output/udpServerRecord-NSDI-primus.txt
scp root@10.0.80.40:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-NSDI-primus.txt