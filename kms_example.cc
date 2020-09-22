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


  CthriftKmsTools tool(str_svr_appkey);
  tool.Update();

  StrStrMap map = tool.GetAppekyTokenMap();
  std::cout << "GetAppekyTokenMap" << endl;
  for(StrStrMap::iterator it = map.begin(); it!= map.end(); it++){
    std::cout << "key:  " << it->first << "; value: " << it->second << std::endl;
  }

  StrStrMap list = tool.GetAppkeyWhitelist();

  std::cout << "GetAppkeyWhitelist" << endl;
  for(StrStrMap::iterator it = list.begin(); it!= list.end(); it++){
    std::cout << "list:  " << it->first << std::endl;
  }


  StrSMMap mmap = tool.GetMethodAppkeyTokenMap();
  std::cout << "GetMethodAppkeyTokenMap" << endl;
  for(StrSMMap::iterator it = mmap.begin(); it!= mmap.end(); it++){
    std::cout << "list:  " << it->first << std::endl;
    for(StrStrMap::iterator it_sub = it->second.begin(); it_sub!= it->second.end(); it_sub++){
      std::cout << "key:  " << it_sub->first << "; value: " << it_sub->second << std::endl;
    }
  }


  std::string local_token = tool.GetLocalTokenString();
  std::cout << "GetLocalTokenString" << endl;
  std::cout << "loal:  " << local_token << std::endl;


  std::cout << "exit" << std::endl;
  CLOG_CLOSE();
  //need wait some time for resources clean.
  sleep(1);
  return 0;
}
