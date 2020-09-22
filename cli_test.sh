#########################################################################
# File Name: cli_test.sh
# Author: shanhai2015
#  
# Created Time: Fri 01 Mar 2019 04:25:25 PM CST
#########################################################################
#!/bin/bash
#make clean 
#make ./
make -j8 >b.txt 2>&1
cat b.txt | grep error

kill -9 `pgrep cthrif`

#time=$(date '+%Y%m%d_%M%S' )
time=$(date "+%Y%m%d%H%M")
mkdir $time
mv ./*.ltxt $time/
rm -rf *.log
#exit 0
#echo "_________________________________________"
#kill -9 `pgrep cthrif`
bufsize=$[1024 * 1 + 1] 
echo $bufsize
serverip="10.4.231.87"
echo $serverip
#exit 0
#sleep(3)
#rpcc=60000 xiancheng 40
rpcc=50000 # threads 80
for ((i=1; i<=5; i ++))
do
   ##echo "test"
    #../bin/local_cthrift_cli_example com.sankuai.inf.newct com.sankuai.inf.newct.client 超时时间 线程数   rpc请求数量 buf大小【1024*1—+1】 serverip
   nohup  ../bin/local_cthrift_cli_example com.sankuai.inf.newct com.sankuai.inf.newct.client 25  80  $rpcc $bufsize  $serverip > $i.ltxt 2>&1 &
done

