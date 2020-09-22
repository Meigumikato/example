#!/bin/bash

#kill -9 `pgrep cthrift`

#time=$(date "+%Y%m%d%H%M")

#mv result.rtxt result.rtxt.$time
#mkdir $time
#mv logs $time/
#rm -rf *.log
#rm -rf logs
#nohup ./cthrift_svr_example com.meituan.inf.rpc.benchmark  16888 0 50[超时时间] 100000  20[work线程数] 50【业务线程数】  >result.rtxt  2>&1 &
#nohup ./cthrift_svr_example com.meituan.inf.rpc.benchmark  16888 0 50 100000  40 50 >result.rtxt  2>&1 &
#nohup ./cthrift_svr_async_example  com.meituan.inf.rpc.benchmark  16888 0 50 100000  20 50 >result.rtxt  2>&1 &
##########################################################
kill -9 `pgrep cthrif`
rm -rf result.rtxt
rm -rf *.log
#./top.sh

usleeps=5  ##ms
iothreadnum=4
asyncqueue=5000
timeout=50
workthreads=100
workthreadsasync=10
businessthreads=110
###aysnc server best result workthreads 10 businessthreads 100  qps 18500   10/120 qps 22317

#nohup ./cthrift_svr_async_example com.meituan.inf.rpc.benchmark  16888 0 50[超时时间] 100000  20[work线程数] 50【业务线程数】 100【Max processing大小】[io 线程数] 5【usleep ms】>result.rtxt  2>&1 &
#nohup ./cthrift_svr_example com.meituan.inf.rpc.benchmark  16888 0 70 100000  100  >result.rtxt  2>&1 &
#nohup ./cthrift_svr_async_cob com.meituan.inf.rpc.benchmark  16888 0 50 100000  40  >result.rtxt  2>&1 &


#nohup ./cthrift_svr_example_usleep30 com.meituan.inf.rpc.benchmark  16888 0 70 100000  100  >result.rtxt  2>&1 &

##这个线程数比例很OK  nohup  ./cthrift_svr_async_example_usleep3  com.meituan.inf.rpc.benchmark  16888 0 80 100000  40  60  5000  4 > rserver.rtxt  2>&1 &

#nohup ./cthrift_svr_example_usleep3 com.meituan.inf.rpc.benchmark  16888 0 80 100000  60  >result.rtxt  2>&1 &
#nohup  ./cthrift_svr_async_example_usleep3  com.meituan.inf.rpc.benchmark  16888 0 80 100000  40  50  5000  4 > rserver.rtxt  2>&1 &
#nohup  ./cthrift_svr_async_example_usleep30  com.meituan.inf.rpc.benchmark  16888 0 80 100000  12  20  5000  4 > rserver.rtxt  2>&1 &


#nohup ./cthrift_svr_example  com.meituan.inf.rpc.benchmark  16888 0 $timeout 100000  $workthreads  $usleeps >result.rtxt  2>&1 &
nohup  ./cthrift_svr_async_example  com.meituan.inf.rpc.benchmark  16888 0 $timeout 100000  $workthreadsasync   $businessthreads  $asyncqueue $usleeps   > rserver.rtxt  2>&1 &
#nohup  ./cthrift_svr_async_example  com.meituan.inf.rpc.benchmark  16888 0 80 100000  40  60  5000 5   > rserver.rtxt  2>&1 &
