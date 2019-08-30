# 准备实验环境，确保不会启动重复的进程
pssh -i -h /home/guolab/InfocomPCHost.txt "rm /home/guolab/output/throughputRatio.txt transmissionTime.txt serverRecord.txt;"
pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
for ((i=1;i<=1;i++))
do
killall -9 CentralizedRoute
pssh -i -h /home/guolab/InfocomHost.txt "killall -9 CentralizedRoute;"
pssh -i -h /home/guolab/InfocomHost.txt -t 0 "rm /home/guolab/output/*;"
rm -rf /home/guolab/output/center.log
/home/guolab/CentralizedRouteTest/CentralizedRoute 180 6666 0 0 2 4 16 2 1000000 & # 100000us是默认的定时器
sleep 3
pssh -i -h /home/guolab/InfocomHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 1000000;" & # 启动Node上的集中式
echo "初始化路由"
pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"



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
# sleep 9
# # # 启动linkchange的服务端
# # echo "启动linkchange-server"
# # pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
# # sleep 1
# # echo "启动linkchange-client"
# # pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client center 10.0.8.79 667 4 900000;" &
# # pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client center 10.0.8.79 667 1 5000000;" &
# sleep 30
# echo "停止发送流量"
# pssh -i -H "root@10.0.8.83" -t 0 "killall udp-client-my-A;killall udp-client-my-B;"
# pssh -i -H "root@10.0.8.84" -t 0 "killall udp-server-my;/home/guolab/udpThroughputAnalysis 1 0.05;" 
# pssh -i -H "root@10.0.8.96" -t 0 "killall udp-server-my;/home/guolab/udpThroughputAnalysis 1 0.05;"



# # client-server iperf
pssh -i -H "root@10.0.8.86" -t 0 "killall iperf;rm /home/guolab/output/serverRecord.txt;"
pssh -i -H "root@10.0.8.98" -t 0 "killall iperf;rm /home/guolab/output/serverRecord.txt;"
pssh -i -H "root@10.0.2.89" -t 0 "killall iperf;"
pssh -i -H "root@10.0.2.90" -t 0 "killall iperf;"
# # 启动server和client
# # 启动server
sleep 10
echo "启动iperf-server"
pssh -i -H "root@10.0.8.86" -t 0 "sudo iperf -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
pssh -i -H "root@10.0.8.98" -t 0 "sudo iperf -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
sleep 10
echo "启动iperf-client"
# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 35;" &
# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 1500000000;" &
pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 1500000000;" &
sleep 3
# # 启动linkchange的服务端
echo "启动linkchange-server"
pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
sleep 1
echo "启动linkchange-client"
pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client center 10.0.8.79 667 6 900000;" &
sleep 1
pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client center 10.0.8.79 667 1 300000;" &
sleep 50
echo "停止发送流量"
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"



# # common
pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
killall -9 CentralizedRoute
pssh -i -h /home/guolab/InfocomHost.txt "killall -9 CentralizedRoute;"
sleep 10
done
# #####################################

killall -9 CentralizedRoute
pssh -i -h /home/guolab/WCMPHost.txt "killall -9 CentralizedRoute;"
rm -rf /home/guolab/output/center.log
/home/guolab/CentralizedRouteTest/CentralizedRoute 192.168.80.0 6666 0 0 2 4 16 2 1000000 &
sleep 3
pssh -i -h /home/guolab/WCMPHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 192.168.80.0 6666 2 4 16 2 1000000;" & 

pssh -i -h /home/guolab/WCMPHost.txt "chmod 777 -R /home/guolab/output/;"

pssh -i -H "root@192.168.80.13" -t 0 "killall iperf3;"
pssh -i -H "root@192.168.80.14" -t 0 "killall iperf3;"
pssh -i -H "root@192.168.80.15" -t 0 "killall iperf3;rm /home/guolab/output/serverRecord*.txt;"
pssh -i -H "root@192.168.80.16" -t 0 "killall iperf3;rm /home/guolab/output/serverRecord*.txt;"
pssh -i -H "root@192.168.80.7" -t 0 "killall linkchange-server;"
pssh -i -H "root@192.168.80.1" -t 0 "killall linkchange-client;"
sleep 10
echo "启动iperf-server"
# pssh -i -H "root@192.168.80.15" -t 0 "sudo iperf3 -i 0.5 -s > /home/guolab/output/serverRecord.txt;" &
# pssh -i -H "root@192.168.80.16" -t 0 "sudo iperf3 -i 0.5 -s > /home/guolab/output/serverRecord.txt;" &
pssh -i -H "root@192.168.80.13" -t 0 "/home/guolab/udp-server-my 767;" &
sleep 20
echo "启动iperf-client"
# pssh -i -H "root@192.168.80.13" -t 0 "sudo iperf3 -c 192.168.3.2 -t 50 -P 100 -w 200K;" &
# pssh -i -H "root@192.168.80.14" -t 0 "sudo iperf3 -c 192.168.4.2 -t 50 -P 100 -w 200K;" &
pssh -i -H "root@192.168.80.15" -t 0 "/home/guolab/udp-client-my 192.168.1.2 767 2048;"

# sleep 7
# # # pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 down;"
# pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 down;"

# sleep 5
# echo "启动linkchange-server"
# pssh -i -H "root@192.168.80.7" -t 0 "/home/guolab/linkchange-server 667;" &
# sleep 2
# echo "启动linkchange-client"
# pssh -i -H "root@192.168.80.1" -t 0 "/home/guolab/linkchange-client center 192.168.80.7 667 1 30000000;" &

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
scp root@192.168.80.15:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord1.txt
scp root@192.168.80.16:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord2.txt
/home/guolab/iperfMaxMinAnalysis 0.5
chmod 777 -R /home/guolab/output/*
pssh -i -H "root@192.168.80.7" -t 0 "ifconfig eth4 up;"
pssh -i -H "root@192.168.80.2" -t 0 "ifconfig eth2 up;"
pssh -i -H "root@192.168.80.5" -t 0 "ifconfig eth3 up;"
sleep 2
killall -9 CentralizedRoute
pssh -i -h /home/guolab/WCMPHost.txt "killall -9 CentralizedRoute;"



