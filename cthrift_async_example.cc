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
#include "Echo.h"

using namespace std;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;

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
      boost::make_shared<muduo::AsyncLogging>("cthrift_async_example",
                                              500 * 1024 * 1024); //500M切分一个文件
  g_sp_async_log->start();
  g_asyncLog = g_sp_async_log.get();

  muduo::Logger::setLogLevel(Logger::WARN); //线上运行推荐用WARN级别日志，正常情况下日志量一天不超过1K
  muduo::Logger::setTimeZone(muduo::TimeZone(8 * 3600, "CST"));
  muduo::Logger::setOutput(AsyncOutput);
}

//callback函数，业务代码实际逻辑。
void my_echo(EchoCobClient* client){
  std::string pong = "";
  try {
    //这行必不可少，请业务参考idl产生的源代码，填写revc函数
    client->recv_echo(pong);
    //业务实际的处理逻辑
  } catch (TException &tx) {
    std::cerr << "Receive Error: " << tx.what() << std::endl;
  }
  std::cout << "Success!" << std::endl;
}

void my_echo_timeout(EchoCobClient* client) {
  std::cout << "Timeout!" << std::endl;
}

void my_echo_default(EchoCobClient* client) {
  std::cout << "Default Error!" << std::endl;
}

//1K 数据echo测试
void Work(const string &str_svr_appkey,
          const string &str_cli_appkey,
          const int32_t &i32_timeout_ms,
          muduo::CountDownLatch *p_countdown) {

  //建议CthriftClient生命期也和线程保持一致，不要一次请求创建销毁一次
  boost::shared_ptr<CthriftClient> cthrift_client = boost::make_shared<CthriftClient>(str_svr_appkey, i32_timeout_ms);
  
  //开启异步化，并设置异步场景下队列阈值，任务超过阈值后会被丢弃，并且向该业务线程抛出异常
  cthrift_client->SetAsync(true);
  cthrift_client->SetThreshold(100);
  cthrift_client->SetFilterService("echo.Echo");
  //cthrift_client.SetBackUpRequest(true,5);
  
  //设置client appkey，方便服务治理识别来源
  if (SUCCESS != cthrift_client->SetClientAppkey(str_cli_appkey)) {
      p_countdown->countDown();
      return;
  }
  if (SUCCESS != cthrift_client->Init()) {
      p_countdown->countDown();
      return;
  }
  /* 注意：72行～84行关键代码段的Cthrift Client的初始化流程，请保证线程安全问题；
 *   避免多线程同时初始化，同一个cthrift client场景
 * */

  boost::shared_ptr<TAsyncChannel> channel= boost::make_shared<CthriftClientChannel>(cthrift_client);
  EchoCobClient client(channel, new CthriftTAsyncProtocolFactory());

  //业务借助AsyncCallback<class type>模版, 传入业务自己的回调处理callback: Success、Timeout ...
  boost::function<void(EchoCobClient*)> echo_cob = boost::bind(&my_echo, _1);
  AsyncCallback<EchoCobClient> *pecho = new AsyncCallback<EchoCobClient>();
  //传入成功处理callback
  pecho->Success(echo_cob);
  pecho->Timeout(boost::bind(&my_echo_timeout, _1));
  pecho->Default(boost::bind(&my_echo_default, _1));
  boost::function<void(EchoCobClient* client)> cob = boost::bind(&AsyncCallback<EchoCobClient>::Callback, pecho, _1);

  string strRet;
  string str_tmp;
  size_t sz;
  char buf[1025];  //1K数据
  memset(buf, 'x', sizeof(buf));

  int i = 10;
  while (true) {
    try{
      str_tmp = boost::lexical_cast<std::string>(i);
    } catch(boost::bad_lexical_cast & e) {

      cerr << "boost::bad_lexical_cast :" << e.what()
           << "i : " << i;
      continue;
    }

    sz = str_tmp.size();
    str_tmp += string(buf, 0, sizeof(buf) - 1 - sz);

  retry:
    try {
      //thrift标准的异步化
      client.echo(cob, str_tmp);
    } catch (TException &tx) {
      cerr << "ERROR: " << tx.what() << endl;
      sleep(1);
      //抛出超出发送队列阈值异常，业务线程应该减速，并且重试本次数据发送操作
      goto retry;
    }

  }

  cout << "tid: " << muduo::CurrentThread::tid() << " END" << endl;

  //受到thrift本身的限制，异步化channel相关对象的生命周期需要和进程一致
  //等待异步回调线程排空数据
  sleep(10);
  p_countdown->countDown();
}

int main(int argc, char **argv) {
  CLOG_INIT();
  string str_svr_appkey("com.sankuai.inf.octo.cthrift"); //服务端的appkey
  string str_cli_appkey("com.sankuai.inf.octo.cthrift.client"); //客户端的appkey
  int32_t i32_timeout_ms = 2000;

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
