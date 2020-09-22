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
          muduo::CountDownLatch *p_countdown) {

  //建议CthriftClient生命期也和线程保持一致，不要一次请求创建销毁一次
  CthriftClient cthrift_client(str_svr_appkey, i32_timeout_ms);
  
  cthrift_client.SetFilterService("echo.Echo");

  std::vector<std::string> srv_tags;
  srv_tags.push_back("tag1");//自定义路由 只保留该appkey下 包含 tag1 tag2 的 server：
  srv_tags.push_back("tag2");
  cthrift_client.SetSrvTags(srv_tags);

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

  char buf[1025];  //1K数据
  memset(buf, 1, sizeof(buf));

  //for (int i = muduo::CurrentThread::tid() * 1000;
    //   i < muduo::CurrentThread::tid() * 1000 + 100; i++) {
 while(1){
     int i = 0;
     sleep(1);
    try{
      str_tmp = boost::lexical_cast<std::string>(i);
    } catch(boost::bad_lexical_cast & e) {

      cerr << "boost::bad_lexical_cast :" << e.what()
           << "i : " << i;
      continue;
    }

    sz = str_tmp.size();
    str_tmp += string(buf, 0, sizeof(buf) - 1 - sz);

    try {
      client.echo(strRet, str_tmp);
    } catch (TException &tx) {
      cerr << "ERROR: " << tx.what() << endl;
      //p_countdown->countDown();
      //return;
    }

    if (str_tmp != strRet) {
      cerr << "tid: " << muduo::CurrentThread::tid() << "strRet " << strRet
           << " str_tmp " << "test" + str_tmp << endl;
      //p_countdown->countDown();
      //return;
    }else{
      std::cout<<"PERFORM OK"<<std::endl;
    }
  }

  cout << "tid: " << muduo::CurrentThread::tid() << " END" << endl;

  p_countdown->countDown();
}

int main(int argc, char **argv) {
  //注意：请使用业务自身的appkey进行cat初始化！！！！！
  catClientInit("com.sankuai.inf.newct");


  CLOG_INIT();
  string str_svr_appkey("com.sankuai.inf.newct"); //服务端的appkey
  string str_cli_appkey("com.sankuai.inf.newct.client"); //客户端的appkey
  int32_t i32_timeout_ms = 20;

  switch (argc) {
    case 1:
      std::cout << "no input arg, use defalut" << std::endl;
      break;

    case 4:
      str_svr_appkey.assign(argv[1]);
      str_cli_appkey.assign(argv[2]);
      i32_timeout_ms = static_cast<int32_t>(atoi(argv[3]));
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

  //10个线程并发
  int32_t i32_thread_num = 10;  //线程数视任务占用CPU时间而定，建议不要超过2*CPU核数
  muduo::CountDownLatch countdown_thread_finish(i32_thread_num);
  for (int i = 0; i < i32_thread_num; i++) {
    muduo::net::EventLoopThread *pt = new muduo::net::EventLoopThread;
    pt->startLoop()->runInLoop(boost::bind(Work,
                                           str_svr_appkey, //服务端Appkey必须填写，不可为空，发现服务
                                           str_cli_appkey, //客户端Appkey必须填写，不可为空，以便于问题追踪
                                           i32_timeout_ms,
                                           &countdown_thread_finish));
  }

  countdown_thread_finish.wait();

  std::cout << "exit" << std::endl;
  CLOG_CLOSE();
  //等待一段时间，释放资源.
  sleep(1);
  return 0;
}
