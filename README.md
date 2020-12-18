# PrimusCode
Primus开发平台上的所有文件，包括代码和测试脚本

# 运行Primus
1、OS: ubuntu 16.04

2、搭建拓扑，拓扑为fattree，规则为：Pod内每个tor与leafnode全连接，每个leafnode与（spinenodes/leafnodes）个spinenode连，即单个pod与spinenode全连接

3、Primus配置文件位置：/usr/local/etc/Primus.conf（每一台设备，包括leader、slave、tor、leafnode、spinenode都需要，默认0.0作leader）
   配置文件内容：leader:192.168.1.1 # 通电时管理员指定的leader IP
               slave:172.16.1.1,172.16.1.2,172.16.1.3 # 管理员指定的slave IP
               level:1 # 0:leader or slave; 1:tor; 2:leafnode; 3:spinenode 
               position:0 # 本机所处的位置

4、启动Primus命令为：Primus 2 2 4 2 0 0 0 eth0，分别代表每个pod内的tor数量、每个pod内的leafNode数量、所有的spineNodes数量、pod个数、是否打印master收集齐所有LS的respone的时间、是否打印   node处理LS的时间、LS源node是否打印收到leader response的时间、管理口名称

5、我们使用pssh控制启动所有设备（可以自行选择启动所有设备的方法），下载代码至leader上（git clone https://github.com/GuihuaZhou/PrimusCode2.0.git），并且需要将node的相关信息添加到/host/node.txt中（有示例）、slave的相关信息添加到/host/master.txt中、tor下挂载的server的相关信息添加到/host/server.txt中、tor下挂载的client的相关信息添加到/host/client.txt中

6、修改/script/start.sh中的相关参数，运行即可启动所有的设备；修改/script/stop.sh中的相关参数，运行即可关闭所有的设备

# 实验复现