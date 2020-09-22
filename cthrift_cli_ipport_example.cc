#include <stdio.h>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <sstream>
#include <whale/cthrift_svr.h>
#include <whale/cthrift_client.h>
#include "Echo.h"

using namespace std;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace muduo;
using namespace muduo::net;

using namespace boost;
using namespace echo;

using namespace cthrift;

muduo::AsyncLogging *g_asyncLog = NULL;
boost::shared_ptr<muduo::AsyncLogging> g_sp_async_log;

void AsyncOutput(const char *msg, int len) {
  g_asyncLog->append(msg, len);
}

//注意cthrift日志为定位死循环等场景，目前不会轮转，需要使用方定期清理
void SetCthriftLog(void) {
  g_sp_async_log =
      boost::make_shared<muduo::AsyncLogging>("cthrift_cli_example",
                                              500 * 1024 * 1024); //500M切分一个文件
  g_sp_async_log->start();
  g_asyncLog = g_sp_async_log.get();

  muduo::Logger::setLogLevel(Logger::WARN); //线上运行推荐用WARN级别日志，正常情况下日志量一天不超过1K
  muduo::Logger::setTimeZone(muduo::TimeZone(8 * 3600, "CST"));
  muduo::Logger::setOutput(AsyncOutput);
}

//1K 数据echo测试
void Work(const string &str_svr_appkey,
          const string &str_cli_appkey,
          const int32_t &i32_timeout_ms,
          const int32_t &test_count,
          const int32_t buf_size,
          const string &server_ip,
          muduo::CountDownLatch *p_countdown) {

  //建议CthriftClient生命期也和线程保持一致，不要一次请求创建销毁一次
 // CthriftClient cthrift_client(str_svr_appkey, i32_timeout_ms);
  CthriftClient cthrift_client(/*"127.0.0.1"*/server_ip, 16888, i32_timeout_ms);  
  cthrift_client.SetFilterService("echo.Echo");

  //设置client appkey，方便服务治理识别来源
  if (SUCCESS != cthrift_client.SetClientAppkey(str_cli_appkey)) {
      p_countdown->countDown();
      return;
  }
  if (SUCCESS != cthrift_client.Init()) {
      p_countdown->countDown();
      return;
  }
  //确保业务EchoClient的生命期和线程生命期同，不要一次请求创建销毁一次EchoClient！！
  EchoClient
          client
          (cthrift_client.GetCthriftProtocol());
  string strRet;
  string str_tmp;
  size_t sz;

  #define  k_1 (1024+1)
  #define  k_2 (1024*4+1)
  #define  k_16 (1024*16+1)
  ///char buf[k_1];  //1K数据
  char *buf = new char[buf_size];
  //memset(buf, 1, sizeof(buf));
  memset(buf,1,buf_size);
  int j=  muduo::CurrentThread::tid() * 1;
  struct timeval g_tv_begin, g_tv_end; 
  gettimeofday(&g_tv_begin, NULL);
  std::cout<<"Thread Begin ID:"<<j<<std::endl;
  int  ct=0;
  int  err=0;
  int  timeout=0;
  for (int i = 0; i< test_count; i++) {

    try{
      str_tmp = boost::lexical_cast<std::string>(i);
    } catch(boost::bad_lexical_cast & e) {

      cerr << "boost::bad_lexical_cast :" << e.what()
           << "i : " << i;
      continue;
    }

    sz = str_tmp.size();
    str_tmp += string(buf, 0, buf_size/*sizeof(buf)*/ - 1 - sz);
    struct timeval tv_begin, tv_end;   
    gettimeofday(&tv_begin, NULL);
    ct++;
    try {
      client.echo(strRet, str_tmp);
    } catch (TException &tx) {
      cerr << "ERROR: " << tx.what() << endl;
      //p_countdown->countDown();
      //return;
      timeout++;
      continue;
    }
    gettimeofday(&tv_end, NULL);  
    //std::cout<<"PERFORM:" <<muduo::CurrentThread::tid()<<",used_time: " << (tv_end.tv_sec - tv_begin.tv_sec) * 1000 * 1000 + (tv_end.tv_usec - tv_begin.tv_usec) << " us"<<std::endl;
    if (str_tmp != strRet) {
      err++;
      cerr << "ERROR2 " << muduo::CurrentThread::tid() << "strRet != str_tmp" <<"sizeof strRet:"<< strRet.size()<< " str_tmp.size: " <<  str_tmp.size() << endl;
      continue;
      // p_countdown->countDown();
     // return;
    }else{
      std::cout<<"PERFORM:" <<muduo::CurrentThread::tid()<<",used_time: " << (tv_end.tv_sec - tv_begin.tv_sec) * 1000 * 1000 + (tv_end.tv_usec - tv_begin.tv_usec) << " us"<<std::endl;
    }
  }
 // std::cout<<std::endl;

  gettimeofday(&g_tv_end, NULL); 
  int64_t difftime=  (g_tv_end.tv_sec - g_tv_begin.tv_sec) * 1000 * 1000 + (g_tv_end.tv_usec - g_tv_begin.tv_usec); 
  double  total_s = difftime/( (1.0)*(1000*1000));

  int qps = ct/total_s;
  cout << "tid:" << muduo::CurrentThread::tid() << "END,totalrpc: "<< ct <<" ,timeout: "<<timeout<<" ,err: "<<err<<" qps: "<<qps<<" ,used_time: "<<total_s<< "s"<<std::endl;    
  delete []buf;
  p_countdown->countDown();
}

