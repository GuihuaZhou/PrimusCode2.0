# 修改BGP配置文件，adv interval
# pssh -i -h /home/guolab/WCMPHost.txt "/home/guolab/modifyBGPConf 0 2 4 16 2;"
# pssh -i -h /home/guolab/WCMPHost.txt "reboot;"
# pssh -i -h /home/guolab/WCMPHost.txt "echo \'Hello\';"
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /home/guolab/output/updateBGPRIBTimeRecord.txt;rm /home/guolab/output/updateKernelTimeRecord.txt;"
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /usr/local/sbin/bgpd;rm /usr/local/sbin/zebra;"
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/bgpd/bgpd-2900 /usr/local/sbin/bgpd
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/bgpd/zebra /usr/local/sbin/zebra
# sudo pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /boot/config-4.12.14*; rm /boot/initrd.img-4.12.14*; rm /boot/System.map-4.12.14*; rm /boot/vmlinuz-4.12.14*;"
# sudo pssh -i -h /home/guolab/WCMPHost.txt -t 0 "cd linux-4.12.14; make modules_install;"
# sudo pssh -i -h /home/guolab/WCMPHost.txt -t 0 "cd linux-4.12.14; make install;"
# sudo update-grub
# sudo vi /etc/default/grub
# GRUB_TIMEOUT_STYLE=menu

# iperf -c 192.168.3.2 -t 20 -P 100 -w 200K

# g++ iperfMaxMinAnalysis.cc -o iperfMaxMinAnalysis
# rm /home/guolab/output/MaxMinAnalysis.txt
# rm /home/guolab/output/temp.txt
# /home/guolab/iperfMaxMinAnalysis 0.5
# chmod 777 -R /home/guolab/output/*

# pssh -i -H "root@192.168.80.13" -t 0 "sudo route add -net 32.0.0.0/8 gw 192.168.1.1;sudo route add -net 21.0.0.0/8 gw 192.168.1.1;sudo route add -net 192.168.3.0/24 gw 192.168.1.1;sudo route add -net 192.168.4.0/24 gw 192.168.1.1;"
# pssh -i -H "root@192.168.80.14" -t 0 "sudo route add -net 32.0.0.0/8 gw 192.168.2.1;sudo route add -net 21.0.0.0/8 gw 192.168.2.1;sudo route add -net 192.168.3.0/24 gw 192.168.2.1;sudo route add -net 192.168.4.0/24 gw 192.168.2.1;"
# pssh -i -H "root@192.168.80.15" -t 0 "sudo route add -net 32.0.0.0/8 gw 192.168.3.1;sudo route add -net 21.0.0.0/8 gw 192.168.3.1;sudo route add -net 192.168.1.0/24 gw 192.168.3.1;sudo route add -net 192.168.2.0/24 gw 192.168.3.1;"
# pssh -i -H "root@192.168.80.16" -t 0 "sudo route add -net 32.0.0.0/8 gw 192.168.4.1;sudo route add -net 21.0.0.0/8 gw 192.168.4.1;sudo route add -net 192.168.1.0/24 gw 192.168.4.1;sudo route add -net 192.168.2.0/24 gw 192.168.4.1;"

