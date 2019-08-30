# pssh -i -H "root@10.0.8.83" -t 0 "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/tcpTestHost.txt -t 0 "rm /home/guolab/output/statusRecord.txt updateKernelTimeRecord.txt;"
sleep 3
# pssh -i -H "root@10.0.8.83" -t 0 "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
# pssh -i -H "root@10.0.8.83" -t 0 "chmod 777 -R /home/guolab/output/;"
sleep 5
# pssh -i -H "root@10.0.8.83" -t 0 "/home/guolab/interfaceUpAndDown eth1 1 900000;"
# pssh -i -H "root@10.0.8.72" -t 0 "/home/guolab/interfaceUpAndDown eth2 1 900000;"
echo "start to change"
sleep 1
pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
pssh -i -H "root@10.0.8.72" -t 0 "killall linkchange-client;"
pssh -i -H "root@10.0.8.79" -t 0 "/home/guolab/linkchange-server 667;" &
pssh -i -H "root@10.0.8.72" -t 0 "/home/guolab/linkchange-client 10.0.8.79 667 2 900000;" &
# pssh -i -h /home/guolab/tcpTestHost.txt -t 0 "/home/guolab/interfaceUpAndDown eth1 2 4000000;" #此处的eth1没有意义
pssh -i -h /home/guolab/tcpTestHost.txt -t 0 "chmod 777 -R /home/guolab/output/;"
sleep 10
pssh -i -H "root@10.0.8.79" -t 0 "killall linkchange-server;"
pssh -i -H "root@10.0.8.72" -t 0 "killall linkchange-client;"
# pssh -i -H "root@10.0.8.83" -t 0 "killall zebra; killall bgpd;"