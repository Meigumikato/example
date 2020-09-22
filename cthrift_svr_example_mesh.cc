#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>

#include <whale/cthrift_svr.h>
#include "Echo.h"

using namespace std;

using namespace echo;
using namespace muduo;
using namespace muduo::net;
using namespace cthrift;

muduo::AsyncLogging *g_asyncLog = NULL;

void asyncOutput(const char *msg, int len) {
  g_asyncLog->append(msg, len);
}

class EchoHandler: virtual public EchoIf {
 public:
  EchoHandler() {
  }

  //echo测试
  void echo(std::string &str_ret, const std::string &str_req) {
      str_ret.assign(str_req);
      Timestamp timeStamp=Timestamp::now();
      std::cout<<"ECHO "<< timeStamp.toString()<<std::endl;
   // CthriftSvr::SetUserTagMap("testKey", "testValue")  ;
   //只有支持统一协议并且客户端使用统一协议才会有容易协议ID，否则是得到的是local traceid
   // std::cout<< CthriftSvr::GetCurrentTraceId() << std::endl;
   // std::cout<< CthriftSvr::GetCurrentConnId() << std::endl;
   // std::cout<< CthriftSvr::GetCurrentSpanId() << std::endl;
   std::cout<<"sp_p_str_current_gray_release_cell_:"<< CthriftSvr::GetCurrentGrayReleaseCell() << std::endl;
  }
};


int
main(int argc, char **argv) {
  //注意：请使用业务自身的appkey进行cat初始化！！！！！
  catClientInit("com.sankuai.inf.octo.cthrift");


  CLOG_INIT();
  string str_svr_appkey("com.sankuai.inf.octo.cthrift"); //服务端的appkey
  uint16_t u16_port = 16888;
  bool b_single_thread = false;  //当时单线程运行时，worker thread num 只能是1
  int32_t i32_timeout_ms = 15;
  int32_t i32_max_conn_num = 100000;
  int16_t i16_worker_thread_num = 10;

  switch (argc) {
    case 1:
      std::cout << "no input arg, use defalut" << std::endl;
      break;

    case 7:
      str_svr_appkey.assign(argv[1]);
      u16_port = static_cast<uint16_t>(atoi(argv[2]));
      b_single_thread = (1 == atoi(argv[3])) ? true : false;
      i32_timeout_ms = atoi(argv[4]);
      i32_max_conn_num = atoi(argv[5]);
      i16_worker_thread_num = static_cast<uint16_t>(atoi(argv[6]));

      break;
    default:
      cerr
          << "prog <svr appkey> <port> <single thread ? 1:true, 0:false><timeout ms> <max connection num> <worker thread num for every IO thread: suggest NOT more than CPU core num> but argc "
          << argc << endl;
      exit(-1);
  }

  std::cout << "svr appkey " << str_svr_appkey << std::endl;
  std::cout << "port " << u16_port << std::endl;
  std::cout << "single thread? " << b_single_thread << std::endl;
  std::cout << "timeout ms " << i32_timeout_ms << std::endl;
  std::cout << "timeout ms " << i32_max_conn_num << std::endl;
  std::cout << "timeout ms " << i16_worker_thread_num << std::endl;

  muduo::AsyncLogging log("cthrift_svr_example", 500 * 1024 * 1024);
  log.start();
  g_asyncLog = &log;

  muduo::Logger::setLogLevel(muduo::Logger::WARN);//线上运行推荐用WARN级别日志
  muduo::Logger::setTimeZone(muduo::TimeZone(8 * 3600, "CST"));
  muduo::Logger::setOutput(asyncOutput);

  muduo::string name("EchoServer");

  //init cat
  //catClientInit("CthriftSvr");

  boost::shared_ptr <EchoHandler> handler(new EchoHandler());
  boost::shared_ptr <TProcessor> processor(new EchoProcessor(handler));

  try {
    //最简单的server设置
    /*CthriftSvr server(str_svr_appkey,   //在OCTO平台上申请的appkey
                      processor,
                      u16_port);*/

    //更多参数设置
    ServerOption srv_option;
    srv_option.b_Mesh  = true;
    CthriftSvr server(str_svr_appkey, processor, u16_port,
                      b_single_thread, //业务逻辑是否只能单线程运行
                      i32_timeout_ms, //毫秒, 服务端超时时间, 只用作日志告警输出
                      i32_max_conn_num //服务最大链接数
        , i16_worker_thread_num,"echo.Echo",srv_option); //自定义工作线程数，如前面设置业务只能单线程运行，这里只能填1

    //支持统一协议的server设置: 使用统一协议版本后，如果需要回滚，请联系开发者进行服务列表清理工作
    /*CthriftSvr server(str_svr_appkey, processor, u16_port,
                      b_single_thread, //业务逻辑是否只能单线程运行
                      i32_timeout_ms, //毫秒, 服务端超时时间, 只用作日志告警输出
                      i32_max_conn_num //服务最大链接数
        , i16_worker_thread_num //自定义工作线程数，如前面设置业务只能单线程运行，这里只能填1
	, "echo.Echo");
    */
    std::cout << "env " <<  server.GetEnvInfo() << std::endl;
 
    server.serve();

  } catch (TException &tx) {
    LOG_ERROR << tx.what();

    /*catClientDestroy();*/
    return -1;
  }

  /*catClientDestroy();*/
  CLOG_CLOSE();
  return 0;
}
