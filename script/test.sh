# g++ convergenceTimeAnalysis.cc -o convergenceTimeAnalysis
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/convergenceTimeAnalysis;"
# pscp -h /home/guolab/host.txt -l root /home/guolab/convergenceTimeAnalysis /home/guolab/

# pssh -i -h /home/guolab/PCHost.txt "reboot;"
# pssh -i -h /home/guolab/PCHost.txt "echo \'Hello\';"

# pssh -i -h /home/guolab/InfocomHost.txt "reboot;"
# pssh -i -h /home/guolab/InfocomHost.txt "echo \'Hello\';"

# g++ mytest.cc -o mytest
# ./mytest

# pssh -i -h /home/guolab/host/host.txt "zebra -d;bgpd -d;"
# pssh -i -h /home/guolab/host/host.txt "killall zebra; killall bgpd;"
# pssh -i -h /home/guolab/host/host.txt "reboot;"
# pssh -i -h /home/guolab/host/host.txt "echo \'Hello\';"
# pssh -i -h /home/guolab/host/host.txt "chmod 777 -R /home/guolab/CentralizedRouteTest"

# pssh -i -h /home/guolab/host.txt "echo \"net.ipv4.ip_forward=1\" | sudo tee -a /etc/sysctl.conf"

# g++ interfaceUpAndDown.cc -o interfaceUpAndDown
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/interfaceUpAndDown"
# pscp -h /home/guolab/host.txt -l root /home/guolab/interfaceUpAndDown /home/guolab/

# ./interfaceUpAndDown 60 76 ifdown eth1

# g++ convergenceTimeTotalAnalysis.cc -o convergenceTimeTotalAnalysis

# g++ recordFirstTime.cc -o recordFirstTime
# pscp -h /home/guolab/host.txt -l root /home/guolab/recordFirstTime /home/guolab/

# g++ recordOffTime.cc -o recordOffTime
# pscp -h /home/guolab/host.txt -l root /home/guolab/recordOffTime /home/guolab/

# g++ modifyBGPConf.cc -o modifyBGPConf
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/modifyBGPConf;"
# pscp -h /home/guolab/host.txt -l root /home/guolab/modifyBGPConf /home/guolab/

# g++ modifyZebraConf.cc -o modifyZebraConf
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/modifyZebraConf;"
# pscp -h /home/guolab/host.txt -l root /home/guolab/modifyZebraConf /home/guolab/

# pssh -i -h /home/guolab/host.txt "rm -rf /home/guolab/home"
# pssh -i -h /home/guolab/host.txt "rm -rf /home/guolab/quaggabyme.tar.gz"
# pssh -i -h /home/guolab/host.txt "rm -rf /home/guolab/bgpd.txt"

# pssh -i -h /home/guolab/host.txt "cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;"

# tar -zcvf quaggabyme.tar.gz quagga-1.2.4/

# pscp -h /home/guolab/host.txt -l root /home/guolab/quaggabyme.tar.gz /home/guolab/

# 先删除client中的bgp_fsm.c，再将Master的bgp_fsm.c拷贝到client，再编译
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/quagga-1.2.4/bgpd/bgp_fsm.c"
# pscp -h /home/guolab/host.txt -l root /home/guolab/quagga-1.2.4/bgpd/bgp_fsm.c /home/guolab/quagga-1.2.4/bgpd/
# pssh -i -h /home/guolab/host.txt "cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;"

# 先删除client中的bgpd.h，再将Master的bgpd.h拷贝到client，再编译
# pssh -i -h /home/guolab/host.txt "rm /home/guolab/quagga-1.2.4/bgpd/bgpd.h;"
# pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/quagga-1.2.4;"
# pscp -h /home/guolab/host.txt -l root /home/guolab/quagga-1.2.4/bgpd/bgpd.h /home/guolab/quagga-1.2.4/bgpd/
# pssh -i -h /home/guolab/host.txt "cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;"

# 先删除client中的zebra_rib.c，再将Master的zebra_rib.c拷贝到client，再编译
pssh -i -h /home/guolab/host.txt "rm /home/guolab/quagga-1.2.4/zebra/zebra_rib.c"
pscp -h /home/guolab/host.txt -l root /home/guolab/quagga-1.2.4/zebra/zebra_rib.c /home/guolab/quagga-1.2.4/zebra/
pssh -i -h /home/guolab/host.txt "cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;"

