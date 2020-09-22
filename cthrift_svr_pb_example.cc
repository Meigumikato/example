#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>

#include <whale/cthrift_svr.h>

#include <protocol/TBinaryProtocol.h>
#include <async/TAsyncProtocolProcessor.h>
#include "Echo.h"
#include "echo.pb.h"
#include <whale/cpb_controller.h>
#include <whale/cpb_channel.h>
#include <whale/cpb_processor.h>
#include <whale/cpb_closure.h>

#include <time.h>
#include <unistd.h>
using namespace std;

using namespace echo;
using namespace muduo;
using namespace muduo::net;
using namespace cthrift;
//using namespace apache::thrift::async;
using namespace example;

using apache::thrift::async::TAsyncProcessor;
using apache::thrift::async::TAsyncBufferProcessor;
using apache::thrift::async::TAsyncProtocolProcessor;

int32_t i16_business_thread_num = 1;

muduo::AtomicInt64 atom_i64_worker_thread_pos_;
std::vector<muduo::net::EventLoop *> vec_worker_event_loop_;

muduo::AsyncLogging *g_asyncLog = NULL;

void asyncOutput(const char *msg, int len) {
 // g_asyncLog->append(msg, len);
}

class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {};
    virtual ~EchoServiceImpl() {};
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done) {
        
        boost::shared_ptr<google::protobuf::Closure> ptr(done);//业务代码必须调用      

       //PbController* cntl = static_cast<PbController*>(cntl_base);
      // std::cout<<"EchoServiceImpl::Echo request:"<<request->message()<<std::endl;
       
       response->set_message(request->message());
       //std::cout<<"EchoServiceImpl::Echo response:"<<response->message()<<std::endl;

       Timestamp timeStamp=Timestamp::now();
       std::cout<<"ECHO "<< timeStamp.toString()<<std::endl;

       //PbClosure *done_ = static_cast<PbClosure*>(done);  
       done->Run();
    }

    virtual void Echo2(google::protobuf::RpcController* cntl_base,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done) {

       boost::shared_ptr<google::protobuf::Closure> ptr(done);//业务代码必须调用,否则内存泄漏

      // PbController* cntl = static_cast<PbController*>(cntl_base);
       response->set_message(request->message());
       std::cout<<"EchoServiceImpl::Echo2 response:"<<response->message()<<std::endl;
       //PbClosure *done_ = static_cast<PbClosure*>(done);
       done->Run();
    }
};


uint16_t usleeps = 3;

muduo::net::EventLoop *p_event = NULL;


void WorkerThreadInit(muduo::CountDownLatch *p_countdown_workthread_init) {
    p_countdown_workthread_init->countDown();
}

void InitWorkerThreadPos(void) {  //init start pos for avoid big-number-mod performance issue
    if (CTHRIFT_LIKELY(1 < i16_business_thread_num)) {
        CLOG_STR_DEBUG(atom_i64_worker_thread_pos_.getAndSet(0)
                << " msg per 5 mins");
    }
}

