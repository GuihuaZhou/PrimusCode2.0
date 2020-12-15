gitDirectory="/home/guolab/PrimusCode2.0"
echo ""
echo "stop master"
killall -9 Primus
killall -9 linkChange
pssh -i -h $gitDirectory/host/master.txt -t 0 "killall -9 Primus;killall -9 linkChange;"
echo ""
echo "stop node"
pssh -i -h $gitDirectory/host/ATChost.txt -t 0 "killall -9 Primus;killall -9 linkChange;" 
#
pssh -i -h $gitDirectory/host/ATChost.txt "killall zebra; killall bgpd;"
# 确保所有网卡都是正常的
# Masterß
ifconfig eth0 up
ifconfig eth1 up
ifconfig eth2 up
pssh -i -h $gitDirectory/host/master.txt "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# # Spine
# ssh root@10.0.80.30 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.31 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.32 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.33 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.34 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.35 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.36 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# ssh root@10.0.80.37 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
# Leaf
ssh root@10.0.80.20 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
ssh root@10.0.80.21 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.22 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ssh root@10.0.80.23 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;ifconfig eth3 up;ifconfig eth4 up;ifconfig eth5 up;"
# ToR
ssh root@10.0.80.10 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"
ssh root@10.0.80.11 "ifconfig eth0 up;ifconfig eth1 up;ifconfig eth2 up;"