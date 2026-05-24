all:client server
client:protobuf_client.cpp request.pb.cc ../include/muduo/proto/codec.cc
	g++ -o $@ $^ -std=c++11 -I../include -L../lib -lmuduo_net -lmuduo_base -pthread -lprotobuf -lz
server:protobuf_server.cpp request.pb.cc ../include/muduo/proto/codec.cc
	g++ -o $@ $^ -std=c++11 -I../include -L../lib -lmuduo_net -lmuduo_base -pthread -lprotobuf -lz

.PHONY:clean
clean:
	rm -rf client server