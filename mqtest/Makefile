.PHONY:mq_hosttest
mq_hosttest:mq_hosttest.cpp ../mqcommon/mq_msg.pb.cc
	g++ -o $@ $^ -std=c++17 -lgtest -lprotobuf -lsqlite3
mq_msgtest:mq_msgtest.cpp ../mqcommon/mq_msg.pb.cc
	g++ -o $@ $^ -std=c++17 -lgtest -lprotobuf -lsqlite3
mq_queuetest:mq_queuetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -lsqlite3
mq_exchangetest:mq_exchangetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -lsqlite3
filetest:mq_filetest.cpp
	g++ -o $@ $^ -std=c++11

clean:
	rm -rf filetest data mq_exchangetest mq_queuetest