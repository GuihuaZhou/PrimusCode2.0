#!/bin/bash
########################### BGP ###############################
pssh -i -h /home/guolab/host/ATChost.txt "ifup -a;"
pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/host/ATChost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /home/guolab/bgpd.log;"

ssh root@10.0.80.40 "killall udp-server;rm /home/guolab/ATCOutput/udpServerRecord.txt;"
ssh root@10.0.80.50 "killall udp-client;rm /home/guolab/ATCOutput/udpClientRecord.txt;"
sleep 10
echo "start udp server"
ssh root@10.0.80.40 "/home/guolab/udp-server 10000 /home/guolab/ATCOutput/udpServerRecord.txt;" &
sleep 2
echo "start udp client"
ssh root@10.0.80.50 "/home/guolab/udp-client 192.168.1.2 50 /home/guolab/ATCOutput/udpClientRecord.txt;" &
# sleep 10
# echo "start change link"
# ssh root@10.0.80.20 "ifconfig eth1 down;"
# ssh root@10.0.80.30 "ifconfig eth1 down;"
sleep 10
echo "stop process"
pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
ssh root@10.0.80.40 "killall udp-server;"
ssh root@10.0.80.50 "killall udp-client;"
# ssh root@10.0.80.20 "ifconfig eth1 up;"
# ssh root@10.0.80.30 "ifconfig eth1 up;"
########################### BGP ###############################