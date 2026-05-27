all:async promise
async:async.cc
	g++ -o $@ $^ -std=c++11 -lpthread

promise:promise.cc
	g++ -o $@ $^ -std=c++11 -lpthread

.PHONY:clean
clean:
	rm -rf async