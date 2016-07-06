CC=g++

ip_log_test: test.cpp
	$(CC) -o ip_log_test test.cpp -std=c++14
