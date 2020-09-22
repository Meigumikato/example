//
// Created by Chao Shu on 16/3/1.
//

#include <iostream>
#include "Echo.h"
#include <cthrift/cthrift_common.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <concurrency/ThreadManager.h>
#include <concurrency/PosixThreadFactory.h>
#include <server/TNonblockingServer.h>
#include <boost/make_shared.hpp>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;


using boost::shared_ptr;
using boost::make_shared;

using namespace ::echo;
using namespace std;

class EchoHandler: virtual public EchoIf {
 public:
  EchoHandler(const boost::shared_ptr <TTransport>& trans) :
      echo_cli_(make_shared<TBinaryProtocol>(trans)), p_event_loop_(0) {
    std::cout << "EchoHandler Object" << std::endl;
    t_time_ = time(0);
  }

  ~EchoHandler() {
    cout << "~EchoHandler t_time_ " << t_time_ << endl;
  }

  //void echo(const std::string& _return) {
  void hello(std::string& arg, const std::string& _return) {
    // Your implementation goes here
    //printf("echo\n");
    //_return = arg;
    std::cout << "recv from client " << _return << std::endl;

    if (!p_event_loop_) {
      std::cout << "create loop" << std::endl;
      sp_sgagent_event_thread_ = boost::make_shared<muduo::net::EventLoopThread>();
      p_event_loop_ = sp_sgagent_event_thread_->startLoop();
      p_event_loop_->runEvery(1.0,
                              boost::bind(&EchoHandler::CallbackThread, this));
    } else {
      std::cout << "NO need to create loop" << std::endl;
    }

  }

  void CallbackThread(void) {
    std::cout << "CallbackThread" << std::endl;

    try {
      //echo_cli_.echo("string from svr");
      string str_recv;
      echo_cli_.hello(str_recv, "string from svr");
    } catch (TException) {
      std::cout << "catch" << std::endl;
      return;
    }
  }

  EchoClient echo_cli_;
  muduo::net::EventLoop* p_event_loop_;
  boost::shared_ptr <muduo::net::EventLoopThread> sp_event_thread_;
  time_t t_time_;
};

class ProcessorFactoryImply: public TProcessorFactory {
  virtual boost::shared_ptr <TProcessor> getProcessor(
      const TConnectionInfo& connInfo) {
    return make_shared<EchoProcessor>(make_shared<EchoHandler>(connInfo.transport));
  }
};


int main(int argc, char** argv) {
  if (3 != argc) {
    std::cout << "prog <port> <threadNum>" << std::endl;
    exit(-1);
  }

  int port = atoi(argv[1]);

  shared_ptr <TProcessorFactory> processorFactory(new ProcessorFactoryImply());
  shared_ptr <TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr <TTransportFactory> transportFactory(new TFramedTransportFactory
                                                      ());
  shared_ptr <TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  shared_ptr <ThreadManager>
      threadMgr = ThreadManager::newSimpleThreadManager(atoi(argv[2]));
  shared_ptr <PosixThreadFactory> threadFactory =
      shared_ptr<PosixThreadFactory>(new PosixThreadFactory()); //PosixThreadFactory¿ÉÒÔ×Ô¶¨Òå£¨¼Ì³ÐÓÚThreadFactory£©


  threadMgr->threadFactory(threadFactory);
  threadMgr->start();

  TNonblockingServer server(processorFactory, transportFactory,
                            transportFactory,
                            protocolFactory, protocolFactory, port, threadMgr);

  server.serve();
  return 0;
}
