# pssh -i -h /home/guolab/host//host.txt "rm /home/guolab/modifyZebraConf;"
# pssh -i -h /home/guolab/host/host.txt "rm /home/guolab/modifyBGPConf;"
# g++ modifyZebraConf.cc -o modifyZebraConf
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/modifyZebraConf /home/guolab/
# g++ modifyBGPConf.cc -o modifyBGPConf
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/modifyBGPConf /home/guolab/
# pssh -i -h /home/guolab/host/host.txt -t 0 "rm /home/guolab/output/bgpd.log;"
# pssh -i -h /home/guolab/host/host.txt "/home/guolab/modifyZebraConf;"
# 修改BGP配置文件，adv interval
# pssh -i -h /home/guolab/host/host.txt "/home/guolab/modifyBGPConf 0 2 4 16 2;"
# pssh -i -h /home/guolab/host/host.txt "reboot;"
# pssh -i -h /home/guolab/host/host.txt "echo \'Hello\';"
# pssh -i -h /home/guolab/host/host.txt -t 0 "rm /home/guolab/output/updateBGPRIBTimeRecord.txt;rm /home/guolab/output/updateKernelTimeRecord.txt;"
# pssh -i -h /home/guolab/host/host.txt -t 0 "rm /usr/local/sbin/bgpd;rm /usr/local/sbin/zebra;"
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/bgpd/bgpd /usr/local/sbin/bgpd
# pscp -h /home/guolab/host/host.txt -l root /home/guolab/bgpd/zebra /usr/local/sbin/zebra

# pssh -i -h /home/guolab/host/InfocomHost.txt -t 0 "rm /usr/local/sbin/bgpd"
# pscp -h /home/guolab/host/InfocomHost.txt -l root /home/guolab/bgpd/bgpd /usr/local/sbin/bgpd

# pssh -i -h /home/guolab/host/InfocomPCHost.txt -t 0 "killall iperf;"
# pssh -i -H "root@10.0.8.86" -t 0 "sudo iperf -s -P 5 -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# pssh -i -H "root@10.0.8.98" -t 0 "sudo iperf -s -P 5 -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# sleep 3
# pssh -i -H "root@10.0.2.89" -t 0 "sudo iperf -c 192.168.1.2 -i 0.5 -n 3000000000 -P 5;" &
# pssh -i -H "root@10.0.2.90" -t 0 "sudo iperf -c 192.168.2.2 -i 0.5 -n 3000000000 -P 5;" &
# sleep 5
# pssh -i -h /home/guolab/host/InfocomHost.txt -t 0 "killall iperf;"
# pssh -i -H "root@10.0.8.74" -t 0 "sudo iperf -s -u -i 0.5 > /home/guolab/output/serverRecord.txt;" &
# sleep 2
# pssh -i -H "root@10.0.8.79" -t 0 "sudo iperf -c 32.15.1.0 -B 32.15.1.1 -i 0.5 -t 10 -u -b 0.5M;" &

pssh -i -h /home/guolab/host/host.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/host/host.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/host/host.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/host/host.txt "chmod 777 -R /home/guolab/output/;"

sudo su
123.com

vi /usr/local/etc/bgpd.conf
cat /usr/local/etc/bgpd.conf

