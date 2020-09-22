#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <vector>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <octoidl/SGAgent.h>
#include <iostream>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace boost;

int main(int argc, char** argv) {
    if(argc != 6) {
        cout << "prog <port> <test_num> <pkg_size(K)> <ip> <appkey>" << endl;
        return -1;
    }

    uint16_t u16_port = atoi(argv[1]);
    uint32_t u32_test_num = atoi(argv[2]);
    uint32_t u32_pkg_size = atoi(argv[3]);

    //shared_ptr<TTransport> socket(new TSocket("dx-mobile-mtthrift-multidc01", u16_port));
    shared_ptr<TTransport> socket(new TSocket(argv[4], u16_port));
    shared_ptr<TTransport> transport(new TFramedTransport(socket));
    shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    //EchoClient client(protocol);
    SGAgentClient client(protocol);


    string str_trans_buf;
    for(int k = 0; k < static_cast<int>(u32_pkg_size * 1024); k++) {
        str_trans_buf.push_back(static_cast<char>(k % 128));
    }

    struct timeval tvBegin;
    struct timeval tvEnd;
    int64_t i64_val;
    transport->open();

    vector<double> vec_rtt;
    vector<double> vec_qps;
                vector<SGService> vec_ret;
    //for(int k = 0; k < 10; k++) {
    for(int k = 0; k < 1; k++) {
        vector<int64_t> vecRec;
     //   for(int i = 0; i < static_cast<int>(u32_test_num); i++) {
        for(int i = 0; i < 1; i++) {
            try {
                gettimeofday(&tvBegin,0);

                //string strRet;
                //printf("str_trans_buf size %d\n", str_trans_buf.size());

                string str_local;
                //string str_req("com.sankuai.inf.sg_agent");
                string str_req(argv[5]);
                client.getServiceList(vec_ret, str_local, str_req);                
//                printf("echo %d\n", strRet.size());

                gettimeofday(&tvEnd, 0);
                i64_val = (tvEnd.tv_sec - tvBegin.tv_sec) * 1000000 + (tvEnd.tv_usec - tvBegin.tv_usec); 
                vecRec.push_back(i64_val);
            } catch (TException &tx) {
                printf("ERROR: %s\n", tx.what());
            }
        }


        vector<SGService>::iterator itTest = vec_ret.begin();
    while(itTest != vec_ret.end()){
        cout << itTest->ip << ":" << itTest->port << endl;
    ++itTest;
}

        vector<int64_t>::iterator it = vecRec.begin();
        int64_t i64_total = 0;
        while(it != vecRec.end()) {
            i64_total += *it;
            ++it; 
        }


        double rtt = double(i64_total) / vecRec.size(); 
        vec_rtt.push_back(rtt);
        double qps = ((double(u32_test_num * u32_pkg_size * 1024) / (1024 * 1024)) / i64_total) * 1000000;
        vec_qps.push_back(qps);
    }

    transport->close();

    cout << "port: " << u16_port << " test_num " << u32_test_num << endl;

    vector<double>::iterator it = vec_rtt.begin();
    double total = 0;
    while(it != vec_rtt.end()){
        total += *it;
        ++it;
    }

    cout << "average rtt: " << total / vec_rtt.size() << endl;

    it = vec_qps.begin();
    total = 0;
    while(it != vec_qps.end()){
        total += *it;
        ++it;
    }

    cout << "average qps: " << total / vec_qps.size() << endl;
}
