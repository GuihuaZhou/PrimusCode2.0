# ##########################虚拟机测试部分##########################
# # pssh -i -H "root@10.0.8.84" -t 0 "ifconfig eth3 down;ifconfig eth4 down;"
# # pssh -i -H "root@10.0.8.85" -t 0 "ifconfig eth3 down;ifconfig eth4 down;"
# # pssh -i -H "root@10.0.8.96" -t 0 "ifconfig eth3 down;ifconfig eth4 down;"
# # pssh -i -H "root@10.0.8.97" -t 0 "ifconfig eth3 down;ifconfig eth4 down;"

# killall -9 Primus
# pssh -i -h /home/guolab/host/host.txt "killall -9 Primus;"
# pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;"

# # # 删除一些日志文件
# pssh -i -h /home/guolab/host/host.txt "rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;"
# pssh -i -h /home/guolab/host/host.txt "rm /var/log/NodeMapToSock*.txt;rm /var/log/NodeInDirPathTable*.txt;rm /var/log/MasterMapToSock*.txt;"

# pssh -i -h /home/guolab/host/master.txt "rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;"
# pssh -i -h /home/guolab/host/master.txt "rm /var/log/NodeMapToSock*.txt;rm /var/log/MasterLinkTable*.txt;rm /var/log/ClusterMasterInfo*.txt;rm /var/log/MasterInDirPathTable*.txt;"

# rm /var/log/Primus*.log
# rm /var/log/PathEntryTable*.txt
# rm /var/log/MappingTable*.txt
# rm /var/log/NodeMapToSock*.txt
# rm /var/log/MasterLinkTable*.txt
# rm /var/log/ClusterMasterInfo*.txt;
# rm /var/log/MasterInDirPathTable*.txt;
# # master先编译
# rm /home/guolab/Primus/Primus
# cd /home/guolab/Primus
# g++ -std=c++0x tcp.cc udp.cc ipv4-global-routing.cc init.cc -o Primus -lpthread
# cd ..
# # 将编译好的Primus复制到虚拟机上
# pssh -i -h /home/guolab/host/host.txt "rm -rf /home/guolab/Primus/Primus;"
# pssh -i -h /home/guolab/host/master.txt "rm -rf /home/guolab/Primus/Primus;"
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus/Primus
# pscp -h /home/guolab/host/master.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus/Primus

# echo ""
# echo "start master."
# pssh -i -H "root@10.0.1.68" -t 0 "/home/guolab/Primus/Primus;" & # chief Master
# sleep 3
# /home/guolab/Primus/Primus &
# pssh -i -H "root@10.0.1.69" -t 0 "/home/guolab/Primus/Primus;" &

# # /home/guolab/Primus/Primus &
# # sleep 3
# # pssh -i -h /home/guolab/host/master.txt -t 0 "/home/guolab/Primus/Primus;" &
# sleep 5
# echo ""
# echo "start node."
# echo ""
# pssh -i -h /home/guolab/host/host.txt -t 0 "/home/guolab/Primus/Primus;" & # 运行Node上的可执行文件
# ########################## ##########################
# pssh -i -H "root@10.0.8.76" -t 0 "ifconfig eth5 down;"&
# pssh -i -H "root@10.0.8.84" -t 0 "ifconfig eth1 down;"&

# pssh -i -H "root@10.0.8.76" -t 0 "ifconfig eth5 up;"&
# pssh -i -H "root@10.0.8.84" -t 0 "ifconfig eth1 up;"&

# pssh -i -H "root@10.0.8.77" -t 0 "ifconfig eth5 down;"&
# pssh -i -H "root@10.0.8.84" -t 0 "ifconfig eth2 down;"&

# pssh -i -H "root@10.0.8.77" -t 0 "ifconfig eth5 up;"&
# pssh -i -H "root@10.0.8.84" -t 0 "ifconfig eth2 up;"&