# 先删除client中的workqueue.c，再将Master的workqueue.c拷贝到client，再编译
pssh -i -h /home/guolab/host.txt "rm /home/guolab/quagga-1.2.4/lib/workqueue.c"
pscp -h /home/guolab/host.txt -l root /home/guolab/quagga-1.2.4/lib/workqueue.c /home/guolab/quagga-1.2.4/lib/
pssh -i -h /home/guolab/host.txt "cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;"

# 集中式实验命令
killall -9 CentralizedRoute
pssh -i -h /home/guolab/host.txt "killall -9 CentralizedRoute;"
pssh -i -h /home/guolab/host.txt "rm /home/guolab/CentralizedRouteTest/*;"
cd /home/guolab/CentralizedRouteTest
./route.sh
cd ..
pscp -h /home/guolab/host.txt -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
pscp -h /home/guolab/host.txt -l root /home/guolab/CentralizedRouteTest/CentralizedRoute /home/guolab/CentralizedRouteTest/
pssh -i -h /home/guolab/host.txt -t 0 "rm /home/guolab/output/*;"
rm -rf /home/guolab/output/center.log
/home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 2 4 16 2 1000000 &
pssh -i -h /home/guolab/host.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 1000000;" & 

# /home/guolab/centerTest.sh
# ./CentralizedRoute 10.0.1.67 6666 1 0 100 4 16 100

# 吞吐率测试
pssh -i -h /home/guolab/PCHost.txt -t 0 "/home/guolab/udp-client-my 192.168.1.2 5000 2000 /home/guolab/output/udpResult.txt;" &
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/udp-server-my 2000 /home/guolab/output/udpResults.txt  /home/guolab/output/udpThroughput.txt" &

pssh -i -h /home/guolab/PCHost.txt "killall udp-client-my;"
pssh -i -H "root@10.0.8.86" "killall udp-server-my;"

# BGP实验命令
pssh -i -h /home/guolab/host.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/host.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/host.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/output/;"

pssh -i -h /home/guolab/tcpTestHost.txt -t 0 "cd /home/guolab/;g++ interfaceUpAndDown.cc -o interfaceUpAndDown"

