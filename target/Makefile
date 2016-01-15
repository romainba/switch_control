#CC=mips-openwrt-linux-gcc
CC=gcc

CFLAGS = -g -O2 -Wall -Wstrict-prototypes -Wno-unused-function -Wno-unused-variable \
	-Wimplicit-function-declaration
CFLAGS += -I ../app
ifneq ($(CC),gcc)
	CFLAGS += -DREAL_TARGET
endif

BIN = server toggle_mode

all: $(BIN)

SERVER_OBJS = util.o sensor.o discover.o server_switch.o
TOGGLE_OBJS = util.o toggle_mode.o

OBJS = $(SERVER_OBJS) $(TOGGLE_OBJS)

server: $(SERVER_OBJS)
	$(CC) -o $@ $^

toggle_mode: $(TOGGLE_OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

CPPFLAGS += -MD -MP

-include $(OBJS:%.o=%.d)

clean:
	rm -f *.o $(BIN) $(OBJS:%.o=%.d)

tar:
	tar cvfz server.tgz *.c *.h Makefile