int main(int argc, char **argv) {

 // return 0;

   Timestamp timeStamp=Timestamp::now();
   std::cout<<timeStamp.toString()<<endl;
   //return 0;
     //注意：请使用业务自身的appkey进行cat初始化！！！！！
  catClientInit("com.sankuai.inf.newct");


  CLOG_INIT();
  string str_svr_appkey("com.sankuai.smartcall.smartmrcp"); //服务端的appkey
  string str_cli_appkey("com.sankuai.smartcall.smartmrcp"); //客户端的appkey
  int32_t i32_timeout_ms = 70;
  int32_t i32_thread_num = 10;
  int32_t test_count = 20000;
  int32_t buf_size = 1025;
  string server_ip="127.0.0.1";
  switch (argc) {
    case 1:
      std::cout << "no input arg, use defalut" << std::endl;
      break;

    case 8:
      str_svr_appkey.assign(argv[1]);
      str_cli_appkey.assign(argv[2]);
      i32_timeout_ms = static_cast<int32_t>(atoi(argv[3]));
      i32_thread_num = static_cast<int32_t>(atoi(argv[4]));
      test_count = static_cast<int32_t>(atoi(argv[5]));
      buf_size = static_cast<int32_t>(atoi(argv[6]));
      server_ip.assign(argv[7]);
      break;
    default:
      cerr << "prog <svr appkey> <client appkey> <timeout ms> but argc " << argc
           << endl;
      exit(-1);
  }

  std::cout << "svr appkey " << str_svr_appkey << std::endl;
  std::cout << "client appkey " << str_cli_appkey << std::endl;
  std::cout << "timeout ms " << i32_timeout_ms << std::endl;

  SetCthriftLog();   //设置框架日志输出，否则无法追踪问题!!
  int len =10;
  int i =0;

  
  //10个线程并发
  //int32_t i32_thread_num = 100;  //线程数视任务占用CPU时间而定，建议不要超过2*CPU核数
  muduo::CountDownLatch countdown_thread_finish(i32_thread_num);
  for (int i = 0; i < i32_thread_num; i++) {
    muduo::net::EventLoopThread *pt = new muduo::net::EventLoopThread;
    pt->startLoop()->runInLoop(boost::bind(Work,
                                           str_svr_appkey, //服务端Appkey必须填写，不可为空，发现服务
                                           str_cli_appkey, //客户端Appkey必须填写，不可为空，以便于问题追踪
                                           i32_timeout_ms,
                                           test_count,
                                           buf_size,
                                           server_ip,
                                           &countdown_thread_finish));
  }

  countdown_thread_finish.wait();

  std::cout << "exit" << std::endl;
  CLOG_CLOSE();
  //等待一段时间，释放资源.
  sleep(1);
  return 0;
}
