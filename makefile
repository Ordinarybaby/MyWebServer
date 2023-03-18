server: server.cpp ./log/log.h ./log/log.cpp ./log/block_queue.h ./lock/locker.h ./http/fd.h
	g++ -o server $^ -lpthread

clean:
	rm -r server