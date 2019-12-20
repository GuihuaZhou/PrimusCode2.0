# # # server
# pssh -i -H "root@10.0.8.86" -t 0 "route add -net 192.168.0.0/16 gw 192.168.1.1;route add -net 32.0.0.0/8 gw 192.168.1.1;route add -net 21.0.0.0/8 gw 192.168.1.1;"
# pssh -i -H "root@10.0.8.98" -t 0 "route add -net 192.168.0.0/16 gw 192.168.1.1;route add -net 32.0.0.0/8 gw 192.168.1.1;route add -net 21.0.0.0/8 gw 192.168.1.1;"
# # # client
# pssh -i -H "root@10.0.8.87" -t 0 "route add -net 192.168.0.0/16 gw 192.168.3.1;route add -net 32.0.0.0/8 gw 192.168.3.1;route add -net 21.0.0.0/8 gw 192.168.3.1;"
# pssh -i -H "root@10.0.8.89" -t 0 "route add -net 192.168.0.0/16 gw 192.168.3.1;route add -net 32.0.0.0/8 gw 192.168.3.1;route add -net 21.0.0.0/8 gw 192.168.3.1;"

###########################################################
sudo pssh -i -h /home/guolab/host/host.txt -t 0 "sysctl net.ipv4.fib_multipath_hash_policy=1;"
###########################################################

###################################################
# # # iperf3 tcp
echo "Tcp"
pssh -i -H "root@10.0.8.86" -t 0 "killall -9 iperf3;rm -rf /home/guolab/output/*;"
pssh -i -H "root@10.0.8.98" -t 0 "killall -9 iperf3;rm -rf /home/guolab/output/*;"
pssh -i -H "root@10.0.8.87" -t 0 "killall -9 iperf3;"
pssh -i -H "root@10.0.8.89" -t 0 "killall -9 iperf3;"
sleep 5
echo "启动iperf-server."
pssh -i -H "root@10.0.8.86" -t 0 "iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
pssh -i -H "root@10.0.8.98" -t 0 "iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
sleep 5
echo "启动iperf-client."
pssh -i -H "root@10.0.8.87" -t 0 "iperf3 -c 192.168.1.2 -i 0.5 -P 10 -t 50 -w 200K;" &
pssh -i -H "root@10.0.8.89" -t 0 "iperf3 -c 192.168.1.3 -i 0.5 -P 10 -t 50 -w 200K;" &
sleep 60
echo "停止iperf."
pssh -i -H "root@10.0.8.86" -t 0 "killall -9 iperf3;"
pssh -i -H "root@10.0.8.98" -t 0 "killall -9 iperf3;"
pssh -i -H "root@10.0.8.87" -t 0 "killall -9 iperf3;"
pssh -i -H "root@10.0.8.89" -t 0 "killall -9 iperf3;"

############################################################
# # # iperf3 udp
# echo "Udp"
# pssh -i -H "root@10.0.8.86" -t 0 "killall -9 iperf3;rm -rf /home/guolab/output/*;"
# pssh -i -H "root@10.0.8.98" -t 0 "killall -9 iperf3;rm -rf /home/guolab/output/*;"
# pssh -i -H "root@10.0.8.87" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.89" -t 0 "killall -9 iperf3;"
# sleep 5
# echo "启动iperf-server."
# pssh -i -H "root@10.0.8.86" -t 0 "iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# pssh -i -H "root@10.0.8.98" -t 0 "iperf3 -s -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# sleep 5
# echo "启动iperf-client."
# pssh -i -H "root@10.0.8.87" -t 0 "iperf3 -u -c 192.168.1.2 -i 0.5 -P 10 -t 50 -w 200K;" &
# pssh -i -H "root@10.0.8.89" -t 0 "iperf3 -u -c 192.168.1.3 -i 0.5 -P 10 -t 50 -w 200K;" &
# sleep 60
# echo "停止iperf."
# pssh -i -H "root@10.0.8.86" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.98" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.87" -t 0 "killall -9 iperf3;"
# pssh -i -H "root@10.0.8.89" -t 0 "killall -9 iperf3;"