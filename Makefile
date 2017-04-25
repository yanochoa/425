#YanOchoa
#PHASE2 NETWORKS425
.PHONY: all clean

all: cproxy sproxy

cproxy: cproxy.o
	gcc -Wall -g -o $@ $^

sproxy: sproxy.o
	gcc -Wall -g -o $@ $^

clean:
	rm *.o cproxy sproxy
