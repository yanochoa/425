#YanOchoa
#PHASE3 NETWORKS425
.PHONY: all clean

all: sproxy cproxy

sproxy: sproxy.o
	gcc -Wall -g -o $@ $^
cproxy: cproxy.o
	gcc -Wall -g -o $@ $^
clean:
	rm *.o sproxy cproxy
