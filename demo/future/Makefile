all:async promise packaged_task
async:async.cc
	g++ -o $@ $^ -std=c++11 -lpthread

promise:promise.cc
	g++ -o $@ $^ -std=c++11 -lpthread

packaged_task:packaged_task.cc
	g++ -o $@ $^ -std=c++11 -lpthread

.PHONY:clean
clean:
	rm -rf async