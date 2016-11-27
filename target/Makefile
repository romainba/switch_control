#CC=mips-openwrt-linux-gcc
CC=gcc

CFLAGS = -g -O2 -Wall -Wstrict-prototypes -Wno-unused-function -Wno-unused-variable \
	-Wimplicit-function-declaration

CFLAGS += -I ../app

CFLAGS += -DREAL_TARGET
CFLAGS += -DCONFIG_RADIATOR2 # Raspberry PI 3 with SHT1x

BIN = server toggle_mode index.cgi

all: $(BIN)

SERVER_OBJS = util.o RPi_SHT1x.o discover.o server_switch.o
TOGGLE_OBJS = util.o toggle_mode.o
INDEX_OBJS = ds1820.o index.o

OBJS = $(SERVER_OBJS) $(TOGGLE_OBJS) $(INDEX_OBJS)

server: $(SERVER_OBJS)
	$(CC) -o $@ $^

toggle_mode: $(TOGGLE_OBJS)
	$(CC) -o $@ $^

index.cgi: $(INDEX_OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

CPPFLAGS += -MD -MP

-include $(OBJS:%.o=%.d)

clean:
	rm -f *.o $(BIN) $(OBJS:%.o=%.d) *~

tar:
	tar cvfz server.tgz *.c *.h Makefile
