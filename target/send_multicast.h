
#ifndef SEND_MULTICAST
#define SEND_MULTICAST

#include <sys/types.h>
#include <sys/socket.h>

char *get_local_ipaddr(char *if_name);
int send_multicast(char *addr, int port, char *data);

#endif
