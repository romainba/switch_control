#
# make RADIATOR=1 for server on TPLink with DS1820 and power switch
# make RADIATOR=2 for server on Raspberry PI3 with DS1820 and SHT1x and power switch
# make            for server on host PC  
#
# Options: DAEMONIZE generates server that is launched as a daemon 
#          CONFIG_MIPS uses the mips cross compiler
#

ifdef CONFIG_MIPS
CC=mips-openwrt-linux-gcc
else
CC=gcc
endif

CFLAGS = -O2 -Wall -Wstrict-prototypes -Wno-unused-function -Wno-unused-variable \
	-Wimplicit-function-declaration

CFLAGS += -I ../app

ifeq ($(RADIATOR), 1)
  CFLAGS += -DCONFIG_RADIATOR1 # TPLink with DS1820
  DS1820=y
else
ifeq ($(RADIATOR), 2)
  CFLAGS += -DCONFIG_RADIATOR2 # Raspberry PI 3 with SHT1x
  CFLAGS += -DSYSLOG
  DS1820=y
  SHT1x=y
  DAEMONIZE=y
else
  CFLAGS += -DCONFIG_SIMU
endif
endif

ifdef DAEMONIZE
CFLAGS += -DCONFIG_DAEMONIZE
endif

LDFLAGS = -pthread

BIN = server toggle_mode index.cgi

all: $(BIN)

SERVER-y = util.o discover.o server_switch.o
SERVER-$(SHT1x) += RPi_SHT1x.o
SERVER-$(DS1820) += ds1820.o
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
