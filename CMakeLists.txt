#[[example project brand new build]]
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_C_COMPILER "/docker/opt/rh/devtoolset-3/root/usr/bin/gcc")
SET(CMAKE_CXX_COMPILER "/docker/opt/rh/devtoolset-3/root/usr/bin/g++")
project(whale_example C CXX)

SET(CMAKE_CXX_FLAGS "-std=c++0x  -g -Wall -O2 -DMUDUO_STD_STRING -DHAVE_NETINET_IN_H")

#for output
#default binary and lib path
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

#endif()


#reset EXAMPLE_INCLUDE_PATH
MESSAGE(STATUS "EXAMPLE_INCLUDE_PATH ${EXAMPLE_INCLUDE_PATH}")
MESSAGE(STATUS "SYSTEM_INCLUDE_PATH ${SYSTEM_INCLUDE_PATH}")

FIND_PACKAGE(Boost 1.45.0 REQUIRED)
MESSAGE(STATUS "Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR}")
MESSAGE(STATUS "Boost_LIBRARIES ${Boost_LIBRARY_DIRS}")


SET(SYSTEM_INCLUDE_PATH /usr/include/)
SET(MEITUAN_THRID_INCLUDE_PATH /opt/meituan/)
INCLUDE_DIRECTORIES(SYSTEM ${SYSTEM_INCLUDE_PATH})
INCLUDE_DIRECTORIES(SYSTEM ${SYSTEM_INCLUDE_PATH}/thrift)
INCLUDE_DIRECTORIES(SYSTEM ${MEITUAN_THRID_INCLUDE_PATH}/kms/include/)
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
LINK_DIRECTORIES(${MEITUAN_THRID_INCLUDE_PATH}/kms/lib/)

INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/../include)
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lib/)

LINK_DIRECTORIES(/usr/lib64)

INCLUDE_DIRECTORIES(SYSTEM /usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)

EXECUTE_PROCESS(COMMAND protoc -I=./ --cpp_out=./ echo.proto WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})



