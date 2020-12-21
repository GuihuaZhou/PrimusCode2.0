# PrimusCode
Primus开发平台上的所有文件，包括代码和测试脚本

# 运行Primus
1、OS: ubuntu or sonic

2、搭建testbed，拓扑为fattree，规则为：Pod内每个tor与leafnode全连接，每个leafnode与（spinenodes/leafnodes）个spinenode连，即单个pod与spinenode全连接

3、Primus配置文件位置：/usr/local/etc/Primus.conf（每一台设备，包括leader、slave、tor、leafnode、spinenode都需要，默认0.0作leader）
   配置文件内容：
   leader:192.168.1.1 # 通电时管理员指定的leader IP
   slave:172.16.1.1,172.16.1.2,172.16.1.3 # 管理员指定的slave IP
   level:1 # 0:leader or slave; 1:tor; 2:leafnode; 3:spinenode 
   position:0 # 本机所处的位置

4、启动Primus命令，例如：Primus 2 2 4 2 0 0 0 0 0 eth0
   分别代表每个pod内的tor数量、每个pod内的leafNode数量、所有的spineNodes数量、pod个数、是否打印master收集齐所有LS的respone的时间、是否打印node处理LS的时间、LS源node是否打印收到leader response的时间、master是否随机产生链路变化、node是否随机产生本地链路变化、管理接口名称

5、我们使用pssh控制启动所有设备（可以自行选择启动所有设备的方法），下载代码至leader上（git clone https://github.com/GuihuaZhou/PrimusCode2.0.git），并且需要将node的相关信息添加到/host/node.txt中（有示例）、slave的相关信息添加到/host/master.txt中、tor下挂载的server的相关信息添加到/host/server.txt中、tor下挂载的client的相关信息添加到/host/client.txt中

6、修改/script/start.sh中的相关参数，运行即可启动所有的设备；修改/script/stop.sh中的相关参数，运行即可关闭所有的设备。

7、代码目录：/Primus/
   脚本目录：/script/
   工具目录：/tool/（部分工具需要重新编译）
   设备地址目录：/host/

# 实验复现
1、Switch processing time(Fig.3)
   1.1、准备两台物理机，一台作leader，一台作switch。leader的启动命令中选择“master随机产生链路变化”（即第8个参数置为1），leader和switch的拓扑参数（前4个参数）修改为用户想要的拓扑规模；
   1.2、启动两台设备上的Primus，在switch的/home/user/CostTime.txt中可以获得processing time，单位：us。

2、Overall routing processing time(Fig.4)

3、Macro-benchmark(Fig.5)
   3.1、搭建如paper描述的testbed；
   3.2、将实验中需要变化的链路相关信息添加到/tool/linkInfo-1.txt中（分别是链路两端的IP和ifName）；
   3.3、修改/script/fig5_fig6.sh中第150行的第2个参数为linkInfo-1.txt的路径；
   3.4、运行/script/fig5_fig6.sh，在server的/home/user/server-record.txt中可以获得job completion time。

4、Routing Reaction Time upon Network Changes(Fig.7)
   4.1、搭建如paper描述的testbed；
   4.2、运行/script/fig7.sh；
   4.3、在/home/user/udpServerRecord-primus.txt中可以获得实验结果。

5、Anatomy of Primus’s Redundancy Efficiency(Fig.6)
   5.1、搭建如paper描述的testbed；
   5.2、将实验中需要变化的链路相关信息添加到/tool/linkInfo-2.txt中（分别是链路两端的IP和ifName，把控制链路也添加进去）；
   5.3、修改/script/fig5_fig6.sh中第150行的第2个参数为linkInfo-2.txt的路径；
   5.4、运行/script/fig5_fig6.sh，在server的/home/user/server-record.txt中可以获得实验数据。