##########################SONIC测试部分##########################
# killall -9 Primus
# pssh -i -h /home/guolab/host/tencent.txt "killall -9 Primus;"
# pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;"
# # # 复制代码到sonic上
# pscp -h /home/guolab/host/tencent.txt -l root /home/guolab/Primus/* /home/tencent/Primus/ 
# pssh -i -h /home/guolab/host/tencent.txt "rm /home/tencent/Primus/Primus;" # 删除各个sonic上已有的Primus
# # # 复制代码到master上
# pscp -h /home/guolab/host/master.txt -l root /home/guolab/Primus/* /home/guolab/Primus/ 
# pssh -i -h /home/guolab/host/master.txt "rm /home/guolab/Primus/Primus;" # 删除各个master上已有的Primus

# # # 删除一些日志文件
# pssh -i -h /home/guolab/host/tencent.txt "rm /var/log/Primus*.log;"
# pssh -i -h /home/guolab/host/tencent.txt "rm /var/log/PathEntryTable*.txt;"
# pssh -i -h /home/guolab/host/tencent.txt "rm /var/log/MappingTable*.txt;"
# pssh -i -h /home/guolab/host/tencent.txt "rm /var/log/NodeMapToSock*.txt;"
# pssh -i -h /home/guolab/host/tencent.txt "rm /var/log/NodeConMasterByNode*.txt;"
# pssh -i -h /home/guolab/host/tencent.txt "rm /var/log/MasterMapToSock*.txt;"

# pssh -i -h /home/guolab/host/master.txt "rm /var/log/Primus*.log;"
# pssh -i -h /home/guolab/host/master.txt "rm /var/log/PathEntryTable*.txt;"
# pssh -i -h /home/guolab/host/master.txt "rm /var/log/MappingTable*.txt;"
# pssh -i -h /home/guolab/host/master.txt "rm /var/log/NodeMapToSock*.txt;"
# pssh -i -h /home/guolab/host/master.txt "rm /var/log/MasterLinkTable*.txt;"

# rm /var/log/Primus*.log
# rm /var/log/PathEntryTableu*.txt
# rm /var/log/MappingTable*.txt
# rm /var/log/NodeMapToSock*.txt
# rm /var/log/MasterLinkTable*.txt
# # # master再编译
# cd /home/guolab/Primus
# rm /home/guolab/Primus/Primus
# g++ -std=c++0x tcp.cc udp.cc ipv4-global-routing.cc init.cc -o Primus -lpthread
# cd ..
# # # 先启动master
# echo ""
# echo "start master"
# /home/guolab/Primus/Primus 0 0 &
# pssh -i -h /home/guolab/host/master.txt "/home/guolab/Primus/run.sh;" &
# # # sonic编译和执行
# sleep 10
# pssh -i -h /home/guolab/host/tencent.txt "/home/tencent/Primus/run.sh;" &
# # # 运行XXs后再关闭
# sleep 80
# pssh -i -h /home/guolab/host/tencent.txt "killall -9 Primus;"
# pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;"
# killall -9 Primus
# sleep 5
# cat /var/log/MasterLinkTable-0.0.txt
# echo ""
# cat /var/log/Primus-0.0.log
# echo ""
# cat /var/log/NodeMapToSock-0.0.txt
########################## ##########################

# ssh-keygen -t rsa
# touch /root/.ssh/authorized_keys
# cat /home/guolab/id_rsa.pub >> /root/.ssh/authorized_keys
# chmod 700 /root/.ssh
# chmod 600 /root/.ssh/authorized_keys

# pssh -i -h /home/guolab/host/host.txt "shutdown -t 10;"
# pssh -i -h /home/guolab/host/host.txt "echo 'hello';"

# 常见的关机命令有

# shutdown -a ===>使用/etc/shutdown.allow 来验证身份

# shutdown -t  ===>t表示time 后面一般会接时间（s秒），表示多久之后，在发送kill信号

# shutdown -r  ===> r表示reboot，重启

# shutdown -h ===> h表示halted停机。

# shutdown -c ===> c表示cancels ，取消shutdown操作

# shutdown -h 20:30 'This system will shutdown,please save your file'


##########################虚拟机测试部分##########################
killall -9 Primus
pssh -i -h /home/guolab/host/ATChost.txt "killall -9 Primus;"
pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;"

# 确保所有网卡都是正常的
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
ssh root@10.0.80.12 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"