killall zebra; killall bgpd;
rm /home/guolab/output/*;
zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;
chmod 777 -R /home/guolab/output/;

./interfaceUpAndDown eth1 1 900000
./interfaceUpAndDown eth5 5 10000

g++ route-test.cc -o route-test
./route-test
ip route

sudo su
123.com

cat /home/guolab/output/updateKernelTimeRecord.txt
cat /home/guolab/output/

/home/guolab/udp-client-my-A 192.168.1.2 767 2048;

sudo vi /usr/local/etc/bgpd.conf

sudo vi /etc/network/interfaces

auto eth6
iface eth6 inet static
        address 21.8.2.0
        netmask 255.255.255.254

cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;cd ..;


route -n

ifdown eth
ifup eth

ifdown eth5
ifdown eth6

ifup eth5
ifup eth6

# server
sudo route add -net 32.0.0.0/8 gw 192.168.1.1
sudo route add -net 21.0.0.0/8 gw 192.168.1.1
sudo route add -net 192.168.3.0/24 gw 192.168.1.1

ip route replace 192.168.3.0/24 via 192.168.1.1 dev eth1 proto static initcwnd 10 rto_min 60 metric 10

# clients
sudo route add -net 32.0.0.0/8 gw 192.168.3.1
sudo route add -net 21.0.0.0/8 gw 192.168.3.1
sudo route add -net 192.168.1.0/24 gw 192.168.3.1

ip route replace 192.168.1.0/24 via 192.168.3.1 dev eth1 proto static initcwnd 10 rto_min 60 metric 10

pssh -i -h /home/guolab/PCHost.txt "ping 192.168.1.2;"
pssh -i -h /home/guolab/PCHost.txt "sudo route add -net 32.0.0.0/8 gw 192.168.3.1;sudo route add -net 21.0.0.0/8 gw 192.168.3.1;sudo route add -net 192.168.1.0/24 gw 192.168.3.1;"

# ip route replace default equalize nexthop via 21.5.1.0 dev eth1 weight 1 nexthop via 21.6.1.0 dev eth2 weight 1 nexthop via 21.7.1.0 dev eth3 weight 1 nexthop via 21.8.1.0 dev eth4 weight 1
ip route replace default scope global nexthop via 21.5.1.0 dev eth1 weight 1 nexthop via 21.6.1.0 dev eth2 weight 1 nexthop via 21.7.1.0 dev eth3 weight 1 nexthop via 21.8.1.0 dev eth4 weight 1
ip route replace 192.168.1.0/24 nexthop via 21.5.1.0 dev eth1 weight 1 nexthop via 21.6.1.0 dev eth2 weight 1 nexthop via 21.7.1.0 dev eth3 weight 1 nexthop via 21.8.1.0 dev eth4 weight 1
ip route add 192.168.1.0/24 nexthop via 21.5.1.0 weight 1 nexthop via 21.6.1.0 weight 2 nexthop via 21.7.1.0 weight 2 nexthop via 21.8.1.0 weight 2
route del -net 192.168.1.0/24

# 端口抓包
# tcpdump ip -i eth5 host 172.16.1.2 and 192.168.1.2
iperf -c 192.168.1.2 -i 0.05 -t 10 # 0.05s测一次吞吐率，共10s，向192.168.1.2发送数据
iperf -s -i 0.05 > a.txt #接收数据，0.05s记录一次吞吐率


# cd /home/guolab/quagga-1.2.4
# ./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64
# make
# make install
# linux 压缩命令  tar -zvcf buodo.tar.gz buodo  tar -jvcf buodo.tar.bz2 buodo 

pssh -i -h /home/guolab/host.txt "rm /home/guolab/quaggabyme.tar.gz;rm -rf /home/guolab/quagga-1.2.4;"
pscp -h /home/guolab/host.txt -l root /home/guolab/quagga-1.2.4.tar /home/guolab/
pssh -i -h /home/guolab/host.txt "cd /home/guolab;tar zxvf quagga-1.2.4.tar;"
pssh -i -h /home/guolab/host.txt "cd /home/guolab/quagga-1.2.4;./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64;make;make install;cd ..;"

pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/quagga-1.2.4.tar;rm -rf /home/guolab/quagga-1.2.4;"
pscp -h /home/guolab/InfocomHost.txt -l root /home/guolab/quagga-1.2.4.tar /home/guolab/
pssh -i -h /home/guolab/InfocomHost.txt "cd /home/guolab;tar zxvf quagga-1.2.4.tar;"

# BGP-Test-2.4 configure
# This file describes the network interfaces available on your system
# and how to activate them. For more information, see interfaces(5).

source /etc/network/interfaces.d/*

# The loopback network interface
auto lo
iface lo inet loopback

# The primary network interface
auto eth0
iface eth0 inet static
        address 10.0.8.79
        netmask 255.255.0.0
        network 10.0.0.0
        broadcast 10.0.255.255
        gateway 10.0.0.1
        # dns-* options are implemented by the resolvconf package, if installed
        dns-nameservers 114.114.114.114
auto eth1
iface eth1 inet static
        address 32.13.1.1
        netmask 255.255.255.254
auto eth2
iface eth2 inet static
        address 32.14.1.1
        netmask 255.255.255.254
auto eth3
iface eth3 inet static
        address 32.15.1.1
        netmask 255.255.255.254
auto eth4
iface eth4 inet static
        address 32.16.1.1
        netmask 255.255.255.254
auto eth5
iface eth5 inet static
        address 21.4.1.0
        netmask 255.255.255.254
auto eth6
iface eth6 inet static
        address 21.4.2.0
        netmask 255.255.255.254



auto eth1
iface eth1 inet static
        address 32.13.2.1
        netmask 255.255.255.254
auto eth2
iface eth2 inet static
        address 32.14.2.1
        netmask 255.255.255.254
auto eth3
iface eth3 inet static
        address 32.15.2.1
        netmask 255.255.255.254
auto eth4
iface eth4 inet static
        address 32.16.2.1
        netmask 255.255.255.254
auto eth5
iface eth5 inet static
        address 21.8.1.0
        netmask 255.255.255.254
auto eth6
iface eth6 inet static
        address 21.8.2.0
        netmask 255.255.255.254

pssh -i -h /home/guolab/InfocomHost.txt "/home/guolab/modifyBGPConf 10 2 4 16 2;"

# 1.1
router bgp 65002
 bgp router-id 10.0.0.1
 neighbor 43.1.1.1 remote-as 65001
 neighbor 43.1.1.1 allowas-in 1
 neighbor 43.2.1.1 remote-as 65001
 neighbor 43.2.1.1 allowas-in 1
 maximum-paths 64
   redistribute connected
 address-family ipv4 unicast
   neighbor 43.1.1.1 activate
   neighbor 43.2.1.1 activate
  exit-address-family


# 1.2
router bgp 65003
 bgp router-id 10.0.0.2
 neighbor 43.1.2.1 remote-as 65001
 neighbor 43.1.2.1 allowas-in 1
 neighbor 43.2.2.1 remote-as 65001
 neighbor 43.2.2.1 allowas-in 1
 maximum-paths 64
   redistribute connected
 address-family ipv4 unicast
   neighbor 43.1.2.1 activate
   neighbor 43.2.2.1 activate
  exit-address-family

# 2.1
router bgp 65001
 bgp router-id 20.0.0.1
 neighbor 43.1.1.2 remote-as 65002
 neighbor 43.1.1.2 allowas-in 1
 neighbor 43.1.2.2 remote-as 65003
 neighbor 43.1.2.2 allowas-in 1
 maximum-paths 64
   redistribute connected
 address-family ipv4 unicast
   neighbor 43.1.1.2 activate
   neighbor 43.1.2.2 activate
  exit-address-family

# 2.2
router bgp 65001
 bgp router-id 20.0.0.2
 neighbor 43.2.1.2 remote-as 65002
 neighbor 43.2.1.2 allowas-in 1
 neighbor 43.2.2.2 remote-as 65003
 neighbor 43.2.2.2 allowas-in 1
 maximum-paths 64
   redistribute connected
 address-family ipv4 unicast
   neighbor 43.2.1.2 activate
   neighbor 43.2.2.2 activate
  exit-address-family

auto eth3
iface eth3 inet static
        address 192.168.1.1
        netmask 255.255.255.0
auto eth4
iface eth4 inet static
        address 192.168.2.1
        netmask 255.255.255.0


# server 1
sudo route add -net 32.0.0.0/8 gw 192.168.1.1
sudo route add -net 21.0.0.0/8 gw 192.168.1.1
sudo route add -net 192.168.0.0/16 gw 192.168.1.1

# server 2
sudo route add -net 32.0.0.0/8 gw 192.168.2.1
sudo route add -net 21.0.0.0/8 gw 192.168.2.1
sudo route add -net 192.168.0.0/16 gw 192.168.2.1

# client 1
sudo route add -net 32.0.0.0/8 gw 192.168.3.1
sudo route add -net 21.0.0.0/8 gw 192.168.3.1
sudo route add -net 192.168.0.0/16 gw 192.168.3.1

# client 2
sudo route add -net 32.0.0.0/8 gw 192.168.4.1
sudo route add -net 21.0.0.0/8 gw 192.168.4.1
sudo route add -net 192.168.0.0/16 gw 192.168.4.1


# auto eth1
# iface eth1 inet static
#         address 21.1.1.1
#         netmask 255.255.255.254
# auto eth2
# iface eth2 inet static
#         address 21.2.1.1
#         netmask 255.255.255.254
# auto eth3
# iface eth3 inet static
#         address 21.3.1.1
#         netmask 255.255.255.254
# auto eth4
# iface eth4 inet static
#         address 21.4.1.1
#         netmask 255.255.255.254
# auto eth5
# iface eth5 inet static
#         address 192.168.1.1
#         netmask 255.255.255.0
auto eth1
iface eth1 inet static
        address 21.4.1.1
        netmask 255.255.255.254
auto eth2
iface eth2 inet static
        address 192.168.1.1
        netmask 255.255.255.0

sudo su
123.com


sudo vi /etc/network/interfaces

auto eth0
iface eth0 inet static
        address 192.168.80.4
        netmask 255.255.0.0
auto eth1
iface eth1 inet static
        address 32.16.1.0
        netmask 255.255.255.254




g++ udpThroughputAnalysis.cc -o udpThroughputAnalysis
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.79" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pscp -H "root@10.0.8.86" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.79" -l root /home/guolab/udpThroughputAnalysis /home/guolab/


g++ udpThroughputAnalysis.cc -o udpThroughputAnalysis
pssh -i -H "root@10.0.8.84" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.96" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.85" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.97" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/udpThroughputAnalysis;"
pscp -H "root@10.0.8.84" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.96" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.85" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.97" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.86" -l root /home/guolab/udpThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/udpThroughputAnalysis /home/guolab/

g++ iperfThroughputAnalysis.cc -o iperfThroughputAnalysis
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/iperfThroughputAnalysis;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/iperfThroughputAnalysis;"
pscp -H "root@10.0.8.86" -l root /home/guolab/iperfThroughputAnalysis /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/iperfThroughputAnalysis /home/guolab/

g++ iperfTimeAnalysis.cc -o iperfTimeAnalysis
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/iperfTimeAnalysis;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/iperfTimeAnalysis;"
pscp -H "root@10.0.8.86" -l root /home/guolab/iperfTimeAnalysis /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/iperfTimeAnalysis /home/guolab/



g++ iperfTimeTotalAnalysis.cc -o iperfTimeTotalAnalysis
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/iperfTimeTotalAnalysis;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/iperfTimeTotalAnalysis;"
pscp -H "root@10.0.8.86" -l root /home/guolab/iperfTimeTotalAnalysis /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/iperfTimeTotalAnalysis /home/guolab/

pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-400 /usr/local/sbin/bgpd

g++ record.cc -o record
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/record;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/record;"
pscp -H "root@10.0.8.86" -l root /home/guolab/record /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/record /home/guolab/

# 3.15
route add -net 21.4.1.0/24 gw 32.15.1.1
route add -net 21.4.2.0/24 gw 32.15.1.1

# 3.16
route add -net 21.4.1.0/24 gw 32.16.1.1
route add -net 21.4.2.0/24 gw 32.16.1.1

# 2.4
route add -net 32.15.2.0/24 gw 32.15.1.0
route add -net 32.16.2.0/24 gw 32.16.1.0

# 2.8
route add -net 32.15.1.0/24 gw 32.15.2.0
route add -net 32.16.1.0/24 gw 32.16.2.0
ip route add 21.4.1.0/24 scope global nexthop via 32.16.2.0 weight 1 nexthop via 32.15.2.0 weight 1
ip route add 21.4.2.0/24 scope global nexthop via 32.16.2.0 weight 1 nexthop via 32.15.2.0 weight 1

# 1.1
route add -net 32.15.2.0/24 gw 21.4.1.0
route add -net 32.16.2.0/24 gw 21.4.1.0

# 1.2
route add -net 32.15.2.0/24 gw 21.4.2.0
route add -net 32.16.2.0/24 gw 21.4.2.0

pssh -i -h /home/guolab/InfocomHost.txt -t 0 "rm /usr/local/sbin/bgpd"
pscp -h /home/guolab/InfocomHost.txt -l root /home/guolab/bgpd/bgpd /usr/local/sbin/bgpd

pscp -H "root@192.168.80.9" -l root /home/guolab/download/linux-4.12.14.tar.gz /home/guolab



# # client--server udp
# pssh -i -H "root@10.0.8.86" -t 0 "killall udp-server-my;"
# pssh -i -H "root@10.0.8.98" -t 0 "killall udp-server-my;"
# pssh -i -H "root@10.0.2.89" -t 0 "killall udp-client-my;"
# pssh -i -H "root@10.0.2.90" -t 0 "killall udp-client-my;"
# # 启动server和client
# sleep 10
# # 启动server
# echo "启动udp-server-my"
# pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/udp-server-my 767;" & 
# pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/udp-server-my 767;" &
# sleep 3
# echo "启动udp-client-my"
# pssh -i -H "root@10.0.2.89" -t 0 "/home/guolab/udp-client-my 192.168.1.2 767 4196;" & # 服务器地址，port，发送总字节数
# pssh -i -H "root@10.0.2.90" -t 0 "/home/guolab/udp-client-my 192.168.2.2 767 4196;" &
# # 1s后再启动linkchange的客户端
# sleep 2
# echo "启动linkchange-server"
# pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
# sleep 1
# echo "启动linkchange-client"
# pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 20 300000;" &
# # pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 2 3000000;" &
# sleep 30
# echo "停止发送流量"
# pssh -i -H "root@10.0.8.86" -t 0 "killall udp-server-my;/home/guolab/udpThroughputAnalysis 1 0.05;"
# pssh -i -H "root@10.0.8.98" -t 0 "killall udp-server-my;/home/guolab/udpThroughputAnalysis 1 0.05;"
# pssh -i -H "root@10.0.2.89" -t 0 "killall udp-client-my;"
# pssh -i -H "root@10.0.2.90" -t 0 "killall udp-client-my;"



# # # 2.8--2.4 udp
# # pssh -i -H "root@10.0.8.79" -t 0 "killall udp-server-my-A;"
# # pssh -i -H "root@10.0.8.79" -t 0 "killall udp-server-my-B;"
# # pssh -i -H "root@10.0.8.83" -t 0 "killall udp-client-my-A;"
# # pssh -i -H "root@10.0.8.83" -t 0 "killall udp-client-my-B;"
# # # 启动server和client
# # sleep 10
# # # 启动server
# # echo "启动udp-server-my"
# # pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/udp-server-my-A 767;" &
# # pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/udp-server-my-B 768;" &
# # sleep 3
# # echo "启动udp-client-my"
# # pssh -i -H "root@10.0.8.83" -t 0 "/home/guolab/udp-client-my-A 32.15.1.1 767 8192;" & 
# # pssh -i -H "root@10.0.8.83" -t 0 "/home/guolab/udp-client-my-B 32.16.1.1 768 8192;" & 
# # # 1s后再启动linkchange的客户端
# # sleep 20
# # echo "启动linkchange-server"
# # pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
# # sleep 1
# # echo "启动linkchange-client"
# # pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 2 900000;" &
# # pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 2 3000000;" &
# # sleep 60
# # echo "停止发送流量"
# # pssh -i -H "root@10.0.8.79" -t 0 "killall udp-server-my-A;/home/guolab/udpThroughputAnalysis-A 0.005;"#两种处理方法，我自己写的和汪老板的
# # pssh -i -H "root@10.0.8.79" -t 0 "killall udp-server-my-B;/home/guolab/udpThroughputAnalysis-B 0.005;"
# # pssh -i -H "root@10.0.8.83" -t 0 "killall udp-client-my-A;killall udp-client-my-B;"



# # # 2.8--ToR udp
# pssh -i -H "root@10.0.8.83" -t 0 "killall udp-client-my-A;killall udp-client-my-B;"
# pssh -i -H "root@10.0.8.84" -t 0 "killall udp-server-my;"
# pssh -i -H "root@10.0.8.96" -t 0 "killall udp-server-my;"
# # # 启动server和client
# sleep 10
# # # 启动server
# echo "启动udp-server-my"
# pssh -i -H "root@10.0.8.84" -t 0 "/home/guolab/udp-server-my 767;" & 
# pssh -i -H "root@10.0.8.96" -t 0 "/home/guolab/udp-server-my 767;" &
# sleep 3
# echo "启动udp-client-my"
# pssh -i -H "root@10.0.8.83" -t 0 "/home/guolab/udp-client-my-A 192.168.1.1 767 2048;" & # 服务器地址，port，发送总字节数
# pssh -i -H "root@10.0.8.83" -t 0 "/home/guolab/udp-client-my-B 192.168.2.1 767 2048;" &
# sleep 3
# # 启动linkchange的服务端
# echo "启动linkchange-server"
# pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
# sleep 1
# echo "启动linkchange-client"
# pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 25 300000;" &
# pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 5000000;" &
# sleep 30
# echo "停止发送流量"
# pssh -i -H "root@10.0.8.83" -t 0 "killall udp-client-my-A;killall udp-client-my-B;"
# pssh -i -H "root@10.0.8.84" -t 0 "killall udp-server-my;/home/guolab/udpThroughputAnalysis 1 0.05;" 
# pssh -i -H "root@10.0.8.96" -t 0 "killall udp-server-my;/home/guolab/udpThroughputAnalysis 1 0.05;"

# 远程到本地
scp root@192.168.80.1:/home/guolab/output/bgpd.log /home/guolab/output/bgpd-3.11.log
scp root@192.168.80.2:/home/guolab/output/bgpd.log /home/guolab/output/bgpd-3.12.log
scp root@192.168.80.7:/home/guolab/output/bgpd.log /home/guolab/output/bgpd-2.7.log
scp root@192.168.80.8:/home/guolab/output/bgpd.log /home/guolab/output/bgpd-2.8.log

# 本地到远程
scp /home/guolab/output/bgpd-3.11.log root@192.168.80.1:/home/guolab/output/bgpd.log
scp /home/guolab/CentralizedRouteTest/id_rsa.pub root@10.0.0.4:/home/tencent/