#!/bin/bash
# 利用udp吞吐率来测试收敛时间
pssh -i -h /home/guolab/host/ATChost.txt "sysctl net.ipv4.fib_multipath_hash_policy=1;"
checkInterval=30000 #ms
sendInterval=1000
packetNum=43
########################### BGP ###############################
echo "start bgp"
/home/guolab/script/stop.sh
pssh -i -h /home/guolab/host/ATChost.txt "ifup -a;"
pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/host/ATChost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /home/guolab/bgpd.log;"
# #
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
ssh root@10.0.80.20 "ifconfig eth5 down;"
ssh root@10.0.80.10 "ifconfig eth1 down;"
sleep 2
ssh root@10.0.80.20 "ifconfig eth5 up;"
ssh root@10.0.80.10 "ifconfig eth1 up;"
ssh root@10.0.80.21 "ifconfig eth5 down;"
ssh root@10.0.80.10 "ifconfig eth2 down;"
sleep 10
echo "stop process"
ssh root@10.0.80.50 "killall udp-client;"
ssh root@10.0.80.40 "killall udp-server;"
/home/guolab/script/stop.sh
rm /home/guolab/output/udpServerRecord-NSDI-bgp.txt
scp root@10.0.80.40:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-NSDI-bgp.txt
# # ########################### BGP ###############################
# sleep 5
# # ########################### Primus normal###############################
# echo "start primus"
# /home/guolab/script/start.sh
# #
# ssh root@10.0.80.40 "killall udp-server;rm /home/guolab/udpServerRecord.txt;"
# ssh root@10.0.80.50 "killall udp-client;rm /home/guolab/udpClientRecord.txt;"
# sleep 10
# echo "start udp server"
# ssh root@10.0.80.40 "/home/guolab/udp-server "$checkInterval" /home/guolab/udpServerRecord.txt;" &
# sleep 2
# echo "start udp client"
# ssh root@10.0.80.50 "/home/guolab/udp-client 192.168.1.2 "$sendInterval" "$packetNum" /home/guolab/udpClientRecord.txt;" &
# sleep 2
# echo "start change link"
# ssh root@10.0.80.20 "ifconfig eth5 down;"
# # ssh root@10.0.80.10 "ifconfig eth1 down;"
# sleep 2
# ssh root@10.0.80.20 "ifconfig eth5 up;"
# # ssh root@10.0.80.10 "ifconfig eth1 up;"
# ssh root@10.0.80.21 "ifconfig eth5 down;"
# # ssh root@10.0.80.10 "ifconfig eth2 down;"
# sleep 10
# echo "stop process"
# ssh root@10.0.80.50 "killall udp-client;"
# ssh root@10.0.80.40 "killall udp-server;"
# /home/guolab/script/stop.sh
# rm /home/guolab/output/udpServerRecord-NSDI-primus.txt
# scp root@10.0.80.40:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-NSDI-primus.txt
# ########################### Primus normal###############################
# # 
# # ########################### Primus control link###############################
# echo "start primus"
# /home/guolab/script/start.sh
# #
# ssh root@10.0.80.40 "killall udp-server;rm /home/guolab/udpServerRecord.txt;"
# ssh root@10.0.80.50 "killall udp-client;rm /home/guolab/udpClientRecord.txt;"
# sleep 10
# echo "start udp server"
# ssh root@10.0.80.40 "/home/guolab/udp-server "$checkInterval" /home/guolab/udpServerRecord.txt;" &
# sleep 2
# echo "start udp client"
# ssh root@10.0.80.50 "/home/guolab/udp-client 192.168.1.2 "$sendInterval" "$packetNum" /home/guolab/udpClientRecord.txt;" &
# sleep 2
# echo "start change link"
# ssh root@10.0.80.20 "ifconfig eth0 down;ifconfig eth5 down;ifconfig eth0 up;"
# # ssh root@10.0.80.10 "ifconfig eth1 down;"
# sleep 2
# ssh root@10.0.80.20 "ifconfig eth5 up;"
# # ssh root@10.0.80.10 "ifconfig eth1 up;"
# ssh root@10.0.80.21 "ifconfig eth0 down;ifconfig eth5 down;ifconfig eth0 up;"
# # ssh root@10.0.80.10 "ifconfig eth2 down;"
# sleep 10
# echo "stop process"
# ssh root@10.0.80.50 "killall udp-client;"
# ssh root@10.0.80.40 "killall udp-server;"
# /home/guolab/script/stop.sh
# rm /home/guolab/output/udpServerRecord-NSDI-primus.txt
# scp root@10.0.80.40:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-NSDI-primus.txt
# ########################### Primus control link###############################
# 
# ########################### Primus master###############################
# echo "start primus"
# /home/guolab/script/start.sh
# #
# ssh root@10.0.80.40 "killall udp-server;rm /home/guolab/udpServerRecord.txt;"
# ssh root@10.0.80.50 "killall udp-client;rm /home/guolab/udpClientRecord.txt;"
# sleep 10
# echo "start udp server"
# ssh root@10.0.80.40 "/home/guolab/udp-server "$checkInterval" /home/guolab/udpServerRecord.txt;" &
# sleep 2
# echo "start udp client"
# ssh root@10.0.80.50 "/home/guolab/udp-client 192.168.1.2 "$sendInterval" "$packetNum" /home/guolab/udpClientRecord.txt;" &
# sleep 2
# echo "start change link"
# ifconfig eth0 down
# ifconfig eth1 down
# ifconfig eth2 down
# ssh root@10.0.80.20 "ifconfig eth5 down;"
# # ssh root@10.0.80.10 "ifconfig eth1 down;"
# sleep 2
# # ifconfig eth0 up
# # ifconfig eth1 up
# # ifconfig eth2 up
# # ssh root@10.0.80.20 "ifconfig eth5 up;"
# # # ssh root@10.0.80.10 "ifconfig eth1 up;"
# # ssh root@10.0.80.21 "ifconfig eth5 down;"
# # # ssh root@10.0.80.10 "ifconfig eth2 down;"
# sleep 10
# echo "stop process"
# ssh root@10.0.80.50 "killall udp-client;"
# ssh root@10.0.80.40 "killall udp-server;"
# /home/guolab/script/stop.sh
# rm /home/guolab/output/udpServerRecord-NSDI-primus.txt
# scp root@10.0.80.40:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-NSDI-primus.txt
########################### Primus master###############################

