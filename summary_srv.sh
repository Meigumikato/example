#!/bin/bash
kill -9 `pgrep cthrift`

rm -rf result_anysys.atxt.*
time=$(date "+%Y%m%d%H%M")
mv result_anysys.atxt result_anysys.atxt.$time
rm -rf result.rtxt.tmp
#echo "############################new"
#cat $i.ltxt | grep PERFORM |  awk  -F " " '{if (  length($0) == length("PERFORM:80567,used_time: 8611 us")  ) print $0}'  >>  PERFORM.result
cat ./result.rtxt | grep ECHO | awk '{if (  length($0) == length("ECHO 1551603117.190229")  ) print $0}' >>result.rtxt.tmp
cat ./result.rtxt.tmp | grep ECHO | wc -l   >> result_anysys.atxt
cat ./result.rtxt.tmp | grep ECHO | head -1 >>result_anysys.atxt
cat ./result.rtxt.tmp | grep ECHO | tail -1 >>result_anysys.atxt
echo "*****************************************************"
cat result_anysys.atxt 
echo "*************************details****************************"
total=$(sed -n 1p ./result_anysys.atxt | cut -d " " -f1)
echo "  total rpc:"$total
s_start=$(sed -n 2p ./result_anysys.atxt | cut -d " " -f2)
echo " start time:" $s_start
s_end=$(sed -n 3p ./result_anysys.atxt | cut -d " " -f2)
echo "   end time:" $s_end
s_diff=`echo "$s_end-$s_start"|bc `
echo "     s_diff:" $s_diff
qps=`echo "$total/$s_diff"|bc`
avgtime=`echo "$s_diff*1000*1000/$total"|bc`
echo "        qps:" $qps
echo "*********************summary******************************"
echo " total rpc:" $total
echo "  use time:" $s_diff "s"
#echo "   avgTime:" $avgtime "us"
echo "       qps:" $qps
 
