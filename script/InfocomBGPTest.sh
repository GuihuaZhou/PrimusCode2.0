# # preparation
pssh -i -h /home/guolab/InfocomPCHost.txt "rm /home/guolab/output/throughputRatio.txt;rm /home/guolab/output/transmissionTime.txt;rm /home/guolab/output/serverRecord.txt;"
pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
times=30 #每个timer的实验次数
# # bgpd-200
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-200 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-200 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 200;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 200;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-400
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-400 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-400 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 400;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 400;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-600
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-600 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-600 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 600;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 600;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-800
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-800 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-800 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 800;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 800;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-1100
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-1100 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-1100 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 1100;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 1100;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-1500
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-1500 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-1500 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 1500;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 1500;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-1900
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-1900 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-1900 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 1900;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 1900;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-2100
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-2100 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-2100 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 2100;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 2100;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-2500
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-2500 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-2500 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 2500;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 2500;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # bgpd-2900
pssh -i -H "root@10.0.8.74" -t 0 "rm /usr/local/sbin/bgpd;"
pssh -i -H "root@10.0.8.75" -t 0 "rm /usr/local/sbin/bgpd;"
pscp -H "root@10.0.8.74" -l root /home/guolab/bgpd/bgpd-2900 /usr/local/sbin/bgpd
pscp -H "root@10.0.8.75" -l root /home/guolab/bgpd/bgpd-2900 /usr/local/sbin/bgpd
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record 2900;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record 2900;"
for ((i=1;i<=times;i++))
do
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/output/*;"
	pssh -i -h /home/guolab/InfocomHost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
	pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"

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
	# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -t 40;" &
	# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -t 40;" &
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 10 900000;" &
	sleep 1
	pssh -i -H "root@10.0.8.75" -t 0 "/home/guolab/linkchange-client bgp 10.0.8.79 667 1 300000;" &
	sleep 50
	echo "停止发送流量"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputAnalysis;"
	pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfTimeAnalysis;" 
	pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfTimeAnalysis;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "killall iperf;"

	# # # common
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -h /home/guolab/InfocomPCHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
	pssh -i -h /home/guolab/InfocomHost.txt "killall zebra; killall bgpd;"
	sleep 10
done
# # center
pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/record center;"
pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/record center;"
for ((i=1;i<=times;i++))
do
	killall -9 CentralizedRoute
	pssh -i -h /home/guolab/InfocomHost.txt "killall -9 CentralizedRoute;"
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "rm /home/guolab/output/*;"
	rm -rf /home/guolab/output/center.log
	/home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 2 4 16 2 1000000 & # 100000us是默认的定时器
	sleep 3
	pssh -i -h /home/guolab/InfocomHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 1000000;" & # 启动Node上的集中式
	echo "初始化路由"
	pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
	pssh -i -H "root@10.0.8.74" -t 0 "killall linkchange-client;"
	pssh -i -H "root@10.0.8.75" -t 0 "killall linkchange-client;"
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
	pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 2000000000;" &
	pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 2000000000;" &
	sleep 3
	# # 启动linkchange的服务端
	echo "启动linkchange-server"
	pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
	sleep 1
	echo "启动linkchange-client"
	pssh -i -H "root@10.0.8.74" -t 0 "/home/guolab/linkchange-client center 10.0.8.79 667 10 900000;" &
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
# pssh -i -H "root@10.0.8.86" -t 0 "/home/guolab/iperfThroughputTotalAnalysis;"
# pssh -i -H "root@10.0.8.98" -t 0 "/home/guolab/iperfThroughputTotalAnalysis;"