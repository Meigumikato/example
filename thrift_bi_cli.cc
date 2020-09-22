//
// Created by Chao Shu on 16/3/1.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sstream>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "Echo.h"
#include <iostream>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace boost;
using namespace echo;

int main(int argc, char** argv) {
  if(argc != 4) {
    cout << "prog <port> <test_num> <pkg_size(K)>" << endl;
    return -1;
  }

  uint16_t u16_port = atoi(argv[1]);
  uint32_t u32_test_num = atoi(argv[2]);
  //uint32_t u32_pkg_size = atoi(argv[3]);

  //shared_ptr<TTransport> socket(new TSocket("dx-mobile-mtthrift-multidc01", u16_port));
  shared_ptr<TTransport> socket(new TSocket("localhost", u16_port));
  shared_ptr<TTransport> transport(new TFramedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  EchoClient client(protocol);

  transport->open();
  string str_trans_buf;
  string strRet;

  for(int i = 0; i < static_cast<int>(u32_test_num); i++) {

    try{
      str_trans_buf = boost::lexical_cast<std::string>(i);
    } catch(boost::bad_lexical_cast & e) {

      cerr << "boost::bad_lexical_cast :" << e.what()
           << "i : " << i;
      continue;
    }

    //client.echo(str_trans_buf);
    //client.echo("string from client");

    client.hello(strRet, "string from client");

    cout << "strRet " << strRet << endl;

    /*if (strRet != str_trans_buf) {
cout << "strRet " << strRet << " str_trans_buf " << str_trans_buf;
}*/
  }

  sleep(10);

  transport->close();

}