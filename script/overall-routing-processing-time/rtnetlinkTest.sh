### change NIC for 10 times more to ensure master and switch exit gracefully
if [ "x$1$2" == "x$1$2$3" ]
then
echo "Input arg1[TestNICName], arg2[TestLinkStateNum], arg3[changeNICInterval (s)]"
exit
fi

TestNICName=$1
TestLinkStateNum=$2
changeNICInterval=$3

g++ rtnetlinkTest.cc -o rtnetlinkTest -pthread

./rtnetlinkTest $TestNICName $TestLinkStateNum 1>rtnetlinkTest.log 2>rtnetlinkTest.stderr &

for ((i=0;i<$(($TestLinkStateNum+5));i++))
do
	temp=$(($i%2))
	if [ $temp -eq 0 ]; then
		nicStatus="down"
	else
		nicStatus="up"
	fi

	if [ $i -eq $TestLinkStateNum ]; then
		echo "change NIC for 5 more times to ensure program exit gracefully..."
	fi
	echo "round $i ... ifconfig $TestNICName $nicStatus"
	ifconfig $TestNICName $nicStatus

	if [ $changeNICInterval -gt 0 ]; then
		sleep $changeNICInterval
	fi
done

cat rtnetlinkTest.log