# # 删除一些日志文件
pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/primusStamp.txt;rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;rm /var/log/NodeMapToSock*.txt;rm /var/log/NodeInDirPathTable*.txt;rm /var/log/MasterMapToSock*.txt;rm /var/log/NodeLinkTable*.txt;"

pssh -i -h /home/guolab/host/master.txt "rm /home/guolab/output/primusStamp.txt;rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;rm /var/log/NodeMapToSock*.txt;rm /var/log/MasterLinkTable*.txt;rm /var/log/ClusterMasterInfo*.txt;rm /var/log/MasterInDirPathTable*.txt;rm /var/log/MasterMapToSock*.txt;"

rm /home/guolab/output/primusStamp.txt
rm /var/log/Primus*.log
rm /var/log/PathEntryTable*.txt
rm /var/log/MappingTable*.txt
rm /var/log/NodeMapToSock*.txt
rm /var/log/MasterLinkTable*.txt
rm /var/log/ClusterMasterInfo*.txt;
rm /var/log/MasterInDirPathTable*.txt;
rm /var/log/MasterMapToSock*.txt;
# master先编译
rm /home/guolab/Primus/Primus
cd /home/guolab/Primus
g++ -std=c++0x tcp.cc udp.cc ipv4-global-routing.cc init.cc -o Primus -lpthread
cd ..
# 将编译好的Primus复制到虚拟机上
pssh -i -h /home/guolab/host/ATChost.txt "rm -rf /home/guolab/Primus/Primus;"
pssh -i -h /home/guolab/host/master.txt "rm -rf /home/guolab/Primus/Primus;"
pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus/Primus
pscp -h /home/guolab/host/master.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus/Primus

echo ""
echo "start master."
/home/guolab/Primus/Primus &
#1>/home/guolab/Primus/master.stdout 2>/home/guolab/Primus/master.stderr &
# sleep 3
# pssh -i -h /home/guolab/host/master.txt "/home/guolab/Primus/Primus;" &

# /home/guolab/Primus/Primus &
# sleep 3
# pssh -i -h /home/guolab/ATChost/master.txt -t 0 "/home/guolab/Primus/Primus;" &
sleep 5
echo ""
echo "start node."
echo ""
pssh -i -h /home/guolab/host/ATChost.txt "/home/guolab/Primus/Primus 1>/home/guolab/Primus/switch.stdout 2>/home/guolab/Primus/switch.stderr;" & # 运行Node上的可执行文件

# sleep 30
# rm /home/guolab/output/primusStamp.txt
# touch /home/guolab/output/primusStamp.txt
# chmod 777 -R /home/guolab/output/*
# pssh -i -h /home/guolab/host/master.txt "rm /home/guolab/output/primusStamp.txt;touch /home/guolab/output/primusStamp.txt;chmod 777 -R /home/guolab/output/*;"
# pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/primusStamp.txt;touch /home/guolab/output/primusStamp.txt;chmod 777 -R /home/guolab/output/*;"


# sleep 5
# for ((i=0;i<500;i++))
# do
# 	ssh root@10.0.80.10 "ifconfig eth1 down"
# 	sleep 2
# 	ssh root@10.0.80.10 "ifconfig eth1 up"
# 	sleep 2
# done
########################## ##########################
# pssh -i -h /home/guolab/host/ATChost.txt "echo "net.ipv4.ip_forward=1" | sudo tee -a /etc/sysctl.conf;sudo sysctl -p;"
# pssh -i -h /home/guolab/host/ATChost.txt "apt install traceroute -y;"

# # linux update github
# git add .
# git commit -m "add"
# git push -u origin master

# sudo su
# 123.com

# rm /home/guolab/tool/tcp-server
# pssh -i -h /home/guolab/host/ATCServerhost.txt "rm -rf /home/guolab/tcp-server;"
# g++ tcp-server.c -o tcp-server -lpthread
# pscp -h /home/guolab/host/ATCServerhost.txt -l root /home/guolab/tool/tcp-server /home/guolab/

# rm /home/guolab/tool/tcp-client
# pssh -i -h /home/guolab/host/ATCClienthost.txt "rm -rf /home/guolab/tcp-client;"
# g++ tcp-client.c -o tcp-client
# pscp -h /home/guolab/host/ATCClienthost.txt -l root /home/guolab/tool/tcp-client /home/guolab/