int
main(int argc, char **argv) {
  //注意：请使用业务自身的appkey进行cat初始化！！！！！
  
   
    catClientInit("com.sankuai.inf.octo.cthrift");
   //catClientInit("com.sankuai.inf.octo.cthrift");


    CLOG_INIT();
    string str_svr_appkey("com.sankuai.inf.octo.cthrift"); //服务端的appkey
    uint16_t u16_port = 16888;
    bool b_single_thread = false;  //当时单线程运行时，worker thread num 只能是1
    int32_t i32_timeout_ms =60;
    int32_t i32_max_conn_num = 100000;
    int16_t i16_worker_thread_num = 15;
    int32_t async_queue_max_size = 3000;
    int16_t io_thread_num = 4;
    switch (argc) {
        case 1:
            std::cout << "no input arg, use defalut" << std::endl;
            break;

        case 10:
            str_svr_appkey.assign(argv[1]);
            u16_port = static_cast<uint16_t>(atoi(argv[2]));
            b_single_thread = (1 == atoi(argv[3])) ? true : false;
            i32_timeout_ms = atoi(argv[4]);
            i32_max_conn_num = atoi(argv[5]);
            i16_worker_thread_num = static_cast<uint16_t>(atoi(argv[6]));
            i16_business_thread_num = static_cast<uint16_t>(atoi(argv[7]));
            if(i16_business_thread_num <=1){
                i16_business_thread_num = 1;
            }
            async_queue_max_size = static_cast<uint16_t>(atoi(argv[8]));
            usleeps = static_cast<uint16_t>(atoi(argv[9]));
            //io_thread_num = static_cast<uint16_t>(atoi(argv[10])); 
            break;
        default:
            cerr
                << "prog <svr appkey> <port> <single thread ? 1:true, 0:false><timeout ms> <max connection num> <worker thread num for every IO thread: suggest NOT more than CPU core num> but argc "
                << argc << endl;
            exit(-1);
    }
    std::cout<<"i16_business_thread_num must >1"<<endl; 

    if(i16_business_thread_num == 1){
        muduo::net::EventLoopThread *pt = new muduo::net::EventLoopThread;
        p_event = pt->startLoop();
    } else{
        do{
            muduo::net::EventLoop event_loop_, * p_event_loop_ = 0 ;  //guarantee init before server_ !!
            muduo::net::EventLoopThread *p_eventloop_thread = 0;
            muduo::CountDownLatch countdown_workerthread_init(i16_business_thread_num);

            string str_pool_name("cthrift_asncsvr_business_event_thread_pool");
            for (int i = 0; i < i16_business_thread_num; i++) {
                char buf[str_pool_name.size() + 32];
                snprintf(buf, sizeof buf, "%s%d", str_pool_name.c_str(), i);
                p_eventloop_thread = new EventLoopThread(boost::bind(
                            &WorkerThreadInit,
                            &countdown_workerthread_init),
                        buf); //memory leak， but should use these threads during whole process lifetime, so ignore

                vec_worker_event_loop_.push_back(p_eventloop_thread->startLoop());
            }

            p_eventloop_thread = new EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "cthrift_svr_schedule");
            p_event_loop_ = p_eventloop_thread->startLoop();
            countdown_workerthread_init.wait();
            std::cout<<"INITED"<<std::endl;
            CLOG_STR_INFO("worker thread init done");
            p_event_loop_->runEvery(60 * 5, boost::bind(&InitWorkerThreadPos));  //every 5 min, clear keep-increment worker thread pos, for performance

            std::cout<<"server..................."<<std::endl; 
        }while(0);
    }
    std::cout << "svr appkey " << str_svr_appkey << std::endl;
    std::cout << "port " << u16_port << std::endl;
    std::cout << "single thread? " << b_single_thread << std::endl;
    std::cout << "timeout ms " << i32_timeout_ms << std::endl;
    std::cout << "i32_max_conn_num " << i32_max_conn_num << std::endl;
    std::cout << "i16_worker_thread_num " << i16_worker_thread_num << std::endl;
    std::cout << "i16_business_thread_num "<< i16_business_thread_num <<std::endl;
    std::cout << "async_queue_max_size " << async_queue_max_size <<std::endl;

    muduo::AsyncLogging log("cthrift_svr_example", 500 * 1024 * 1024);
    log.start();
    g_asyncLog = &log;

    muduo::Logger::setLogLevel(muduo::Logger::WARN);//线上运行推荐用WARN级别日志
    muduo::Logger::setTimeZone(muduo::TimeZone(8 * 3600, "CST"));
    muduo::Logger::setOutput(asyncOutput);

    muduo::string name("EchoServer");

    //init cat
    //catClientInit("CthriftSvr");

    EchoServiceImpl  serice_impl;
    //PbProcessor  pb_processor(&serice_impl);
    boost::shared_ptr<PbProcessor> pb_processor(new PbProcessor(&serice_impl));
    std::string str_serice_name("example.EchoService");
    try {


        //更多参数设置
        ServerOption  op;//建议直接使用op参数版本，方便以后扩展
        //op.srv_tags.push_back("tag1");//自定义路由 该appkey下 注册tag1 tag2 标签
        //op.srv_tags.push_back("tag2");
        CthriftSvr server(str_svr_appkey, pb_processor/*processor*/, u16_port,
                b_single_thread, //业务逻辑是否只能单线程运行
                i32_timeout_ms, //毫秒, 服务端超时时间, 只用作日志告警输出
                i32_max_conn_num, //服务最大链接数
                i16_worker_thread_num,//自定义工作线程数，如前面设置业务只能单线程运行，这里只能填1
                str_serice_name,//service_name格式：echo.proto 文件内： "package.service("本例是:example.EchoService");
                op); 

        std::cout << "env " <<  server.GetEnvInfo() << std::endl;
        server.SetConnGCInterval(20);
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
