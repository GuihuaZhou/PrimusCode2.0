#!/bin/bash
########################### BGP ###############################
pssh -i -h /home/guolab/host/ATChost.txt "ifup -a;"
pssh -i -h /home/guolab/host/ATChost.txt "killall zebra; killall bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/*;"
pssh -i -h /home/guolab/host/ATChost.txt "zebra -d;bgpd -d;ps -e|grep zebra; ps -e|grep bgpd;"
pssh -i -h /home/guolab/host/ATChost.txt "chmod 777 -R /home/guolab/bgpd.log;"