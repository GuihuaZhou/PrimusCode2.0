rm /home/guolab/modifyBGPConf
pssh -i -h /home/guolab/host/host.txt "rm /home/guolab/modifyBGPConf;"
g++ modifyBGPConf.cc -o modifyBGPConf
pscp -h /home/guolab/host/host.txt -l root /home/guolab/tool/modifyBGPConf /home/guolab/
pssh -i -h /home/guolab/host/host.txt "/home/guolab/modifyBGPConf 5 2 4 16 2;"