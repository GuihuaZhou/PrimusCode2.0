rm /home/guolab/udp-throughput/udp-client.o
rm /home/guolab/udp-throughput/udp-client-my
rm /home/guolab/udp-throughput/udp-server.o
rm /home/guolab/udp-throughput/udp-server-my
g++ -fPIC -g -c udp-client.c
g++ -pie -rdynamic udp-client.o -o udp-client-my
g++ -fPIC -g -c udp-server.c
g++ -pie -rdynamic udp-server.o -o udp-server-my

pssh -i -H "root@192.168.80.13" -t 0 "rm /home/guolab/udp-client-my;"
pscp -H "root@192.168.80.13" -l root /home/guolab/udp-throughput/udp-client-my /home/guolab/




# pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/udp-server-my;"
# pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/udp-server-my;"
# pssh -i -H "root@10.0.2.89" -t 0 "rm /home/guolab/udp-client-my;"
# pssh -i -H "root@10.0.2.90" -t 0 "rm /home/guolab/udp-client-my;"
# pscp -H "root@10.0.8.86" -l root /home/guolab/udp-throughput/udp-server-my /home/guolab/
# pscp -H "root@10.0.8.98" -l root /home/guolab/udp-throughput/udp-server-my /home/guolab/
# pscp -H "root@10.0.2.89" -l root /home/guolab/udp-throughput/udp-client-my /home/guolab/
# pscp -H "root@10.0.2.90" -l root /home/guolab/udp-throughput/udp-client-my /home/guolab/
# pssh -i -H "root@10.0.8.84" -t 0 "rm /home/guolab/udp-server-my;"
# pssh -i -H "root@10.0.8.96" -t 0 "rm /home/guolab/udp-server-my;"
# pssh -i -H "root@10.0.8.85" -t 0 "rm /home/guolab/udp-client-my;"
# pssh -i -H "root@10.0.8.97" -t 0 "rm /home/guolab/udp-client-my;"
# pscp -H "root@10.0.8.84" -l root /home/guolab/udp-throughput/udp-server-my /home/guolab/
# pscp -H "root@10.0.8.96" -l root /home/guolab/udp-throughput/udp-server-my /home/guolab/
# pscp -H "root@10.0.8.85" -l root /home/guolab/udp-throughput/udp-client-my /home/guolab/
# pscp -H "root@10.0.8.97" -l root /home/guolab/udp-throughput/udp-client-my /home/guolab/
# g++ -fPIC -g -c linkchange-server.c 
# g++ -pie -rdynamic linkchange-server.o -o linkchange-server -lpthread

# g++ -fPIC -g -c linkchange-client.c 
# g++ -pie -rdynamic linkchange-client.o -o linkchange-client -lpthread

# cp -f /home/guolab/dce/source/ns-3-dce/udp-throughput/udp-client-my /home/guolab/dce/build/bin_dce/udp-client-my
# cp -f /home/guolab/dce/source/ns-3-dce/udp-throughput/udp-server-my /home/guolab/dce/build/bin_dce/udp-server-my
