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

# sudo su
# 123.com

# cat /etc/network/interfaces

# vi /usr/local/etc/Primus.conf
# masterAddress:10.0.1.67
# masterPort:6666
# level:1
# position:0
# defaultMasterTimer(ms):1000
# defaultKeepaliveTimer(s):3
# ToRNodes:2
# LeafNodes:4
# SpineNodes:16
# nPods:2


# ifconfig eth2 down

# ifconfig eth3 down
# ifconfig eth4 down

# ifconfig eth2 up
# ifconfig eth3 up
# ifconfig eth4 up

# cat /var/log/Primus.log
# cat /var/log/PathEntryTable.txt

# pssh -i -h /home/guolab/host/host.txt "echo 'hello';"
# pssh -i -h /home/guolab/host/host.txt "reboot;"
# pssh -i -h /home/guolab/host/host.txt "rm -rf /var/log/Primus.txt;"
# vi /etc/network/interfaces

# touch /usr/local/etc/Primus.conf
# vi /usr/local/etc/Primus.conf

# masterAddress:10.0.1.67
# masterPort:6666
# level:1
# position:3
# defaultMasterTimer(ms):1000
# defaultKeepaliveTimer(s):3
# ToRNodes:2
# LeafNodes:4
# SpineNodes:16
# nPods:2


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

# # 删除一些日志文件
pssh -i -h /home/guolab/host/ATChost.txt "rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;"
pssh -i -h /home/guolab/host/ATChost.txt "rm /var/log/NodeMapToSock*.txt;rm /var/log/NodeInDirPathTable*.txt;rm /var/log/MasterMapToSock*.txt;"

pssh -i -h /home/guolab/host/master.txt "rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;"
pssh -i -h /home/guolab/host/master.txt "rm /var/log/NodeMapToSock*.txt;rm /var/log/MasterLinkTable*.txt;rm /var/log/ClusterMasterInfo*.txt;rm /var/log/MasterInDirPathTable*.txt;"

rm /var/log/Primus*.log
rm /var/log/PathEntryTable*.txt
rm /var/log/MappingTable*.txt
rm /var/log/NodeMapToSock*.txt
rm /var/log/MasterLinkTable*.txt
rm /var/log/ClusterMasterInfo*.txt;
rm /var/log/MasterInDirPathTable*.txt;
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
pssh -i -H "root@10.0.1.68" -t 0 "/home/guolab/Primus/Primus;" & # chief Master
sleep 3
/home/guolab/Primus/Primus &
pssh -i -H "root@10.0.1.69" -t 0 "/home/guolab/Primus/Primus;" &

# /home/guolab/Primus/Primus &
# sleep 3
# pssh -i -h /home/guolab/ATChost/master.txt -t 0 "/home/guolab/Primus/Primus;" &
sleep 5
echo ""
echo "start node."
echo ""
pssh -i -h /home/guolab/host/ATChost.txt -t 0 "/home/guolab/Primus/Primus;" & # 运行Node上的可执行文件

# pssh -i -h /home/guolab/host/ATChost.txt "echo 'hello';"
# pssh -i -h /home/guolab/host/ATChost.txt "reboot;"
########################## ##########################