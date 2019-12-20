# echo ""
# echo "stop node"
# pssh -i -h /home/guolab/host/host.txt "killall -9 Primus;"
# echo ""
# echo "stop master"
# pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;"
# killall -9 Primus
# echo ""
# cat /var/log/MasterLinkTable-0.0.txt
# echo ""
# cat /var/log/Primus-0.0.log
# echo ""
# cat /var/log/NodeMapToSock-0.0.txt

###############################################
echo ""
echo "stop node"
pssh -i -h /home/guolab/host/ATChost.txt "killall -9 Primus;"
echo ""
echo "stop master"
pssh -i -h /home/guolab/host/master.txt "killall -9 Primus;"
killall -9 Primus
echo ""
cat /var/log/MasterLinkTable-0.0.txt
echo ""
cat /var/log/Primus-0.0.log
echo ""
cat /var/log/NodeMapToSock-0.0.txt