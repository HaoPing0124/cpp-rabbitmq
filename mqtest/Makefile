mq_exchangetest:mq_exchangetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -lsqlite3

filetest:mq_filetest.cpp
	g++ -o $@ $^ -std=c++11

clean:
	rm -rf filetest mq_exchangetest