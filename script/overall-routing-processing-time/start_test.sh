
num=0
s=200
for i in {1..50}
do
num=$[i*200]
echo ${num}
source ./SwitchMasterTest.sh ${num} 10000 10 2 1 0 
done