SOURCE  = src
VPATH   = src
CC 		= gcc
CFLAGS  = -Wall  -g -std=gnu99
OBJS	= proxy.o log.o handle.o


default: proxy

$(SOURCE)/%.o:%.c
	$(CC)  -c $(CFLAGS) $<

proxy: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ 

clean:
	@rm -f *~ *.o proxy 

run:
	./proxy /tmp/proxy.log 1.0 7777 1.0.0.1 5.0.0.1 5555 3.0.0.1

