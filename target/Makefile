ifdef CONFIG_MIPS
CC=mips-openwrt-linux-gcc
else
CC=gcc
endif

CFLAGS = -O2 -Wall -Wstrict-prototypes -Wno-unused-function -Wno-unused-variable \
	-Wimplicit-function-declaration

CFLAGS += -I ../app

ifdef RADIATOR1
CFLAGS += -DCONFIG_RADIATOR1 # TPLink with DS1820
endif
ifdef RADIATOR2
CFLAGS += -DCONFIG_RADIATOR2 # Raspberry PI 3 with SHT1x
CFLAGS += -DSYSLOG
endif
ifdef SIMU
CFLAGS += -DCONFIG_SIMU
endif

ifdef DAEMONIZE
CFLAGS += -DDAEMONIZE
endif

LDFLAGS = -pthread

BIN = server toggle_mode index.cgi

all: $(BIN)

SERVER-y = util.o RPi_SHT1x.o discover.o server_switch.o ds1820.o
SERVER-$(DAEMONIZE) += daemonize.o

TOGGLE-y = util.o toggle_mode.o
INDEX-y = ds1820.o index.o

OBJS = $(SERVER-y) $(TOGGLE-y) $(INDEX-y)

server: $(SERVER-y)
	$(CC) $(LDFLAGS) -o $@ $^

toggle_mode: $(TOGGLE-y)
	$(CC) -o $@ $^

index.cgi: $(INDEX-y)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

CPPFLAGS += -MD -MP

-include $(OBJS:%.o=%.d)

clean:
	rm -f *.o $(BIN) $(OBJS:%.o=%.d) *~

tar:
	tar cvfz server.tgz *.c *.h Makefile
