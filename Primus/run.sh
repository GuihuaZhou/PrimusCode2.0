# route del -net 192.168.1.0/24
route del -net 192.168.2.0/24
route del -net 192.168.3.0/24
route del -net 192.168.4.0/24
rm /home/tencent/Primus/Primus
g++ -std=c++0x tcp-server-route.cc tcp-client-route.cc ipv4-global-routing.cc udp-bcast.cc node.cc init.cc -o Primus -lpthread # && g++ modifyRun.cc -o modifyRun
# g++ -std=c++0x tcp-server-route.cc tcp-client-route.cc ipv4-global-routing.cc udp-bcast.cc node.cc init.cc -o Primus -lpthread # && g++ modifyRun.cc -o modifyRun
rm /var/log/Primus.log
/home/tencent/Primus/Primus &

# g++ -std=c++0x udp-bcast.cc bcastTest.cc -o bcast -lpthread