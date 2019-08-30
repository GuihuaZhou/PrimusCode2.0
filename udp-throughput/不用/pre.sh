gcc -fPIC -g -c udp-client.c
gcc -pie -rdynamic udp-client.o -o udp-client-my
gcc -fPIC -g -c udp-server.c
gcc -pie -rdynamic udp-server.o -o udp-server-my
# cp -f /home/guolab/dce/source/ns-3-dce/udp-throughput/udp-client-my /home/guolab/dce/build/bin_dce/udp-client-my
# cp -f /home/guolab/dce/source/ns-3-dce/udp-throughput/udp-server-my /home/guolab/dce/build/bin_dce/udp-server-my