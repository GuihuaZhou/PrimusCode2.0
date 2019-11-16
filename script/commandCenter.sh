killall -9 CentralizedRoute # 杀死Master上的程序
# pssh -i -h /home/guolab/host/host.txt "killall -9 CentralizedRoute;" # 杀死Nodes上的程序

# cd /home/guolab/CentralizedRouteTest
# ./route.sh # 编译
# cd ..

# pssh -i -h /home/guolab/host/host.txt "rm /home/guolab/CentralizedRouteTest/*;"
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/CentralizedRouteTest/CentralizedRoute /home/guolab/CentralizedRouteTest/

# pssh -i -h /home/guolab/host/host.txt "rm /home/guolab/output/*;" # 删除已有的日志和实验结果
rm -rf /home/guolab/output/center.log
rm -rf /home/guolab/output/check.log

/home/guolab/CentralizedRouteTest/CentralizedRoute & # 执行master上的可执行文件
# pssh -i -H "root@10.0.8.84" -t 0 "/home/guolab/CentralizedRouteTest/CentralizedRoute;" &
# pssh -i -h /home/guolab/host/host.txt -t 0 "/home/guolab/CentralizedRouteTest/CentralizedRoute;" & # 运行Node上的可执行文件
# pssh -i -h /home/guolab/host/host.txt "chmod 777 -R /home/guolab/output/;"

# /home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 2 4 16 2 100000 6 &
# pssh -i -h /home/guolab/host/host.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 100000 6;" & 
# pssh -i -h /home/guolab/host/host.txt "chmod 777 -R /home/guolab/output/;"

# pssh -i -h /home/guolab/host/host.txt -t 0 "rm /usr/local/etc/center.conf;" 
# pssh -i -h /home/guolab/host/host.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 100000 6;"

# pssh -i -h /home/guolab/host/host.txt "reboot;"
# pssh -i -h /home/guolab/host/host.txt "echo 'hello';"

# # killall -9 CentralizedRoute
# # pssh -i -h /home/guolab/host/InfocomHost.txt "killall -9 CentralizedRoute;"
# # pssh -i -h /home/guolab/host/InfocomHost.txt "rm /home/guolab/CentralizedRouteTest/*;"
# # cd /home/guolab/CentralizedRouteTest
# # ./route.sh
# # pscp -h /home/guolab/host/InfocomHost.txt -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
# # pscp -h /home/guolab/host/InfocomHost.txt -l root /home/guolab/CentralizedRouteTest/CentralizedRoute /home/guolab/CentralizedRouteTest/
# # pssh -i -h /home/guolab/host/InfocomHost.txt -t 0 "rm /home/guolab/output/*;"
# # rm -rf /home/guolab/output/center.log
# # cd ..
# # /home/guolab/InfocomCenterTest.sh

# # /home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 2 4 16 2 100000 2000000 &
# # pssh -i -h /home/guolab/host/InfocomHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 100000 2000000;" & 
# # pssh -i -h /home/guolab/host/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"


# # /home/guolab/InfocomCenterTest.sh 
# # pscp root@10.0.8.86:/home/guolab/output/throughputRatio.txt -r /home/guolab/output/
# # pscp -H "root@10.0.8.86" -l root /home/guolab/output/throughputRatio.txt /home/guolab/output/ 
# #  

# scp root@192.168.80.16:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
# scp tencent@10.0.0.4:/home/tencent/CentralizedRoute /home/guolab/CentralizedRouteTest/CentralizedRoute

# scp guolab@10.0.1.67:/home/guolab/CentralizedRouteTest/* /home/tencent/CenterRouteTest/   
# scp tencent@10.0.0.6:/etc/sonic/config_db.json /home/guolab/


# route add -net 192.168.3.0/24 gw 21.1.1.0 dev eth1
# route add -net 192.168.3.0/24 dev Ethernet1

# 添加路由时 eth 改成 Ethernet1
# 不能使用 scope global
# 汇报链路时出错

# ip route add 192.168.3.0/24 nexthop via 21.1.1.0 weight 4 nexthop via 21.2.1.0 weight 4 

# # 10.0.0.4
# ## Debian mirror on Microsoft Azure
# ## Ref: http://debian-archive.trafficmanager.net/

# ## deb [trusted=yes] http://10.6.199.34/debian jessie main contrib non-free
# ## deb-src [trusted=yes] http://10.6.199.34/debian jessie main contrib non-free
# ## deb [trusted=yes] http://10.6.199.34/debian jessie-updates main
# ## deb-src [trusted=yes] http://10.6.199.34/debian jessie-updates main
# ## deb [trusted=yes] http://10.6.199.34/debian-security jessie/updates main contrib non-free
# ## deb-src [trusted=yes] http://10.6.199.34/debian-security jessie/updates main contrib non-free
# ## deb [trusted=yes] http://10.6.199.34/microsoft/sonic-dev/ jessie main

# ## 麾X认注轇~J乾F湾P溠~A轕~\佃~O以彏~P骾X apt update 轀~_度﻾L奾B彜~I轜~@襾A住¯罇ª蠾L住~V浾H注轇~J
# deb https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie main contrib non-free
# deb https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-updates main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-updates main contrib non-free
# deb https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-backports main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-backports main contrib non-free
# deb https://mirrors.tuna.tsinghua.edu.cn/debian-security jessie/updates main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian-security jessie/updates main contrib non-free


# # 10.0.0.5
# # deb http://debian-archive.trafficmanager.net/debian/ jessie main contrib non-free
# # deb-src http://debian-archive.trafficmanager.net/debian/ jessie main contrib non-free
# # deb http://debian-archive.trafficmanager.net/debian-security/ jessie/updates main contrib non-free
# # deb https://apt.dockerproject.org/repo/ ubuntu-jessie main
# # # deb-src https://apt.dockerproject.org/repo/ ubuntu-jessie main
# # # deb-src https://apt.dockerproject.org/repo/ ubuntu-jessie main
# # # deb-src https://apt.dockerproject.org/repo/ ubuntu-jessie main
# # # deb-src https://apt.dockerproject.org/repo/ ubuntu-jessie main
# # deb-src http://debian-archive.trafficmanager.net/debian-security/ jessie/updates main contrib non-free

# ## 麾X认注轇~J乾F湾P溠~A轕~\佃~O以彏~P骾X apt update 轀~_度﻾L奾B彜~I轜~@襾A住¯罇ª蠾L住~V浾H注轇~J
# deb https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie main contrib non-free
# deb https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-updates main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-updates main contrib non-free
# deb https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-backports main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ jessie-backports main contrib non-free
# deb https://mirrors.tuna.tsinghua.edu.cn/debian-security jessie/updates main contrib non-free
# ## deb-src https://mirrors.tuna.tsinghua.edu.cn/debian-security jessie/updates main contrib non-free


# route del -net 192.168.1.0/24
# route del -net 192.168.2.0/24
# route del -net 192.168.3.0/24
# route del -net 192.168.4.0/24