#!/bin/sh

if [ "x$1$2" == "x$1" ]
then
echo "Input arg1[CPU frequency governor] & arg2[test NIC name]"
echo "Available governors are:"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors
echo "Available NICs are:"
ifconfig
exit
fi

# see http://www.thinkwiki.org/wiki/How_to_make_use_of_Dynamic_Frequency_Scaling

cpucount=`cat /proc/cpuinfo|grep processor|wc -l`

# load the governors if compiled as modules
modprobe cpufreq_performance cpufreq_ondemand cpufreq_conservative cpufreq_powersave cpufreq_userspace

FLROOT=/sys/devices/system/cpu

i=0
while [ $i -ne $cpucount ]
do

	FLNM="$FLROOT/cpu"$i"/cpufreq/scaling_governor"
	# echo "Setting $FLNM to " $1
	echo $1 > $FLNM

	i=`expr $i + 1`
done 

# RedHat EL4 64 bit all kernel versions have cpuid and msr driver built in. 
# You can double check it by looking at /boot/config* file for the kernel installed. 
# And look for CPUID and MSR driver config option. It it says 'y' then it is 
# builtin the kernel. If it says 'm', then it is a module and modprobe is needed.

echo "check cpu freq in /proc/cpuinfo"
cat /proc/cpuinfo |grep -i mhz

echo "display current governors"
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor


#==================


sysctl net.ipv4.ip_local_port_range="15000 61000"
sysctl net.ipv4.tcp_fin_timeout=30
sysctl net.ipv4.tcp_tw_reuse=1 
sysctl net.core.somaxconn=1024
sysctl net.core.netdev_max_backlog=2000
sysctl net.ipv4.tcp_max_syn_backlog=2048
ifconfig $2 txqueuelen 5000
echo "$2: $(ifconfig $2 | grep txqueuelen)"