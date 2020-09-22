#include <iostream>
#include "Echo.h"
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <concurrency/ThreadManager.h>
#include <concurrency/PosixThreadFactory.h>
#include <server/TNonblockingServer.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace::apache::thrift::concurrency;

using boost::shared_ptr;

using namespace  ::echo;

class EchoHandler : virtual public EchoIf {
 public:
  EchoHandler() {
    // Your initialization goes here
  }

  void echo(std::string& _return, const std::string& arg) {
    // Your implementation goes here
    //printf("echo\n");
    _return = arg; 
  }

};

int main(int argc, char **argv) {
  if(3 != argc) {
std::cout << "prog <port> <threadNum>" << std::endl;
exit(-1);
}

  int port = atoi(argv[1]);
  shared_ptr<EchoHandler> handler(new EchoHandler());
  shared_ptr<TProcessor> processor(new EchoProcessor(handler));
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(atoi(argv[2]));
  shared_ptr<PosixThreadFactory> threadFactory = shared_ptr<PosixThreadFactory> (new PosixThreadFactory()); //PosixThreadFactory可以自定义（继承于ThreadFactory）
  threadManager->threadFactory(threadFactory);
  threadManager->start();     
  TNonblockingServer server(processor, protocolFactory, port, threadManager);

  server.serve();
  return 0;
}

