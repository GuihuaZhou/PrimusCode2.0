#!/bin/bash
########################### BGP ###############################
pssh -i -h /home/guolab/host/ATChost.txt "ifup -a;"
pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/host/ATChost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /home/guolab/bgpd.log;"

ssh root@10.0.80.40 "killall iperf3;rm /home/guolab/ATCOutput/iperfServerRecord.txt;"
ssh root@10.0.80.50 "killall iperf3;"
sleep 20
echo "start iperf server"
ssh root@10.0.80.40 "sudo iperf3 -s -i 0.5 > /home/guolab/ATCOutput/iperfServerRecord.txt;" &
sleep 3
echo "start iperf client"
ssh root@10.0.80.50 "sudo iperf3 -c 192.168.1.2 -i 0.5 -b 200M -t 25;" &
sleep 10
echo "start change link"
ssh root@10.0.80.20 "ifconfig eth1 down;"
ssh root@10.0.80.30 "ifconfig eth1 down;"
sleep 30
echo "stop process"
pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
ssh root@10.0.80.40 "killall iperf3;"
ssh root@10.0.80.50 "killall iperf3;"
ssh root@10.0.80.20 "ifconfig eth1 up;"
ssh root@10.0.80.30 "ifconfig eth1 up;"
########################### BGP ###############################