#include <stdio.h>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/async/TAsyncChannel.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <sstream>
#include <whale/cthrift_svr.h>
#include <whale/cthrift_client.h>
#include <whale/cthrift_client_channel.h>
#include <whale/cthrift_async_callback.h>
#include <whale/auth_sdk/cthrift_auth.h>

using namespace std;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;

using namespace muduo;
using namespace muduo::net;

using namespace boost;

using namespace cthrift;

muduo::AsyncLogging *g_asyncLog = NULL;
boost::shared_ptr<muduo::AsyncLogging> g_sp_async_log;

void AsyncOutput(const char *msg, int len) {
  g_asyncLog->append(msg, len);
}

//注意cthrift日志为定位死循环等场景，目前不会轮转，需要使用方定期清理
void SetCthriftLog(void) {
  g_sp_async_log =
      boost::make_shared<muduo::AsyncLogging>("cthrift_async_example",
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

  bool retServer = InitAuth(str_svr_appkey);


  bool retClient = InitAuth(str_cli_appkey);

  if( !retServer ||  !retClient){
    cout << "init error: " << endl;
    return;
  }

  for(int i = 0; i< 100000; i++){

    struct timeval tv_begin, tv_end;
    gettimeofday(&tv_begin, NULL);

    std::string sig;
    SignForRequest(str_cli_appkey, "test.servicename", sig);

    bool success = IsAuthPassed(str_svr_appkey, AuthAppkey, "127.0.0.1", sig); //签名是否通过

    gettimeofday(&tv_end, NULL);
    std::cout<<"PERFORM: " << ", used time : " << (tv_end
        .tv_sec - tv_begin.tv_sec) * 1000 * 1000 + (tv_end.tv_usec - tv_begin
        .tv_usec) << " us"<<std::endl;

    std::cout<< "sig : " <<  sig  << " ret: " << success << std::endl;
  }


  cout << "tid: " << muduo::CurrentThread::tid() << " END" << endl;

  p_countdown->countDown();
}


int main(int argc, char **argv) {
  CLOG_INIT();
  string str_svr_appkey("com.sankuai.inf.newct"); //服务端的appkey
  string str_cli_appkey("com.sankuai.inf.client"); //客户端的appkey
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
