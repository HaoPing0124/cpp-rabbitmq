all:dict_client dict_server
dict_client:dict_client.cpp
	g++ -o $@ $^ -std=c++11 -I ./include -L./lib -lmuduo_net -lmuduo_base -pthread
dict_server:dict_server.cpp
	g++ -o $@ $^ -std=c++11 -I./include -L./lib -lmuduo_net -lmuduo_base -pthread

.PHONY:clean
clean:
	rm -rf dict_server