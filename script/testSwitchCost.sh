rm /home/guolab/Primus/Primus
cd /home/guolab/Primus
g++ -std=c++0x tcp.cc udp.cc ipv4-global-routing.cc init.cc -o Primus -lpthread
pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus/Primus
pssh -i -h /home/guolab/host/ATChost.txt "rm /home/guolab/output/primusStamp.txt;rm /var/log/Primus*.log;rm /var/log/PathEntryTable*.txt;rm /var/log/MappingTable*.txt;rm /var/log/NodeMapToSock*.txt;rm /var/log/NodeInDirPathTable*.txt;rm /var/log/MasterMapToSock*.txt;rm /var/log/NodeLinkTable*.txt;"
