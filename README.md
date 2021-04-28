# PrimusCode
Contains all files on the Primus development platform, including code and test scripts.

# Run Primus
1、OS: ubuntu or sonic

2、To build a Fat tree testbed, the rule is: each ToR in the Pod is fully connected to leafnode, and each leafnode is connected to (spinenodes/leafnodes) spinenode, that is, a single pod is fully connected to spinenode.

3、Every device (leader, slave, tor, leafnode, spinenode) needs the following configuration file. the path of the configuration file is /usr/local/etc/Primus.conf.
   
   leader:192.168.1.1 
   
   slave:172.16.1.1,172.16.1.2,172.16.1.3
   
   level:1 
   
   position:0 

4、The command to run Primus, for example:Primus 2 2 4 2 0 0 0 0 0 0 eth0

   (The parameters represent:the number of ToR in each pod, the number of leafNode in each pod, the number of all spineNodes, the number of pod, whether to print the message, whether to print the time when the master collects all the responders of all LSs, whether to print the time of node processing LS, whether the LS source node prints the time when the leader's response is received, whether the master randomly generates link change, whether the node randomly generates a local link change, and the name of the management interface)

5、We use pssh to control and start all devices (you can choose the method to start all devices by yourself), and download the code to the leader（git clone https://github.com/GuihuaZhou/PrimusCode2.0.git)

   (And you need to add the relevant information of the node in the network topology to /host/node.txt,add the relevant information of the slave to /host/master.txt,add the relevant information of the server mounted under ToR to /host/server.txt,add the relevant information of the client mounted under ToR to /host/client.txt)

6、You need to modify the relevant parameters in /script/start.sh and run to start all devices; modify the relevant parameters in /script/stop.sh and run to shut down all devices.

# Experiment
1、Switch processing time(Fig.3)

   1.1、Prepare two physical machines, one as the leader and one as the switch. In the leader's startup command, select "master randomly generates link changes" (set the 9th parameter to 1), modify the topology parameters of leader and switch (the first 4 parameters) to the topology scale that the user wants, and select "Print node Time to process LS";
   
   1.2、Start Primus on the two devices, you can get the processing time in /var/log/CostTime.txt of the switch, unit: us.

2、Overall routing processing time(Fig.4)

   2.1、Prepare two physical machines: one to run the master process, and the other to run multiple switch processes. Among them, the master process opens 9 receiving threads and 2 sending threads.
  
   2.2、The switch process is increased from 200 to 10K with a step size of 200.
   
   2.3、The experiment script is /script/overall-routing-processing-time/start_test.sh.

3、Macro-benchmark(Fig.5)

   3.1、Build a testbed as described in the paper;
   
   3.2、Add the link-related information that needs to be changed in the experiment to /tool/linkInfo-1.txt (the first two lines are the management IPs of the devices at both ends of the link, and the last two lines are the ifNames at both ends of the link);
   
   3.3、Modify the second parameter of line 150 in /script/fig5_fig6.sh to the path of linkInfo-1.txt;
   
   3.4、Run /script/fig5_fig6.sh, and you can get the job completion time in the server's /home/user/server-record.txt.

4、Routing Reaction Time upon Network Changes(Fig.7)

   4.1、Build a testbed as described in the paper;
   
   4.2、Run /script/fig7.sh；
   
   4.3、The experimental results can be obtained in /home/user/udpServerRecord-primus.txt.

5、Anatomy of Primus’s Redundancy Efficiency(Fig.6)

   5.1、Build a testbed as described in the paper;
   
   5.2、Add the link-related information that needs to be changed in the experiment to /tool/linkInfo-2.txt (the first two lines are the management IPs of the devices at both ends of the link, and the last two lines are the ifNames at both ends of the link);
   
   5.3、Modify the second parameter of line 150 in /script/fig5_fig6.sh to the path of linkInfo-2.txt;
   
   5.4、Run /script/fig5_fig6.sh, the experimental data can be obtained in the server's /home/user/server-record.txt
   
# Other 
   The directory of code is /Primus/
   
   The directory of script is /script/
   
   The directory of tool is /tool/ (Some tools need to be recompiled)
   
   The directory of equipment is /host/
   
   And we are sorry that the comments in the code may be in Chinese,and we will translate it into English in the future.
