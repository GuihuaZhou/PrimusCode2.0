if [ "x$1$2$3$4$5$6" == "x$1$2$3$4$5" ]
then
echo "Input arg1[TestSwitchClientNum], arg2[TestLinkStateNum], arg3[Master SendThreadNum], arg4[RecvThreadNum], arg5[TestSwitchReportLS], arg6[WhichSwitchFuncAll]"
exit
fi


TestSwitchClientNum=$1
TestLinkStateNum=$2
SendThreadNum=$3
RecvThreadNum=$4
TestSwitchReportLS=$5
WhichSwitchFuncAll=$6

TestDelayOrThroughput=0
MutexCndWaitOrBusySleepWaitOrBusyYieldWait=2
TestEpoll=1

SwitchClientsSetAffinity=0
## 0: no affinity; 1: round-robin affinity; 2: random affinity

CPUFreqGovernor="powersave"
# CPUFreqGovernor="performance"

### Be careful!!! "performance" has lower performance than "powersave". 
### Don't know why 
### Maybe the governor has some bug on our workload 

ifDoSwitchCalcForOtherSwitch=0
cotestWithMasterForOtherSwitch=2
ifMonitorLocalNICChangeForOtherSwitch=0

ifDoSwitchCalcForReporter=1

changeNICInterval=2

echo -e "\n\n*******************"
if [ $TestSwitchReportLS -eq 0 ]; then
	echo "Master generate link-states; Switch $WhichSwitchFuncAll do all switch processing; Other switches only do LS recv/send..."
	localGenerateLSorGetLSFromSwitch=0
	cotestWithMasterForReporter=2
	ifMonitorLocalNICChangeForReporter=0
elif [ $TestSwitchReportLS -eq 1 ]; then
	echo "Switch $WhichSwitchFuncAll fakely generate link-states and report and do all switch processing; Other switches only do LS recv/send..."
	localGenerateLSorGetLSFromSwitch=1
	cotestWithMasterForReporter=1
	ifMonitorLocalNICChangeForReporter=0
elif [ $TestSwitchReportLS -eq 2 ]; then
	echo "Switch $WhichSwitchFuncAll monitor NIC IFs and generate link-states and report and do all switch processing; Other switches only do LS recv/send..."
	echo "NIC automatically change, with $changeNICInterval (s) change interval"
	localGenerateLSorGetLSFromSwitch=1
	cotestWithMasterForReporter=1
	ifMonitorLocalNICChangeForReporter=1
else
	echo "TestSwitchReportLS should only be 0|1|2 !"
	exit
fi
echo -e "\n"

echo "TestSwitchClientNum: $TestSwitchClientNum, TestLinkStateNum: $TestLinkStateNum, SendThreadNum: $SendThreadNum, RecvThreadNum: $RecvThreadNum" 
echo "TestDelayOrThroughput(0 dl, 1 tp): $TestDelayOrThroughput, MutexCndWaitOrBusySleepWaitOrBusyYieldWait(0 cnd_wait, 1 sleep, 2 yield): $MutexCndWaitOrBusySleepWaitOrBusyYieldWait, TestEpoll(0 no, 1 yes): $TestEpoll"
echo "SwitchClientsSetAffinity($SwitchClientsSetAffinity) 0: no affinity; 1: round-robin affinity; 2: random affinity"
echo "CPUFreqGovernor: $CPUFreqGovernor"
echo -e "*******************\n\n"

sleep 3

MasterHosts=("10.0.9.1")
MasterHostsPassword=("123.com")
MasterExeTestIP="172.31.9.1"
MasterTestNICName=("enp4s0f0")

SwitchHosts=("10.0.10.1")
SwitchHostsPassword=("123.com")
SwitchTestNICName=("enp4s0f0")

sshpass -p ${SwitchHostsPassword[0]} ssh root@${SwitchHosts[0]} "mkdir /home/guolab/LFS/NSDI/TCP-send/switch_result_TCP_send_elapsed_time_num-""$1"
sshpass -p ${SwitchHostsPassword[0]} ssh root@${SwitchHosts[0]} "mkdir /home/guolab/LFS/NSDI/TCP-recv/switch_result_TCP_recv_elapsed_time_num-""$1"
# SwitchHosts=("10.0.5.1" "10.0.10.1")
# SwitchHostsPassword=("123.com" "123.com")
# SwitchTestNICName=("enp65s0f0" "enp4s0f0")

