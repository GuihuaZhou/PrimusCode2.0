# pssh -i -h /home/guolab/host/ATChost.txt "echo \'hello\'"
spineNodes=8
leafNodes=2
torNodes=1
nPods=2
#
killall -9 Primus
rm /home/guolab/LinkTable*.txt;
rm /home/guolab/PathTable*.txt;
rm /home/guolab/NodeSockTable*.txt;
rm /home/guolab/ControllerSockTable*.txt;
rm /home/guolab/PrimusLog*.txt;
rm /home/guolab/CostTime*.txt;
rm /home/guolab/NeighborTable*.txt;
rm /home/guolab/Primus/Primus;
#
pssh -i -h /home/guolab/host/ATChost.txt "killall -9 Primus;rm /home/guolab/LinkTable*.txt;
rm /home/guolab/PathTable*.txt;rm /home/guolab/NodeSockTable*.txt;rm /home/guolab/NeighborTable*.txt;
rm /home/guolab/ControllerSockTable*.txt;rm /home/guolab/PrimusLog*.txt;
rm /home/guolab/CostTime*.txt;rm /home/guolab/Primus/Primus;"
# 编译
rm /home/guolab/Primus/Primus
cd /home/guolab/Primus
g++ -std=c++0x init.cpp primus.cpp -o Primus -lpthread
cd ..
# # 传输
echo "transport"
pscp -h /home/guolab/host/ATChost.txt -l root /home/guolab/Primus/Primus /home/guolab/Primus
# # 启动
tempCommand=''
tempCommand=$tempCommand" "$torNodes" "$leafNodes" "$spineNodes" "$nPods
# 
command=''
command=$command"/home/guolab/Primus/Primus"$tempCommand
echo "Master"
$command &
echo "Node"
command=''
command=$command"pssh -i -h /home/guolab/host/ATChost.txt /home/guolab/Primus"$tempCommand
$command &