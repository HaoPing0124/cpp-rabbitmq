.PHONY: mqserver
CFLAG= -I../mqthird/include
LFLAG= -L../mqthird/lib -lgtest -lprotobuf -lsqlite3 -pthread -lmuduo_net -lmuduo_base -lz
mqserver:mq_server.cc ../mqcommon/mq_msg.pb.cc ../mqcommon/mq_proto.pb.cc ../mqthird/include/muduo/proto/codec.cc
	g++ $(CFLAG) -o $@ $^ $(LFLAG) -std=c++17 -g