# scp root@10.0.80.20:/usr/local/sbin/bgpd /home/guolab/bgp/bgpd
# scp root@10.0.80.20:/usr/local/sbin/zebra /home/guolab/bgp/zebra

# pssh -i -h /home/guolab/host/ATCClienthost.txt "rm /home/guolab/"
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/bgp/quagga-1.2.4.tar.gz /home/guolab/
# pssh -t 0 -i -h /home/guolab/host/ATChost.txt "tar zxvf /home/guolab/quagga-1.2.4.tar.gz;"
# pssh -i -h /home/guolab/host/ATChost.txt "cp /usr/local/lib/libzebra.so.1 /lib;"
# pssh -i -h /home/guolab/host/ATChost.txt "rm /usr/local/sbin/bgpd;rm /usr/local/sbin/zebra;"
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/bgp/bgpd /usr/local/sbin/bgpd
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/bgp/zebra /usr/local/sbin/zebra
# pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /usr/local/sbin/bgpd;"

# scp root@10.0.80.22:/usr/local/sbin/bgpd /home/guolab/bgp/bgpd 
# scp root@10.0.80.22:/usr/local/sbin/zebra /home/guolab/bgp/zebra 

# sudo su
# 123.com

# tar zxvf quagga-1.2.4.tar.gz

# cd quagga-1.2.4
# ./configure --enable-vtysh --enable-user=root --enable-group=root --enable-multipath=64
# make
# sudo make install

# ssh root@10.0.80.40 "rm /home/guolab/tcp-server;"
# pssh -i -h /home/guolab/host/ATCClienthost.txt "rm /home/guolab/tcp-client;"
# cd tool
# rm tcp-server
# rm tcp-client
# g++ tcp-server.c -o tcp-server -pthread -std=c++11
# g++ tcp-client.c -o tcp-client -pthread -std=c++11
# cd ..
# scp /home/guolab/tool/tcp-server root@10.0.80.40:/home/guolab/tcp-server
# pscp -h /home/guolab/host/ATCClienthost.txt -l root /home/guolab/tool/tcp-client /home/guolab/tcp-client