# rm /home/guolab/tool/udp-server
# pssh -i -h /home/guolab/host/ATCServerhost.txt "rm -rf /home/guolab/udp-server;"
# g++ udp-server.c -o udp-server -lpthread
# pscp -h /home/guolab/host/ATCServerhost.txt -l root /home/guolab/tool/udp-server /home/guolab/

# rm /home/guolab/tool/udp-client
# pssh -i -h /home/guolab/host/ATCClienthost.txt "rm -rf /home/guolab/udp-client;"
# g++ udp-client.c -o udp-client
# pscp -h /home/guolab/host/ATCClienthost.txt -l root /home/guolab/tool/udp-client /home/guolab/

# /home/guolab/script/ATCTest2.sh

# rm /home/guolab/ATCOutput/*
# killall -9 tcp-server 
# ./tcp-server 3 &
# cat /home/guolab/ATCOutput/server-record.txt

# rm /home/guolab/ATCOutput/*
# killall -9 tcp-client
# ./tcp-client 192.168.1.2 &
# cat ATCOutput/client-record.txt

# pssh -i -h /home/guolab/host/ATChost.txt "rm /var/log/Primus*.log"
# ;ip route replace 192.168.0.0/16 via 192.168.3.1 dev eth1 proto static initcwnd 10 rto_min 60 metric 10;

# rm /home/guolab/tool/linkChange
# g++ /home/guolab/tool/linkChange.cc -o /home/guolab/tool/linkChange -lpthread
# cp /home/guolab/tool/linkChange /home/guolab/script/linkChange
# rm /home/guolab/output/linkChangeRecord.txt
# killall -9 linkChange
# /home/guolab/script/linkChange /home/guolab/output/ATCTest-primus.log /home/guolab/tool/linkInfo.txt 10 0 0 0.25 Primus

# /home/guolab/script/linkChange /home/guolab/output/linkChangeRecord.txt 2 100000 0 0.25 Primus

# /home/guolab/script/linkChange /home/guolab/output/linkChangeRecord.txt 100 &

# pssh -i -h /home/guolab/host/ATChost.txt "rm /usr/local/sbin/bgpd;"
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/bgpd /usr/local/sbin/
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/quagga-1.2.4.tar.gz /home/guolab/
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/libreadline6-dev_6.3-8ubuntu2_amd64.deb /home/guolab/
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/c-ares-1.14.0.tar.gz /home/guolab/
# pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/pkg-config-0.29.2.tar.gz /home/guolab/
# pssh -i -h /home/guolab/host/ATChost.txt "mkdir /home/guolab/output;"
# pssh -i -h /home/guolab/host/ATChost.txt "reboot;"
# pssh -i -h /home/guolab/host/master.txt "reboot;"
# pssh -i -h /home/guolab/host/ATChost.txt "echo \'hello\';"
# pssh -i -h /home/guolab/host/master.txt "echo \'hello\';"
# pssh -i -h /home/guolab/host/ATChost.txt "apt install wget -y;"
# pssh -i -h /home/guolab/host/ATChost.txt "wget http://download.savannah.gnu.org/releases/quagga/quagga-1.2.4.tar.gz;"
# pssh -i -h /home/guolab/host/ATChost.txt "cp /usr/local/etc/bgpd.conf.sample /usr/local/etc/bgpd.conf;cp /usr/local/etc/zebra.conf.sample /usr/local/etc/zebra.conf;"
# pssh -i -h /home/guolab/host/ATChost.txt "date -s 01/01/1970"

# cd /var/lib/dpkg/updates
# rm -r ./*
# sudo apt-get update
# cd /home/guolab/quagga-1.2.4
# ./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities --enable-vtysh --enable-zebra --enable-multipath=64 CFLAGS="-fPIC -g" LDFLAGS="-pie -rdynamic"
# make
# make install
# cp /usr/local/sbin/bgpd /home/guolab/

# exit
# screen

# sudo su
# 123.com

