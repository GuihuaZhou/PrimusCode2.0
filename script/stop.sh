echo ""
echo "stop master"
killall -9 Primus
pssh -i -h /home/guolab/host/master.txt -t 0 "killall -9 Primus;"
echo ""
echo "stop node"
pssh -i -h /home/guolab/host/ATChost.txt -t 0 "killall -9 Primus;" 
echo ""
cat /var/log/MasterLinkTable-0.0.txt
echo ""
# cat /var/log/Primus-0.0.log
echo ""
cat /var/log/NodeMapToSock-0.0.txt