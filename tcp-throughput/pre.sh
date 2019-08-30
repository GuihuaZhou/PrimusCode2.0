rm /home/guolab/tcp-throughput/tcp-client.o
rm /home/guolab/tcp-throughput/tcp-client-my
rm /home/guolab/tcp-throughput/tcp-server.o
rm /home/guolab/tcp-throughput/tcp-server-my
g++ -fPIC -g -c tcp-client.c
g++ -pie -rdynamic tcp-client.o -o tcp-client-my
g++ -fPIC -g -c tcp-server.c
g++ -pie -rdynamic tcp-server.o -o tcp-server-my
pssh -i -H "root@10.0.8.84" -t 0 "rm /home/guolab/tcp-server-my;"
pssh -i -H "root@10.0.8.96" -t 0 "rm /home/guolab/tcp-server-my;"
pssh -i -H "root@10.0.8.86" -t 0 "rm /home/guolab/tcp-server-my;"
pssh -i -H "root@10.0.8.98" -t 0 "rm /home/guolab/tcp-server-my;"
pssh -i -H "root@10.0.8.85" -t 0 "rm /home/guolab/tcp-client-my;"
pssh -i -H "root@10.0.8.83" -t 0 "rm /home/guolab/tcp-client-my;"
pssh -i -H "root@10.0.8.97" -t 0 "rm /home/guolab/tcp-client-my;"
pssh -i -H "root@10.0.2.89" -t 0 "rm /home/guolab/tcp-client-my;"
pssh -i -H "root@10.0.2.90" -t 0 "rm /home/guolab/tcp-client-my;"
pscp -H "root@10.0.8.84" -l root /home/guolab/tcp-throughput/tcp-server-my /home/guolab/
pscp -H "root@10.0.8.96" -l root /home/guolab/tcp-throughput/tcp-server-my /home/guolab/
pscp -H "root@10.0.8.86" -l root /home/guolab/tcp-throughput/tcp-server-my /home/guolab/
pscp -H "root@10.0.8.98" -l root /home/guolab/tcp-throughput/tcp-server-my /home/guolab/
pscp -H "root@10.0.8.85" -l root /home/guolab/tcp-throughput/tcp-client-my /home/guolab/
pscp -H "root@10.0.8.83" -l root /home/guolab/tcp-throughput/tcp-client-my /home/guolab/
pscp -H "root@10.0.8.97" -l root /home/guolab/tcp-throughput/tcp-client-my /home/guolab/
pscp -H "root@10.0.2.89" -l root /home/guolab/tcp-throughput/tcp-client-my /home/guolab/
pscp -H "root@10.0.2.90" -l root /home/guolab/tcp-throughput/tcp-client-my /home/guolab/


# 潘念
# 编译
g++ -fPIC -g -c tcp-client.c
g++ -pie -rdynamic tcp-client.o -o tcp-client-my
g++ -fPIC -g -c tcp-server.c
g++ -pie -rdynamic tcp-server.o -o tcp-server-my
# 传输
pssh -i -H "root@10.0.8.60" -t 0 "rm /home/guolab/tcp-server-my;"
pssh -i -H "root@10.0.8.76" -t 0 "rm /home/guolab/tcp-client-my;"
pscp -H "root@10.0.8.60" -l root /home/guolab/tcp-throughput/tcp-server-my /home/guolab/
pscp -H "root@10.0.8.76" -l root /home/guolab/tcp-throughput/tcp-client-my /home/guolab/
# 执行
pssh -i -H "root@10.0.8.60" -t 0 "/home/guolab/tcp-server-my;" &
sleep 3
pssh -i -H "root@10.0.8.76" -t 0 "/home/guolab/tcp-client-my 32.1.1.0 3072;"




# # gcc -fPIC -g -c tcp-client.c
# # gcc -pie -rdynamic tcp-client.o -o tcp-client-my
# # gcc -fPIC -g -c tcp-server.c
# # gcc -pie -rdynamic tcp-server.o -o tcp-server-my
# cp -f /home/guolab/tcp-throughput/tcp-client-my /home/guolab/tcp-throughput/tcp-client-my
# cp -f /home/guolab/tcp-throughput/tcp-server-my /home/guolab/tcp-throughput/tcp-server-my