# # screen -rd
# cat /usr/local/etc/bgpd.conf
# cat /etc/network/interfaces
# # 3.7
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.37
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.8.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.8.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 10.0.80.37
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.8
#  neighbor 32.8.1.2 remote-as 65020
#  neighbor 32.8.1.2 advertisement-interval 1
#  neighbor 32.8.2.2 remote-as 65020
#  neighbor 32.8.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.8.1.2 activate
#    neighbor 32.8.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout


# # 3.6
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.36
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.7.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.7.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 10.0.80.36
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114

# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.7
#  neighbor 32.7.1.2 remote-as 65020
#  neighbor 32.7.1.2 advertisement-interval 1
#  neighbor 32.7.2.2 remote-as 65020
#  neighbor 32.7.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.7.1.2 activate
#    neighbor 32.7.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout


# # 3.5
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.35
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.6.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.6.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 10.0.80.35
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114

# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.6
#  neighbor 32.6.1.2 remote-as 65020
#  neighbor 32.6.1.2 advertisement-interval 1
#  neighbor 32.6.2.2 remote-as 65020
#  neighbor 32.6.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.6.1.2 activate
#    neighbor 32.6.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout

# # 3.4
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.34
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.5.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.5.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 10.0.80.34
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.5
#  neighbor 32.5.1.2 remote-as 65020
#  neighbor 32.5.1.2 advertisement-interval 1
#  neighbor 32.5.2.2 remote-as 65020
#  neighbor 32.5.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.5.1.2 activate
#    neighbor 32.5.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout

# # 3.3
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.33
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.4.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.4.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 10.0.80.33
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.4
#  neighbor 32.4.1.2 remote-as 65020
#  neighbor 32.4.1.2 advertisement-interval 1
#  neighbor 32.4.2.2 remote-as 65020
#  neighbor 32.4.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.4.1.2 activate
#    neighbor 32.4.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout


# # 3.2
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.32
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.3.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.3.2.1
#         netmask 255.255.255.0
# iface eth3 inet static
#         address 10.0.80.32
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114

# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.3
#  neighbor 32.3.1.2 remote-as 65020
#  neighbor 32.3.1.2 advertisement-interval 1
#  neighbor 32.3.2.2 remote-as 65020
#  neighbor 32.3.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.3.1.2 activate
#    neighbor 32.3.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout


# # 3.1
# # The loopback network interface
# auto lo
# iface lo inet loopback

# # The primary network interface
# auto eth0
# iface eth0 inet static
#       address 172.16.80.31
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.2.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.2.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 10.0.80.31
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.2
#  neighbor 32.2.1.2 remote-as 65020
#  neighbor 32.2.1.2 advertisement-interval 1
#  neighbor 32.2.2.2 remote-as 65020
#  neighbor 32.2.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.2.1.2 activate
#    neighbor 32.2.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout


# # 3.0
# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#       address 172.16.80.30
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.1.1.1
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.1.2.1
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#   address 10.0.80.30
#   netmask 255.255.0.0
#   network 10.0.0.0
#   broadcast 10.0.255.255
#   gateway 10.0.0.1
#   dns-nameservers 114.114.114.114


# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65030
#  bgp router-id 30.0.0.1
#  neighbor 32.1.1.2 remote-as 65020
#  neighbor 32.1.1.2 advertisement-interval 1
#  neighbor 32.1.2.2 remote-as 65020
#  neighbor 32.1.2.2 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.1.1.2 activate
#    neighbor 32.1.2.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout


# # 2.3
# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65020
#  bgp router-id 20.0.0.4
#  neighbor 32.5.2.1 remote-as 65030
#  neighbor 32.5.2.1 advertisement-interval 1
#  neighbor 32.6.2.1 remote-as 65030
#  neighbor 32.6.2.1 advertisement-interval 1
#  neighbor 32.7.2.1 remote-as 65030
#  neighbor 32.7.2.1 advertisement-interval 1
#  neighbor 32.8.2.1 remote-as 65030
#  neighbor 32.8.2.1 advertisement-interval 1
#  neighbor 21.4.1.2 remote-as 65012
#  neighbor 21.4.1.2 advertisement-interval 1
#  neighbor 32.5.2.1 allowas-in 1
#  neighbor 32.6.2.1 allowas-in 1
#  neighbor 32.7.2.1 allowas-in 1
#  neighbor 32.8.2.1 allowas-in 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.5.2.1 activate
#    neighbor 32.6.2.1 activate
#    neighbor 32.7.2.1 activate
#    neighbor 32.8.2.1 activate
#    neighbor 21.4.1.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout

# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#       address 172.16.80.23
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.5.2.2
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.6.2.2
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 32.7.2.2
#         netmask 255.255.255.0
# auto eth4
# iface eth4 inet static
#         address 32.8.2.2
#         netmask 255.255.255.0
# auto eth5
# iface eth5 inet static
#         address 21.4.1.1
#         netmask 255.255.255.0
# auto eth6
# iface eth6 inet static
#         address 10.0.80.23
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114



# # 2.2
# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65020
#  bgp router-id 20.0.0.3
#  neighbor 32.1.2.1 remote-as 65030
#  neighbor 32.1.2.1 advertisement-interval 1
#  neighbor 32.2.2.1 remote-as 65030
#  neighbor 32.2.2.1 advertisement-interval 1
#  neighbor 32.3.2.1 remote-as 65030
#  neighbor 32.3.2.1 advertisement-interval 1
#  neighbor 32.4.2.1 remote-as 65030
#  neighbor 32.4.2.1 advertisement-interval 1
#  neighbor 21.3.1.2 remote-as 65012
#  neighbor 21.3.1.2 advertisement-interval 1
#  neighbor 32.1.2.1 allowas-in 1
#  neighbor 32.2.2.1 allowas-in 1
#  neighbor 32.3.2.1 allowas-in 1
#  neighbor 32.4.2.1 allowas-in 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.1.2.1 activate
#    neighbor 32.2.2.1 activate
#    neighbor 32.3.2.1 activate
#    neighbor 32.4.2.1 activate
#    neighbor 21.3.1.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout

# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#       address 172.16.80.22
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.1.2.2
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.2.2.2
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 32.3.2.2
#         netmask 255.255.255.0
# auto eth4
# iface eth4 inet static
#         address 32.4.2.2
#         netmask 255.255.255.0
# auto eth5
# iface eth5 inet static
#         address 21.3.1.1
#         netmask 255.255.255.0
# auto eth6
# iface eth6 inet static
#         address 10.0.80.22
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# # 2.1
# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65020
#  bgp router-id 20.0.0.2
#  neighbor 32.5.1.1 remote-as 65030
#  neighbor 32.5.1.1 advertisement-interval 1
#  neighbor 32.6.1.1 remote-as 65030
#  neighbor 32.6.1.1 advertisement-interval 1
#  neighbor 32.7.1.1 remote-as 65030
#  neighbor 32.7.1.1 advertisement-interval 1
#  neighbor 32.8.1.1 remote-as 65030
#  neighbor 32.8.1.1 advertisement-interval 1
#  neighbor 21.2.1.2 remote-as 65010
#  neighbor 21.2.1.2 advertisement-interval 1
#  neighbor 32.5.1.1 allowas-in 1
#  neighbor 32.6.1.1 allowas-in 1
#  neighbor 32.7.1.1 allowas-in 1
#  neighbor 32.8.1.1 allowas-in 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.5.1.1 activate
#    neighbor 32.6.1.1 activate
#    neighbor 32.7.1.1 activate
#    neighbor 32.8.1.1 activate
#    neighbor 21.2.1.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout

# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#       address 172.16.80.21
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.5.1.2
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.6.1.2
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 32.7.1.2
#         netmask 255.255.255.0
# auto eth4
# iface eth4 inet static
#         address 32.8.1.2
#         netmask 255.255.255.0
# auto eth5
# iface eth5 inet static
#         address 21.2.1.1
#         netmask 255.255.255.0
# auto eth6
# iface eth6 inet static
#         address 10.0.80.21
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114



