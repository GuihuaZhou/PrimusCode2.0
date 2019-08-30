killall -9 CentralizedRoute
pssh -i -h /home/guolab/host.txt "killall -9 CentralizedRoute;"

cd /home/guolab/CentralizedRouteTest
./route.sh
cd ..

pssh -i -h /home/guolab/host.txt "rm /home/guolab/CentralizedRouteTest/*;"
pscp -h /home/guolab/host.txt -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
pscp -h /home/guolab/host.txt -l root /home/guolab/CentralizedRouteTest/CentralizedRoute /home/guolab/CentralizedRouteTest/

pssh -i -h /home/guolab/host.txt "rm /home/guolab/output/*;"
rm -rf /home/guolab/output/center.log
rm -rf /home/guolab/output/check.log

/home/guolab/CentralizedRouteTest/CentralizedRoute &
# pssh -i -H "root@10.0.8.84" -t 0 "/home/guolab/CentralizedRouteTest/CentralizedRoute;" &
pssh -i -h /home/guolab/host.txt -t 0 "/home/guolab/CentralizedRouteTest/CentralizedRoute;" & 
pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/output/;"

# /home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 2 4 16 2 100000 6 &
# pssh -i -h /home/guolab/host.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 100000 6;" & 
# pssh -i -h /home/guolab/host.txt "chmod 777 -R /home/guolab/output/;"


pssh -i -h /home/guolab/host.txt -t 0 "rm /usr/local/etc/center.conf;" 
pssh -i -h /home/guolab/host.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 100000 6;"

pssh -i -h /home/guolab/host.txt "reboot;"
pssh -i -h /home/guolab/host.txt "echo 'hello';"

# killall -9 CentralizedRoute
# pssh -i -h /home/guolab/InfocomHost.txt "killall -9 CentralizedRoute;"
# pssh -i -h /home/guolab/InfocomHost.txt "rm /home/guolab/CentralizedRouteTest/*;"
# cd /home/guolab/CentralizedRouteTest
# ./route.sh
# pscp -h /home/guolab/InfocomHost.txt -l root /home/guolab/CentralizedRouteTest/modifyRun /home/guolab/CentralizedRouteTest/
# pscp -h /home/guolab/InfocomHost.txt -l root /home/guolab/CentralizedRouteTest/CentralizedRoute /home/guolab/CentralizedRouteTest/
# pssh -i -h /home/guolab/InfocomHost.txt -t 0 "rm /home/guolab/output/*;"
# rm -rf /home/guolab/output/center.log
# cd ..
# /home/guolab/InfocomCenterTest.sh

# /home/guolab/CentralizedRouteTest/CentralizedRoute 10.0.1.67 6666 0 0 2 4 16 2 100000 2000000 &
# pssh -i -h /home/guolab/InfocomHost.txt -t 0 "/home/guolab/CentralizedRouteTest/modifyRun 10.0.1.67 6666 2 4 16 2 100000 2000000;" & 
# pssh -i -h /home/guolab/InfocomHost.txt "chmod 777 -R /home/guolab/output/;"


# /home/guolab/InfocomCenterTest.sh 
# pscp root@10.0.8.86:/home/guolab/output/throughputRatio.txt -r /home/guolab/output/
# pscp -H "root@10.0.8.86" -l root /home/guolab/output/throughputRatio.txt /home/guolab/output/ 
#  

scp root@192.168.80.16:/home/guolab/output/serverRecord.txt /home/guolab/output/serverRecord.txt
scp /home/guolab/CentralizedRouteTest/CentralizedRoute root@10.0.0.4:/home/tencent/CentralizedRoute