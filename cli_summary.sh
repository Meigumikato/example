#########################################################################
# File Name: cli_summary.sh
# Author: shanhai2015
#  
# Created Time: Sat 02 Mar 2019 11:24:19 AM CST
#########################################################################
#!/bin/bash


periThreadRPC=20000 #每线程请求rpc和运行参数对应
echo $perRPC
#exit 0
time=$(date "+%Y%m%d%H%M")
rm -rf old_*
mv rcount.result old_rcount.result.$time
mv  PERFORM.result  old_PERFORM.result.$time
mv  PERFORM_useTime.result old_PERFORM_useTime.result.$time
for ((i=1; i<=5; i ++))
do
    cat $i.ltxt | grep qps | grep -v "PERFORM"  >>  rcount.result
    cat $i.ltxt | grep PERFORM |  awk  -F " " '{if (  length($0) == length("PERFORM:80567,used_time: 8611 us")  ) print $0}'  >>  PERFORM.result
done


#exit 0
#awk -F " " `print $1,$2,$3,$4,$5,$6,$7,$8,$9,$10}`
#awk -v perRPC="$periThreadRPC" -F " "  '{s2+=$2;s4+=$4;s6+=$6;s8+=$8;s10+=$10} END{print "rpc_total:"s2,"timeouttotal:"s4,"errortotal:"s6,"qpstotal:"s8," Averageusetime:"s10/NR/perRPC}' rcount.result

awk -F " " '{print $2}' PERFORM.result  | sort -t" " -k 2 -rn >PERFORM_useTime.result

lines=$(cat PERFORM_useTime.result | wc -l)
echo "lines:" $lines
line95=`echo "$lines*95/100"|bc`
#echo "line95:" $line95
line95=$(($lines - $line95))
echo ">line95:" $line95

line99=`echo "$lines*99/100"|bc`
#echo "line99:" $line99
line99=$(($lines - $line99))
echo ">line99:" $line99

AvgTime=$(awk -v sline=$lines '{sum+=$1}END{print  sum/sline}' PERFORM_useTime.result)

#echo "line99:" $(cat PERFORM_useTime.result | head -$line99 | tail -1)
#echo "line95:" $(cat PERFORM_useTime.result | head -$line95 | tail -1)

echo "client ******************summary************************"
echo "line99:" $(head  -$line99 PERFORM_useTime.result  | tail -1) "us"
echo "line95:" $(head  -$line95 PERFORM_useTime.result  | tail -1) "us"
awk -v sAvgTime="$AvgTime" -F " "  '{s2+=$2;s4+=$4;s6+=$6;s8+=$8;s10+=$10} END{print "rpc_total:"s2,"timeouttotal:"s4,"errortotal:"s6,"qpstotal:"s8,"AvgTime:"sAvgTime "us"}' rcount.result

##echo  $(awk  -F " " '{print $2}' testperform.txt | sort -t" " -k 2 -rn | tail -9 | head -1)
#echo  "99line:" $(tail -$line99 PERFORM_useTime.result | head -1)
#echo  "95line:" $(tail -$line95 PERFORM_useTime.result | head -1) 



#awk -v perRCP="$perRCP" -F " " '{s2+=$2;s4+=$4;s6+=$6;s8+=$8;s10+=$10} END{print "rpc_total:"s2,"timeouttotal:"s4,"errortotal:"s6,"qpstotal:"s8," Averageusetime:"s10/NR/perRCP}' rcount.result


#echo "PERFORM:"
#cat  PERFORM.result | wc -l
#echo "SUMMARY:"
#cat rcount.result  
#cat rcount.result  | grep qps | wc -l
