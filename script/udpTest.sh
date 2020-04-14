# ssh root@10.0.9.1 "rm /home/guolab/udp-server;"
# ssh root@10.0.10.1 "rm /home/guolab/udp-server;"
# cd /home/guolab/tool
# g++ udp-server.c -o udp-server -pthread
# # pscp -h /home/guolab/host/ATCServerhost.txt -l root /home/guolab/tool/udp-server /home/guolab/udp-server
# scp /home/guolab/tool/udp-server root@10.0.9.1:/home/guolab/udp-server
# # 
# g++ udp-client.c -o udp-client -pthread
# # pscp -h /home/guolab/host/ATCClienthost.txt -l root /home/guolab/tool/udp-client /home/guolab/udp-client
# scp /home/guolab/tool/udp-client root@10.0.10.1:/home/guolab/udp-client
# 
ssh root@10.0.9.1 "killall -9 udp-server;rm /home/guolab/udpServerRecord.txt;"
ssh root@10.0.10.1 "killall -9 udp-client;rm /home/guolab/udpServerRecord.txt;"
sleep 5
echo "start udp server"
ssh root@10.0.9.1 "taskset -c 0 /home/guolab/udp-server 10000 /home/guolab/udpServerRecord.txt;" &
sleep 2
echo "start udp client"
ssh root@10.0.10.1 "taskset -c 0 /home/guolab/udp-client 172.31.9.1 1000 43 /home/guolab/udpClientRecord.txt;" &
sleep 10
ssh root@10.0.9.1 "killall -9 udp-server;"
ssh root@10.0.10.1 "killall -9 udp-client;"
rm /home/guolab/output/udpServerRecord-udpTest.txt
scp root@10.0.9.1:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-udpTest.txt

# # 关闭9.1 RP_filter
# net.ipv4.conf.eno1.rp_filter=0
# net.ipv4.conf.enp4s0f0.rp_filter=0
# net.ipv4.conf.enp4s0f1.rp_filter=0
# net.ipv4.conf.default.rp_filter=0
# net.ipv4.conf.all.rp_filter=0
# # 关闭10.1 RP_filter
# net.ipv4.conf.eno4.rp_filter=0
# net.ipv4.conf.enp4s0f0.rp_filter=0
# net.ipv4.conf.enp4s0f1.rp_filter=0
# net.ipv4.conf.default.rp_filter=0
# net.ipv4.conf.all.rp_filter=0

# scp /home/guolab/tool/udp-server root@10.0.5.1:/home/guolab/udp-server
# ssh root@10.0.5.1 "killall -9 udp-server;rm /home/guolab/udpServerRecord.txt;"
# ssh root@10.0.10.1 "killall -9 udp-client;rm /home/guolab/udpServerRecord.txt;"
# sleep 5
# echo "start udp server"
# ssh root@10.0.5.1 "taskset -c 0 /home/guolab/udp-server 10000 /home/guolab/udpServerRecord.txt;" &
# sleep 2
# echo "start udp client"
# ssh root@10.0.10.1 "taskset -c 0 /home/guolab/udp-client 10.0.5.1 1000 43 /home/guolab/udpClientRecord.txt;" &
# sleep 10
# ssh root@10.0.5.1 "killall -9 udp-server;"
# ssh root@10.0.10.1 "killall -9 udp-client;"
# rm /home/guolab/output/udpServerRecord-udpTest.txt
# scp root@10.0.5.1:/home/guolab/udpServerRecord.txt /home/guolab/output/udpServerRecord-udpTest.txt