bin=HttpdServer
cc=g++
LDFLAGS=-lpthread

HttpdServer:HttpdServer.cc
	$(cc) -o -std=c++11 $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	rm -f $(bin)
