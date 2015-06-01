CC=mips-openwrt-linux-gcc
#CC=gcc

CFLAGS= -g -O2 -Wall -Wstrict-prototypes -Wno-unused-function -Wno-unused-variable

BIN = server client test

all: $(BIN)

server: util.o sensor.o server_switch.o
	$(CC) -o $@ $^

client: client_switch.o util.o
	$(CC) -o $@ $^

test: sensor.o test.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o $(BIN)

tar:
	tar cvfz server.tgz *.c *.h Makefile
