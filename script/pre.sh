# ssh -O stop root@10.0.80.30 
# ssh -O stop root@10.0.80.31 
# ssh -O stop root@10.0.80.32 
# ssh -O stop root@10.0.80.33 
# ssh -O stop root@10.0.80.34 
# ssh -O stop root@10.0.80.35 
# ssh -O stop root@10.0.80.36 
# ssh -O stop root@10.0.80.37 
# # Leaf
# ssh -O stop root@10.0.80.20 
# ssh -O stop root@10.0.80.21 
# ssh -O stop root@10.0.80.22 
# ssh -O stop root@10.0.80.23 
# # ToR
# ssh -O stop root@10.0.80.10 
# ssh -O stop root@10.0.80.12 
# # master
# ssh -O stop root@10.0.1.68 
# ssh -O stop root@10.0.1.69 
# 
/home/guolab/script/stop.sh
pssh -i -h /home/guolab/host/ATChost.txt -t 0 "killall zebra; killall bgpd;"
# # Spine
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

# rm /home/guolab/output/test.log
# ssh root@10.0.80.30 "ifconfig eth1 up;"
# date +%s.%3N >> /home/guolab/output/test.log
# for ((i=1;i<=30;i++))
# do
# 	echo " "
# 	echo "-----------------"
# 	netstat -n | grep '10.0.80.30'
# 	netstat -n | grep '10.0.80.20'
# 	echo "-----------------"
# 	ssh root@10.0.80.30 "ifconfig eth1 down"
# 	ssh root@10.0.80.20 "ifconfig eth1 down"
# 	# ssh -O stop root@10.0.80.30
# 	# ssh -O stop root@10.0.80.20
# 	sleep 20
# 	netstat -n | grep '10.0.80.30'
# 	netstat -n | grep '10.0.80.20'
# 	echo "-----------------"
# 	ssh root@10.0.80.30 "ifconfig eth1 up"
# 	ssh root@10.0.80.20 "ifconfig eth1 up"
# 	echo " "
# 	# date +%s.%3N >> /home/guolab/output/test.log
# done
# date +%s.%3N >> /home/guolab/output/test.log