# killall -9 CentralizedRoute
# pssh -i -h /home/guolab/WCMPHost.txt "killall -9 CentralizedRoute;"
# pssh -i -h /home/guolab/WCMPHost.txt "rm /home/guolab/CentralizedRouteTest/*;"
# cd /home/guolab/CentralizedRouteTest
# ./route.sh
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/CentralizedRouteTest/CentralizedRoute /home/guolab/CentralizedRouteTest/
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /home/guolab/output/*;"
# rm -rf /home/guolab/output/center.log
# cd ..

# /home/guolab/CentralizedRouteTest/CentralizedRoute 192.168.80.0 6666 0 0 2 4 16 2 100000 &
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 192.168.80.0 6666 2 4 16 2 100000;" & 
# pssh -i -h /home/guolab/WCMPHost.txt "chmod 777 -R /home/guolab/output/;"

# pscp -H "root@192.168.80.5" -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
# pscp -H "root@192.168.80.7" -l root /home/guolab/linkchange-server /home/guolab/
# pscp -H "root@192.168.80.1" -l root /home/guolab/linkchange-client /home/guolab/

# scp root@192.168.80.6:/home/guolab/linkchange-server /home/guolab/linkchange-server


sudo pssh -i -h /home/guolab/WCMPHost.txt -t 0 "sysctl net.ipv4.fib_multipath_hash_policy=1;"

# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /usr/local/sbin/bgpd"
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/bgpd/bgpd /usr/local/sbin/bgpd

### BGP Script begin  =================


pssh -i -h /home/guolab/WCMPHost.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/WCMPHost.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/WCMPHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/WCMPHost.txt "chmod 777 -R /home/guolab/output/;"
pssh -i -H "root@192.168.80.13" -t 0 "killall iperf3;"
pssh -i -H "root@192.168.80.14" -t 0 "killall iperf3;"
pssh -i -H "root@192.168.80.15" -t 0 "killall iperf3;rm /home/guolab/output/serverRecord*.txt;"
pssh -i -H "root@192.168.80.16" -t 0 "killall iperf3;rm /home/guolab/output/serverRecord*.txt;"
pssh -i -H "root@192.168.80.7" -t 0 "killall linkchange-server;"
pssh -i -H "root@192.168.80.1" -t 0 "killall linkchange-client;"
sleep 10
echo "启动iperf-server"
pssh -i -H "root@192.168.80.15" -t 0 "sudo iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
pssh -i -H "root@192.168.80.16" -t 0 "sudo iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
sleep 20
echo "启动iperf-client"
pssh -i -H "root@192.168.80.13" -t 0 "sudo iperf3 -c 192.168.3.2 -t 50 -P 120 -w 200K;" &
pssh -i -H "root@192.168.80.14" -t 0 "sudo iperf3 -c 192.168.4.2 -t 50 -P 120 -w 200K;" &

sleep 20
pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 down;"
pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 down;"

# sleep 5
# echo "启动linkchange-server"
# pssh -i -H "root@192.168.80.7" -t 0 "/home/guolab/linkchange-server 667;" &
# sleep 2
# echo "启动linkchange-client"
# pssh -i -H "root@192.168.80.1" -t 0 "/home/guolab/linkchange-client bgp 192.168.80.7 667 1 30000000;" &

sleep 100
pssh -i -H "root@192.168.80.13" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
pssh -i -H "root@192.168.80.14" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
pssh -i -H "root@192.168.80.15" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
pssh -i -H "root@192.168.80.16" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
pssh -i -H "root@192.168.80.7" -t 0 "killall linkchange-server;"
pssh -i -H "root@192.168.80.1" -t 0 "killall linkchange-client;"

rm /home/guolab/output/serverRecord*.txt
rm /home/guolab/output/MaxMinAnalysis.txt
rm /home/guolab/output/temp.txt
scp root@192.168.80.15:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
# scp root@192.168.80.16:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
/home/guolab/iperfMaxMinAnalysis 0.5
chmod 777 -R /home/guolab/output/*
# pssh -i -h /home/guolab/WCMPHost.txt "killall zebra; killall bgpd;"
pssh -i -H "root@192.168.80.1" -t 0 "ifconfig eth1 up;"
pssh -i -H "root@192.168.80.5" -t 0 "ifconfig eth3 up;"
pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 up;"
pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 up;"


### BGP Script end  =================


# # # #### Centralized Script begin  =================



# killall -9 CentralizedRoute
# pssh -i -h /home/guolab/WCMPHost.txt "killall -9 CentralizedRoute;"
# rm -rf /home/guolab/output/center.log
# /home/guolab/CentralizedRouteTest/CentralizedRoute 192.168.80.0 6666 0 0 2 4 16 2 1000000 &
# sleep 3
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 192.168.80.0 6666 2 4 16 2 1000000;" & 

# pssh -i -h /home/guolab/WCMPHost.txt "chmod 777 -R /home/guolab/output/;"

# pssh -i -H "root@192.168.80.13" -t 0 "killall iperf3;"
# pssh -i -H "root@192.168.80.14" -t 0 "killall iperf3;"
# pssh -i -H "root@192.168.80.15" -t 0 "killall iperf3;rm /home/guolab/output/serverRecord*.txt;"
# pssh -i -H "root@192.168.80.16" -t 0 "killall iperf3;rm /home/guolab/output/serverRecord*.txt;"
# pssh -i -H "root@192.168.80.7" -t 0 "killall linkchange-server;"
# pssh -i -H "root@192.168.80.1" -t 0 "killall linkchange-client;"
# sleep 10
# echo "启动iperf-server"
# pssh -i -H "root@192.168.80.15" -t 0 "sudo iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# # pssh -i -H "root@192.168.80.16" -t 0 "sudo iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# sleep 20
# echo "启动iperf-client"
# pssh -i -H "root@192.168.80.13" -t 0 "sudo iperf3 -c 192.168.3.2 -t 50 -P 120 -w 200K;" &
# # pssh -i -H "root@192.168.80.14" -t 0 "sudo iperf3 -c 192.168.4.2 -t 50 -P 120 -w 200K;" &

# #######  Application begin
# # sleep 20
# # ###pssh -i -H "root@192.168.80.1" -t 0 "ifconfig eth1 down;"
# # pssh -i -H "root@192.168.80.5" -t 0 "ifconfig eth3 down;"
# # pssh -i -H "root@192.168.80.9" -t 0 "ip route replace 192.168.3.0/24 nexthop via 21.4.1.0 dev eth4 weight 1;" & 
# # pssh -i -H "root@192.168.80.10" -t 0 "ip route replace 192.168.4.0/24 nexthop via 21.3.2.0 dev eth3 weight 2 nexthop via 21.4.2.0 dev eth4 weight 2;" & 
# #######  Application end

# sleep 20
# # pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 down;"
# pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 down;"

# # sleep 5
# # echo "启动linkchange-server"
# # pssh -i -H "root@192.168.80.7" -t 0 "/home/guolab/linkchange-server 667;" &
# # sleep 2
# # echo "启动linkchange-client"
# # pssh -i -H "root@192.168.80.1" -t 0 "/home/guolab/linkchange-client center 192.168.80.7 667 1 30000000;" &

# sleep 100
# pssh -i -H "root@192.168.80.13" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
# pssh -i -H "root@192.168.80.14" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
# pssh -i -H "root@192.168.80.15" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
# pssh -i -H "root@192.168.80.16" -t 0 "killall iperf3;chmod 777 -R /home/guolab/output/;"
# pssh -i -H "root@192.168.80.7" -t 0 "killall linkchange-server;"
# pssh -i -H "root@192.168.80.1" -t 0 "killall linkchange-client;"

# rm /home/guolab/output/serverRecord*.txt
# rm /home/guolab/output/MaxMinAnalysis.txt
# rm /home/guolab/output/temp.txt
# scp root@192.168.80.15:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
# # scp root@192.168.80.16:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
# /home/guolab/iperfMaxMinAnalysis 0.5
# chmod 777 -R /home/guolab/output/*
# pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 up;"
# pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 up;"
# pssh -i -H "root@192.168.80.5" -t 0 "ifconfig eth3 up;"
# sleep 2
# killall -9 CentralizedRoute
# pssh -i -h /home/guolab/WCMPHost.txt "killall -9 CentralizedRoute;"



# # # #### Centralized Script end  =================


# # BGP ##########################
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /usr/local/sbin/bgpd"
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/bgpd/bgpd /usr/local/sbin/bgpd


# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /usr/local/sbin/bgpd"
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/bgpd/bgpd-delay /usr/local/sbin/bgpd

# pssh -i -h /home/guolab/WCMPHost.txt "killall zebra; killall bgpd;"
# pssh -i -h /home/guolab/WCMPHost.txt "rm /home/guolab/output/*;"
# pssh -i -h /home/guolab/WCMPHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
# pssh -i -h /home/guolab/WCMPHost.txt "chmod 777 -R /home/guolab/output/;"

# sleep 20
# pssh -i -H "root@192.168.80.15" -t 0 "rm /home/guolab/output/serverRecord.txt;"
# pssh -i -H "root@192.168.80.15" -t 0 "/home/guolab/udp-server-my 767;" &
# sleep 5
# pssh -i -H "root@192.168.80.13" -t 0 "/home/guolab/udp-client-my 192.168.3.2 767 1000;" &

# sleep 10
# pssh -i -H "root@192.168.80.8" -t 0 "ifconfig eth5 down;"
# sleep 5
# pssh -i -H "root@192.168.80.8" -t 0 "ifconfig eth5 up;"
# sleep 1
# # sleep 5
# # pssh -i -H "root@192.168.80.1" -t 0 "ifconfig eth2 down;"
# # sleep 3
# # pssh -i -H "root@192.168.80.3" -t 0 "ifconfig eth2 down;"
# # pssh -i -H "root@192.168.80.1" -t 0 "ifconfig eth2 up;"
# # sleep 3
# # pssh -i -H "root@192.168.80.4" -t 0 "ifconfig eth2 down;"
# # pssh -i -H "root@192.168.80.3" -t 0 "ifconfig eth2 up;"
# # sleep 3
# # pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 down;"
# # pssh -i -H "root@192.168.80.4" -t 0 "ifconfig eth2 up;"
# # sleep 5
# # pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 up;"

# pssh -i -H "root@192.168.80.13" -t 0 "killall udp-client-my;"
# pssh -i -H "root@192.168.80.15" -t 0 "killall udp-server-my;" 
# rm /home/guolab/output/serverRecord.txt
# rm /home/guolab/output/udpInterval.txt
# rm /home/guolab/output/serverRecord.txt.tr
# scp root@192.168.80.15:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
# rm /home/guolab/output/udpInterval.txt
# ./udpIntervalAnalysis
# ./plot_throughput.pl /home/guolab/output/serverRecord.txt 0.005

# # rm /home/guolab/output/throughputRatio.txt
# # ./udpThroughputAnalysis 1 0.005
# 

# # center
# killall -9 CentralizedRoute
# pssh -i -h /home/guolab/WCMPHost.txt "killall -9 CentralizedRoute;"
# rm -rf /home/guolab/output/center.log
# /home/guolab/CentralizedRouteTest/CentralizedRoute 192.168.80.0 6666 0 0 2 4 16 2 1000000 &
# sleep 3
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 192.168.80.0 6666 2 4 16 2 1000000;" & 
# pssh -i -h /home/guolab/WCMPHost.txt "chmod 777 -R /home/guolab/output/;"

# sleep 10
# pssh -i -H "root@192.168.80.15" -t 0 "rm /home/guolab/output/serverRecord.txt;"
# pssh -i -H "root@192.168.80.15" -t 0 "/home/guolab/udp-server-my 767;" &
# sleep 5
# pssh -i -H "root@192.168.80.13" -t 0 "/home/guolab/udp-client-my 192.168.3.2 767 1000;" &

# sleep 10
# pssh -i -H "root@192.168.80.1" -t 0 "ifconfig eth2 down;"
# sleep 5
# pssh -i -H "root@192.168.80.1" -t 0 "ifconfig eth2 up;"
# sleep 1
# # sleep 5
# # pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth3 down;"
# # sleep 3
# # pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth3 up;ifconfig eth4 down;"
# # sleep 3
# # pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 up;"
# # pssh -i -H "root@192.168.80.8" -t 0 "ifconfig eth3 down;"
# # sleep 3
# # pssh -i -H "root@192.168.80.8" -t 0 "ifconfig eth3 up;ifconfig eth4 down;"
# # sleep 5
# # pssh -i -H "root@192.168.80.8" -t 0 "ifconfig eth4 up;"

# pssh -i -H "root@192.168.80.13" -t 0 "killall udp-client-my;"
# pssh -i -H "root@192.168.80.15" -t 0 "killall udp-server-my;" 
# rm /home/guolab/output/serverRecord.txt
# rm /home/guolab/output/udpInterval.txt
# rm /home/guolab/output/serverRecord.txt.tr
# scp root@192.168.80.15:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
# rm /home/guolab/output/udpInterval.txt
# ./udpIntervalAnalysis
# ./plot_throughput.pl /home/guolab/output/serverRecord.txt 0.005
# # # rm /home/guolab/output/throughputRatio.txt
# # # ./udpThroughputAnalysis 1 0.005


# pscp -H "root@192.168.80.5" -l root /home/guolab/quagga-1.2.4.tar /home/guolab/
# scp root@192.168.80.5:/usr/local/sbin/bgpd /home/guolab/bgpd/bgpd-delay
# pssh -i -h /home/guolab/WCMPHost.txt -t 0 "rm /usr/local/sbin/bgpd"
# pscp -h /home/guolab/WCMPHost.txt -l root /home/guolab/bgpd/bgpd-delay /usr/local/sbin/bgpd