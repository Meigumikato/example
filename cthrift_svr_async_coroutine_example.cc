#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>

#include <whale/cthrift_svr.h> 
#include <whale/coroutine/threadpool.h>
#include <whale/coroutine/wait_group.h>

#include <protocol/TBinaryProtocol.h>
#include <async/TAsyncProtocolProcessor.h>
#include "Echo.h"
#include <time.h>
#include <unistd.h>
using namespace std;

using namespace echo;
using namespace muduo;
using namespace muduo::net;
using namespace cthrift;
using namespace cthrift::coroutine;
//using namespace apache::thrift::async;

using apache::thrift::async::TAsyncProcessor;
using apache::thrift::async::TAsyncBufferProcessor;
using apache::thrift::async::TAsyncProtocolProcessor;

int32_t i16_business_thread_num = 1;

muduo::AtomicInt64 atom_i64_worker_thread_pos_;
std::vector<muduo::net::EventLoop *> vec_worker_event_loop_;

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

   // CthriftSvr::SetUserTagMap("testKey", "testValue")  ;
   //只有支持统一协议并且客户端使用统一协议才会有容易协议ID，否则是得到的是local traceid
   // std::cout<< CthriftSvr::GetCurrentTraceId() << std::endl;
   // std::cout<< CthriftSvr::GetCurrentConnId() << std::endl;
   // std::cout<< CthriftSvr::GetCurrentSpanId() << std::endl;
  }
};

uint16_t usleeps = 3;

muduo::net::EventLoop *p_event = NULL;

void runChildTask(WaitGroup* wait_group, boost::shared_ptr<string> str_ptr, string &ret) {
    Task::getCurrentTask()->yield(Task::RUNNABLE);  // 挂起当前任务,等待其他协程任务运行完毕重新运行该任务
    std::cout<<"runChildTask "<<std::endl;
    str_ret.assign(string(str_ptr->c_str()));
    wait_group->Done();
}

class EchoAsyncHandler: public EchoCobSvIf {
    public:
        EchoAsyncHandler(){
            pool = std::make_shared<coroutine::ThreadPool>(0);
            pool->start();
        }
        virtual ~EchoAsyncHandler(){
        }

        void echo(std::tr1::function<void(std::string const& _return)> cob, const std::string& arg){
           // std::cout<<"ECHO "<<std::endl;
            //cob(arg);
           // return;

            boost::shared_ptr<string> str_ptr( new string(arg));
            if(i16_business_thread_num <=1){
                p_event->runInLoop(boost::bind(aysnc_example, cob, str_ptr));

            }else {
                EventLoop *p_worker_event_loop = 0;
                p_worker_event_loop =vec_worker_event_loop_[atom_i64_worker_thread_pos_.getAndAdd(1) % i16_business_thread_num];
                p_worker_event_loop->runInLoop(boost::bind(&EchoAsyncHandler::aysnc_example2, cob, str_ptr));
            }
        }
    private:
        void aysnc_example(std::tr1::function<void(std::string const& _return)> cob, boost::shared_ptr<string> str_ptr) {
            // 当前函数运行与协程环境，可以使用waitgroup并行计算等功能
            std::string str_ret;
            //str_ret.assign(string(str_ptr->c_str()));
            coroutine::WaitGroup wait_group(4);
            for (int32_t index = 0; index < 4; ++index) {
                coroutine::ThreadPool::startTask(runChildTask, &wait_group, str_ptr,str_ret);
            }
            wait_group.Wait();  // 挂起当前任务，但是不hang住当前线程，子任务全部Done之后自动唤醒当前任务
            std::cout<<"aysnc_example OK"<<std::endl;
            cob(str_ret);
            return;
        }

        void aysnc_example2(std::tr1::function<void(std::string const& _return)> cob, boost::shared_ptr<string> str_ptr) {
            if (pool->exceedWaitingTaskLimit()) {
                // 等待运行的task任务超出限制
                return;
            }
            pool->addTask(&EchoAsyncHandler::aysnc_example, this, cob, str_ptr);  // 让aysnc_example函数运行在协程环
            return;
        }

    private:
        std::shared_ptr<coroutine::ThreadPool> pool;  // 可以交给一个公共的地方管理
};


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


  CLOG_INIT();
    string str_svr_appkey("com.sankuai.inf.octo.cthrift"); //服务端的appkey
    uint16_t u16_port = 16888;
    bool b_single_thread = false;  //当时单线程运行时，worker thread num 只能是1
    int32_t i32_timeout_ms = 25;
    int32_t i32_max_conn_num = 100000;
    int16_t i16_worker_thread_num = 10;
    int32_t async_queue_max_size = 30;
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

    // boost::shared_ptr <EchoHandler> handler(new EchoHandler());
    // boost::shared_ptr <TProcessor> processor(new EchoProcessor(handler));


    boost::shared_ptr<EchoCobSvIf> asyncHandler(new EchoAsyncHandler());
    boost::shared_ptr<TAsyncProcessor> asyncProcceser(new EchoAsyncProcessor(asyncHandler));
    boost::shared_ptr<TAsyncBufferProcessor> async_processor(new TAsyncProtocolProcessor(asyncProcceser , boost::make_shared<CthriftTBinaryProtocolFactory>() ));

    /*
       boost::shared_ptr<AggrAsyncHandler> handler(new AggrAsyncHandler());
       boost::shared_ptr<TAsyncProcessor> proc(new AggrAsyncProcessor(handler));
       boost::shared_ptr<TProtocolFactory> pfact(new TBinaryProtocolFactory());
       boost::shared_ptr<TAsyncBufferProcessor> bufproc(new TAsyncProtocolProcessor(proc, pfact));
       */

    try {
        //最简单的server设置
        /*CthriftSvr server(str_svr_appkey,   //在OCTO平台上申请的appkey
          processor,
          u16_port);*/

        //更多参数设置
        CthriftSvr server(str_svr_appkey, async_processor/*processor*/, u16_port,
                b_single_thread, //业务逻辑是否只能单线程运行
                i32_timeout_ms, //毫秒, 服务端超时时间, 只用作日志告警输出
                i32_max_conn_num, //服务最大链接数
                i16_worker_thread_num,"echo.Echo"); //自定义工作线程数，如前面设置业务只能单线程运行，这里只能填1

        //支持统一协议的server设置: 使用统一协议版本后，如果需要回滚，请联系开发者进行服务列表清理工作
        /*CthriftSvr server(str_svr_appkey, processor, u16_port,
          b_single_thread, //业务逻辑是否只能单线程运行
          i32_timeout_ms, //毫秒, 服务端超时时间, 只用作日志告警输出
          i32_max_conn_num //服务最大链接数
          , i16_worker_thread_num //自定义工作线程数，如前面设置业务只能单线程运行，这里只能填1
          , "echo.Echo");
          */
        std::cout << "env " <<  server.GetEnvInfo() << std::endl;
        server.SetConnGCInterval(1);
        server.SetAsyncServerQueueSize(async_queue_max_size);
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