# SwitchHosts=("10.0.5.1" "10.0.5.1" "10.0.5.1" "10.0.5.1" "10.0.10.1")
# SwitchHostsPassword=("123.com" "123.com" "123.com" "123.com" "123.com")
# SwitchTestNICName=("enp65s0f0" "enp65s0f0" "enp65s0f0" "enp65s0f0" "enp4s0f0")
# MasterHosts=("127.0.0.1")
# MasterExeTestIP="127.0.0.1"
# SwitchHosts=("127.0.0.1")
numOfMasterHosts=${#MasterHosts[@]}
numOfSwitchHosts=${#SwitchHosts[@]}

testFolderName="/home/guolab/LFS/NSDI/PrimusTestFolder/TestSwitchClientNum-""$1"
changeNICsh="changeNIC.sh"

rm changeNIC*.sh
rm switchExeStartScript*.sh

function compile_exe() {
	g++ MasterCalcSpeed.cc -o master -lm -pthread 
	g++ SwitchCalcSpeed.cc -o switch
}

function clear_hosts_and_copy_new_exes_to_them() {
	echo "init master hosts..."
	for ((i=0;i<$numOfMasterHosts;i++))
	do
		echo "init master host $i at ${MasterHosts[$i]}..."
		exe_command=''
		exe_command=$exe_command"pkill master; pkill switch;"
		exe_command=$exe_command"rm -rf $testFolderName;"
		exe_command=$exe_command"mkdir $testFolderName;"
		ssh root@${MasterHosts[$i]} $exe_command
		# echo "ssh guolab@$i..."
		# echo $exe_command

		scp ./master ./switch ./SystemSetting.sh root@${MasterHosts[$i]}:$testFolderName 1>/dev/null
		 
		sshpass -p ${MasterHostsPassword[$i]} ssh root@${MasterHosts[$i]} "bash $testFolderName/SystemSetting.sh $CPUFreqGovernor ${MasterTestNICName[$i]}" 1>/dev/null
	done

	echo "init switch hosts..."
	for ((i=0;i<$numOfSwitchHosts;i++))
	do
		echo "init switch host $i at ${SwitchHosts[$i]}..."
		exe_command=''
		exe_command=$exe_command"pkill master; pkill switch; pkill -f 'bash $testFolderName/$changeNICsh'"
		sshpass -p ${SwitchHostsPassword[$i]} ssh root@${SwitchHosts[$i]} $exe_command 1>/dev/null
		
		exe_command=''
		exe_command=$exe_command"rm -rf $testFolderName;"
		exe_command=$exe_command"mkdir $testFolderName;"
		ssh root@${SwitchHosts[$i]} $exe_command
		# echo "ssh guolab@$i..."
		# echo $exe_command

		scp ./master ./switch ./SystemSetting.sh root@${SwitchHosts[$i]}:$testFolderName 1>/dev/null
		
		sshpass -p ${SwitchHostsPassword[$i]} ssh root@${SwitchHosts[$i]} "bash $testFolderName/SystemSetting.sh $CPUFreqGovernor ${SwitchTestNICName[$i]}" 1>/dev/null
	done
}

function start_master() {
	echo "start master exe..."
	for i in "$MasterHosts"
	do
		exe_command=''
		exe_command=$exe_command"$testFolderName/master $TestEpoll $localGenerateLSorGetLSFromSwitch $TestSwitchClientNum $TestLinkStateNum $SendThreadNum $RecvThreadNum $TestDelayOrThroughput $MutexCndWaitOrBusySleepWaitOrBusyYieldWait 1>$testFolderName/master.log 2>$testFolderName/master.stderr &"
		# echo "Master start at $i ...."
		ssh root@$i $exe_command
	done
}

function start_switch() {
	echo "start switch exes..."
	for ((i=0;i<$numOfSwitchHosts;i++))
	do
		switchExeStartScript[$i]="switchExeStartScript$i.sh"
		echo "sleep 2" >> ${switchExeStartScript[$i]}
		switchNumOnHost[$i]=0
	done

	for ((i=0;i<$TestSwitchClientNum;i++))
	do
		switchhost=${SwitchHosts[$(($i%$numOfSwitchHosts))]}
		# echo "Switch $i start at host $switchhost ...."
		if [ $i -eq $WhichSwitchFuncAll ]; then
			testSwitchFuncAllOnWhichHost=$(($i%$numOfSwitchHosts))
			ifDoSwitchCalc=$ifDoSwitchCalcForReporter
			ifMonitorLocalNICChange=$ifMonitorLocalNICChangeForReporter
			cotestWithMaster=$cotestWithMasterForReporter
		else
			ifDoSwitchCalc=$ifDoSwitchCalcForOtherSwitch
			ifMonitorLocalNICChange=$ifMonitorLocalNICChangeForOtherSwitch
			cotestWithMaster=$cotestWithMasterForOtherSwitch
		fi
		
		if [ $SwitchClientsSetAffinity -eq 0 ]; then
			echo "$testFolderName/switch 1 $ifDoSwitchCalc $cotestWithMaster $i $TestLinkStateNum $MasterExeTestIP $ifMonitorLocalNICChange $TestSwitchClientNum 1>$testFolderName/switch$i.log 2>$testFolderName/switch$i.stderr &" >> ${switchExeStartScript[$(($i%$numOfSwitchHosts))]}
		elif [ $SwitchClientsSetAffinity -eq 1 ]; then
			echo "taskset -c \$((${switchNumOnHost[$(($i%$numOfSwitchHosts))]}%\$(getconf _NPROCESSORS_ONLN))) $testFolderName/switch 1 $ifDoSwitchCalc $cotestWithMaster $i $TestLinkStateNum $MasterExeTestIP $ifMonitorLocalNICChange 1>$testFolderName/switch$i.log 2>$testFolderName/switch$i.stderr &" >> ${switchExeStartScript[$(($i%$numOfSwitchHosts))]}
		else
			echo "taskset -c \$(($RANDOM%\$(getconf _NPROCESSORS_ONLN))) $testFolderName/switch 1 $ifDoSwitchCalc $cotestWithMaster $i $TestLinkStateNum $MasterExeTestIP $ifMonitorLocalNICChange 1>$testFolderName/switch$i.log 2>$testFolderName/switch$i.stderr &" >> ${switchExeStartScript[$(($i%$numOfSwitchHosts))]}
		fi
		temp=$(($i%1000))
		if [ $temp -eq 0 ]; then
			# echo "insert sleep 2 seconds"
			sleep 1
			# echo "sleep 2" >> ${switchExeStartScript[$(($i%$numOfSwitchHosts))]}
		fi
		switchNumOnHost[$(($i%$numOfSwitchHosts))]=$((${switchNumOnHost[$(($i%$numOfSwitchHosts))]}+1))
	done

	for ((i=0;i<$numOfSwitchHosts;i++))
	do
		# echo "---- ssh ${SwitchHosts[$i]} ----"
		echo "${SwitchHosts[$i]}: switchNumOnHost[$(($i%$numOfSwitchHosts))]=${switchNumOnHost[$(($i%$numOfSwitchHosts))]}"
		# cat ${switchExeStartScript[$i]}
		scp ${switchExeStartScript[$i]} root@${SwitchHosts[$i]}:$testFolderName 1>/dev/null
		sshpass -p ${SwitchHostsPassword[$i]} ssh root@${SwitchHosts[$i]} "bash $testFolderName/${switchExeStartScript[$i]}"
	done
}

function changeNICPeriodically() {
	### change NIC for 10 times more to ensure master and switch exit gracefully
	for ((i=0;i<$(($TestLinkStateNum+10));i++))
	do
		temp=$(($i%2))
		if [ $temp -eq 0 ]; then
			echo "ifconfig eno2 down" >> $changeNICsh
		else
			echo "ifconfig eno2 up" >> $changeNICsh
		fi

		if [ $changeNICInterval -gt 0 ]; then
			echo "sleep $changeNICInterval" >> $changeNICsh
		fi
	done
	echo "switch host ${SwitchHosts[$testSwitchFuncAllOnWhichHost]} NIC eno2 up/down periodically"
	scp $changeNICsh root@${SwitchHosts[$testSwitchFuncAllOnWhichHost]}:$testFolderName
	sshpass -p ${SwitchHostsPassword[$testSwitchFuncAllOnWhichHost]} ssh root@${SwitchHosts[$testSwitchFuncAllOnWhichHost]} "nohup bash $testFolderName/$changeNICsh > /dev/null 2>&1 &"
	echo "switch host ${SwitchHosts[$testSwitchFuncAllOnWhichHost]} NIC eno2 up/down periodically. Now begin..."
}

compile_exe
clear_hosts_and_copy_new_exes_to_them
start_master
sleep 5
start_switch
sleep 20
if [ $TestSwitchReportLS -eq 2 ]; then
	changeNICPeriodically
fi
r1=`ssh root@${SwitchHosts[0]} /home/guolab/LFS/NSDI/check.sh`
# r2=`ssh root@${SwitchHosts[1]} /home/guolab/LFS/NSDI/check.sh`

while [ `ps -ef|grep $testFolderName/master | grep -v grep |wc -l` -gt 0 ]
do
	sleep 1
	echo "master proc till!"
done

while [[ ${r1} -gt 20 ]]
do
	r1=`ssh root@${SwitchHosts[0]} /home/guolab/LFS/NSDI/check.sh`
	echo "@${SwitchHosts[0]} switch proc till! --> ${r1}"
	sleep 1
done

# while [[ ${r2} -gt 20 ]]
# do
# 	r2=`ssh root@${SwitchHosts[1]} /home/guolab/LFS/NSDI/check.sh`
# 	echo "@${SwitchHosts[1]} switch proc till! --> ${r2}"
# 	sleep 1
# done

# ssh guolab@${MasterHosts[0]} "tail -F $testFolderName/master.log -n 1000"
# ssh guolab@${MasterHosts[0]} "cat $testFolderName/master.log | tail -n 10"