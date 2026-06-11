.PHONY:mq_queuetest
.PHONY:all
all:mq_exchangetest filetest mq_queuetest

mq_queuetest:mq_queuetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -lsqlite3
mq_exchangetest:mq_exchangetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -lsqlite3
filetest:mq_filetest.cpp
	g++ -o $@ $^ -std=c++11

clean:
	rm -rf filetest data mq_exchangetest mq_queuetest