IF (NOT EXISTS ./echo)
    EXECUTE_PROCESS(COMMAND thrift --gen cpp:cob_style echo.thrift WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    EXECUTE_PROCESS(COMMAND mv gen-cpp echo WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
ENDIF (NOT EXISTS ./echo)

INCLUDE_DIRECTORIES(SYSTEM echo)


SET(ECHO_THRIFT_SRCS
        echo/echo_constants.cpp
        echo/echo_types.cpp
        echo/Echo.cpp
        )

ADD_EXECUTABLE(cthrift_cli_example_mesh ${ECHO_THRIFT_SRCS} cthrift_cli_example_mesh.cc)
TARGET_LINK_LIBRARIES(cthrift_cli_example_mesh -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a protobuf  libthrift.a libcryptopp.a libkms_comm.a   ssl crypto -Xlinker "-)")

ADD_EXECUTABLE(cthrift_svr_example_mesh ${ECHO_THRIFT_SRCS} cthrift_svr_example_mesh.cc)
TARGET_LINK_LIBRARIES(cthrift_svr_example_mesh -Xlinker "-(" libwhale.a liblion.a librhino.a libcmtraceV2.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a    protobuf  libthrift.a libcryptopp.a libkms_comm.a  ssl crypto -Xlinker "-)")


ADD_EXECUTABLE(cthrift_svr_tags_example ${ECHO_THRIFT_SRCS} cthrift_svr_tags_example.cc)

TARGET_LINK_LIBRARIES(cthrift_svr_tags_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcmtraceV2.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
      protobuf  libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")

ADD_EXECUTABLE(cthrift_cli_tags_example ${ECHO_THRIFT_SRCS} cthrift_cli_tags_example.cc)

TARGET_LINK_LIBRARIES(cthrift_cli_tags_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
      protobuf  libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")


ADD_EXECUTABLE(cthrift_svr_pb_example echo.pb.cc cthrift_svr_pb_example.cc)

TARGET_LINK_LIBRARIES(cthrift_svr_pb_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcmtraceV2.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a  libthrift.a libcryptopp.a libkms_comm.a ssl protobuf crypto -Xlinker "-)")

ADD_EXECUTABLE(cthrift_pb_cli_example echo.pb.cc cthrift_pb_cli_example.cc)

TARGET_LINK_LIBRARIES(cthrift_pb_cli_example  -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a  libthrift.a libcryptopp.a libkms_comm.a protobuf  ssl crypto -Xlinker "-)")


ADD_EXECUTABLE(cthrift_svr_example ${ECHO_THRIFT_SRCS} cthrift_svr_example.cc)

TARGET_LINK_LIBRARIES(cthrift_svr_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcmtraceV2.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
      protobuf  libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")

ADD_EXECUTABLE(cthrift_cli_example ${ECHO_THRIFT_SRCS} cthrift_cli_example.cc)

TARGET_LINK_LIBRARIES(cthrift_cli_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
      protobuf  libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")

ADD_EXECUTABLE(cthrift_cli_ipport_example  ${ECHO_THRIFT_SRCS} cthrift_cli_ipport_example.cc)

TARGET_LINK_LIBRARIES(cthrift_cli_ipport_example  -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
      protobuf  libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")


ADD_EXECUTABLE(cthrift_svr_auth_example ${ECHO_THRIFT_SRCS} cthrift_svr_auth_example.cc)

TARGET_LINK_LIBRARIES(cthrift_svr_auth_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcmtraceV2.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a   protobuf libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")

ADD_EXECUTABLE(cthrift_cli_auth_example ${ECHO_THRIFT_SRCS} cthrift_cli_auth_example.cc)

TARGET_LINK_LIBRARIES(cthrift_cli_auth_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
     protobuf   libthrift.a libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")


ADD_EXECUTABLE(cthrift_async_example ${ECHO_THRIFT_SRCS} cthrift_async_example.cc)

TARGET_LINK_LIBRARIES(cthrift_async_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
     protobuf   libthrift.a libcryptopp.a libkms_comm.a ssl  crypto -Xlinker "-)")

ADD_EXECUTABLE(kms_example  kms_example.cc)

TARGET_LINK_LIBRARIES(kms_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
      protobuf  libthrift.a  libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")

ADD_EXECUTABLE(auth_example  auth_example.cc)

TARGET_LINK_LIBRARIES(auth_example -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
    protobuf    libthrift.a  libcryptopp.a libkms_comm.a ssl crypto -Xlinker "-)")


ADD_EXECUTABLE(cthrift_svr_async_example cthrift_svr_async_example.cc  ${ECHO_THRIFT_SRCS})

TARGET_LINK_LIBRARIES(cthrift_svr_async_example  -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
protobuf    libthrift.a libcryptopp.a libthriftnb.a  libkms_comm.a ssl  crypto -Xlinker "-)")


#ADD_EXECUTABLE(cthrift_svr_async_coroutine_example cthrift_svr_async_coroutine_example.cc ${ECHO_THRIFT_SRCS} )
#
#TARGET_LINK_LIBRARIES(cthrift_svr_async_coroutine_example  -Xlinker "-(" libwhale.a liblion.a librhino.a libcatclient.a libmuduo_net_cpp11.a libmuduo_base_cpp11.a libmuduo_http.a pthread  rt z uuid libclogV2.a libcmtraceV2.a liboctoidlV2.a
#    protobuf    libthrift.a libcryptopp.a libthriftnb.a  libkms_comm.a ssl  crypto -Xlinker "-)")


#[[ADD_EXECUTABLE(thrift_echo_cli ${ECHO_THRIFT_SRCS} thrift_echo_cli.cc)
#TARGET_LINK_LIBRARIES(thrift_echo_cli -Xlinker "-(" event
   #     libthrift.a -Xlinker "-)")

#ADD_EXECUTABLE(thrift_getSvrList_cli thrift_getSvrList_cli.cc)
#TARGET_LINK_LIBRARIES(thrift_getSvrList_cli -Xlinker "-(" libthriftnb.a libthrift.a liboctoidlV2.a event -Xlinker "-)")

#ADD_EXECUTABLE(thrift_echo_svr ${ECHO_THRIFT_SRCS} thrift_echo_svr.cc)
#TARGET_LINK_LIBRARIES(thrift_echo_svr -Xlinker "-(" event libthriftnb.a libthrift.a -Xlinker "-)")

#ADD_EXECUTABLE(thrift_echo_cli_perf ${ECHO_THRIFT_SRCS} thrift_echo_cli_perf.cc)
#TARGET_LINK_LIBRARIES(thrift_echo_cli_perf -Xlinker "-(" event libthriftnb.a libthrift.a -Xlinker "-)")

#INSTALL(CODE "execute_process(COMMAND find ${CTHRIFT_SRC_PATH}/example -name *example*.cc -exec cp -t ${EXAMPLE_CTHRIFT_OUTPUT_PATH} {} \;)")]]