# # 2.0
# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65020
#  bgp router-id 20.0.0.1
#  neighbor 32.1.1.1 remote-as 65030
#  neighbor 32.1.1.1 advertisement-interval 1
#  neighbor 32.2.1.1 remote-as 65030
#  neighbor 32.2.1.1 advertisement-interval 1
#  neighbor 32.3.1.1 remote-as 65030
#  neighbor 32.3.1.1 advertisement-interval 1
#  neighbor 32.4.1.1 remote-as 65030
#  neighbor 32.4.1.1 advertisement-interval 1
#  neighbor 21.1.1.2 remote-as 65010
#  neighbor 21.1.1.2 advertisement-interval 1
#  neighbor 32.1.1.1 allowas-in 1
#  neighbor 32.2.1.1 allowas-in 1
#  neighbor 32.3.1.1 allowas-in 1
#  neighbor 32.4.1.1 allowas-in 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 32.1.1.1 activate
#    neighbor 32.2.1.1 activate
#    neighbor 32.3.1.1 activate
#    neighbor 32.4.1.1 activate
#    neighbor 21.1.1.2 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout

# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#       address 172.16.80.20
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 32.1.1.2
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 32.2.1.2
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 32.3.1.2
#         netmask 255.255.255.0
# auto eth4
# iface eth4 inet static
#         address 32.4.1.2
#         netmask 255.255.255.0
# auto eth5
# iface eth5 inet static
#         address 21.1.1.1
#         netmask 255.255.255.0
# auto eth6
# iface eth6 inet static
#         address 10.0.80.20
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# # 1.0
# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#         address 172.16.80.10
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 21.1.1.2
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 21.2.1.2
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 192.168.1.1
#         netmask 255.255.255.0
# auto eth4
# iface eth4 inet static
#         address 10.0.80.10
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114


# chiefMasterAddress:10.0.1.68
# commonMasterAddress:10.0.1.67,10.0.1.69
# level:1
# position:0
# defaultMasterTimer(ms):1000
# defaultKeepaliveTimer(s):3
# ToRNodes:2
# LeafNodes:4
# SpineNodes:16
# nPods:2

# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65010
#  bgp router-id 10.0.0.1
#  neighbor 21.1.1.1 remote-as 65020
#  neighbor 21.1.1.1 advertisement-interval 1
#  neighbor 21.2.1.1 remote-as 65020
#  neighbor 21.2.1.1 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 21.1.1.1 activate
#    neighbor 21.2.1.1 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout



# # 1.2
# # The loopback network interface
# auto lo
# iface lo inet loopback

# auto eth0
# iface eth0 inet static
#       address 172.16.80.12
#         netmask 255.255.255.0
# auto eth1
# iface eth1 inet static
#         address 21.3.1.2
#         netmask 255.255.255.0
# auto eth2
# iface eth2 inet static
#         address 21.4.1.2
#         netmask 255.255.255.0
# auto eth3
# iface eth3 inet static
#         address 192.168.3.1
#         netmask 255.255.255.0
# auto eth4
# iface eth4 inet static
#         address 10.0.80.12
#         netmask 255.255.0.0
#         network 10.0.0.0
#         broadcast 10.0.255.255
#         gateway 10.0.0.1
#         # dns-* options are implemented by the resolvconf package, if installed
#         dns-nameservers 114.114.114.114

# ! -*- bgp -*-
# !
# ! BGPd sample configuratin file
# !
# ! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $
# !
# hostname bgpd
# password zebra
# debug bgp
# debug bgp fsm
# debug bgp events
# debug bgp updates
# !enable password please-set-at-here
# !
# !bgp mulitple-instance
# !
# router bgp 65012
#  bgp router-id 10.0.0.3
#  neighbor 21.3.1.1 remote-as 65020
#  neighbor 21.3.1.1 advertisement-interval 1
#  neighbor 21.4.1.1 remote-as 65020
#  neighbor 21.4.1.1 advertisement-interval 1
#  maximum-paths 64
#    redistribute connected
#  address-family ipv4 unicast
#    neighbor 21.3.1.1 activate
#    neighbor 21.4.1.1 activate
#   exit-address-family
# !
# ! access-list all permit any
# !
# !route-map set-nexthop permit 10
# ! match ip address all
# ! set ip next-hop 10.0.0.1
# !
# log file /home/guolab/bgpd.log
# !
# log stdout