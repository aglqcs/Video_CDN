SOURCE  = src
VPATH   = src
CC 		= gcc
CFLAGS  = -Wall -Werror  -g
OBJS	= proxy.o log.o handle.o bitrate.o mydns.o
OBJS_N	= dijkstra.o log.o mydns.o nameserver.o

default: proxy nameserver

$(SOURCE)/%.o:%.c
	$(CC)  -c $(CFLAGS) $<

proxy: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ 

nameserver: $(OBJS_N)
	$(CC) $(CFLAGS) $(OBJS_N) -o $@ 

clean:
	@rm -f *~ *.o proxy nameserver

run:
	./proxy /tmp/proxy.log 1.0 7777 1.0.0.1 5.0.0.1 5555 3.0.0.1

rundnsrr:
	./nameserver -r /tmp/nlog 5.0.0.1 5555 topos/topo1/topo1.servers topos/topo1/topo1.lsa

rundns:
	./nameserver /tmp/nlog 5.0.0.1 5555 topos/topo1/topo1.servers topos/topo1/topo1.lsa

