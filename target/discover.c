#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>

#include "util.h"
#include "switch.h"

static char *get_local_ipaddr(char *if_name)
{
	char *host, *str;
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		DEBUG("socket open failed");
		return NULL;
	}

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);

	if (ioctl(fd, SIOCGIFADDR, &ifr)) {
	  DEBUG("SIOCGIFADDR %s failed: %s", if_name, strerror(errno));
	  return NULL;
	}
	close(fd);

	host = malloc(100);
	strcpy(host, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	return host;
}


static int send_multicast(char *addr, int port, char *msg, int len)
{
	struct sockaddr_in groupSock;
	char loopch = 0;

	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0) {
		ERROR("socket failed: %s", strerror(errno));
		return 1;
	}
#if 0
	/* Disable loopback so you do not receive your own datagrams. */
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP,
		      (char *)&loopch, sizeof(loopch)) < 0)
	{
		ERROR("Setting IP_MULTICAST_LOOP error");
		close(sd);
		return 1;
	}
#endif
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr(addr);
	groupSock.sin_port = htons(port);

#if 0
	/* Set local interface for outbound multicast datagrams.
	 * The IP address specified must be associated with a local,
	 * multicast capable interface.
	 */
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface,
		       sizeof(localInterface)) < 0) {
		ERROR("Setting local interface failed: %s", strerror(errno));
		return 1;
	}

	DEBUG("Set the local interface");
#endif

	/* Send a message to the multicast group specified by the
	 * groupSock sockaddr structure.
	 */

	if (sendto(sd, msg, len, 0, (struct sockaddr *)&groupSock,
		   sizeof(groupSock)) < 0) {
		ERROR("Sending datagram message error");
	}
	DEBUG("Sent datagram msg: %s", msg);
	close(sd);
	return 0;
}

int discover_service(char *if_name, char *name, int port)
{
	char *buffer, buf[100], msg[50];
	int ret, sock, s, cnt, optval = 1, t, i, dev_type;
	unsigned int sin_len;
	struct sockaddr_in sin;
	struct ip_mreq mreq;
	int timeout = 10;
	
	srand(time(NULL));

	while (timeout--) {
	  buffer = get_local_ipaddr(if_name);
	  if (buffer)
	    break;
	  sleep(1);
	}
	if (!buffer) {
	    DEBUG("%s interface not found", if_name);
	    return 1;
	}

#if (defined CONFIG_RADIATOR1) || (defined CONFIG_SIMU)
	dev_type = RADIATOR1;
#endif
#ifdef CONFIG_RADIATOR2
	dev_type = RADIATOR2;
#endif

	sprintf(buf, "%s:%s:%d:%s", (char *)(dev_type_name + dev_type), buffer, port, name);
	free(buffer);

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(MULTICAST_PORT);
	sin_len = sizeof(sin);

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
		ERROR("setsockopt failed: %s", strerror(errno));
		return 1;
	}

	if (bind(sock, (struct sockaddr *) &sin, sizeof(sin))) {
	  ERROR("bind failed: %s\n", strerror(errno));
	  return 1;
	}

	mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &mreq, sizeof(mreq)) < 0) {
		ERROR("setsockopt failed: %s", strerror(errno));
		return 1;
	}

	DEBUG("listening multicase port %d\n", MULTICAST_PORT);

	while (1) {

		cnt = recvfrom(sock, msg, sizeof(msg), 0,
			(struct sockaddr *) &sin, &sin_len);
		if (cnt < 0) {
			ERROR("recvfrom failed: %s", strerror(errno));
			break;
		} else if (cnt == 0)
			break;

		if (cnt > 50) {
			ERROR("message too big");
			break;
		}
		msg[cnt] = 0;
		if (strncmp(msg, DISCOVER_MSG, strlen(DISCOVER_MSG))) {
			DEBUG("not expected msg: %s", msg);
			continue;
		}

		t = 200000 + rand()/(RAND_MAX/256/1000);
		usleep(t); /* 200 to 455 ms sleep */
		//printf("sleep %d ms\n", t / 1000);

		DEBUG("send multicast msg\n");
		
		if (send_multicast(MULTICAST_ADDR, MULTICAST_PORT + 1,
				   buf, strlen(buf)))
			ERROR("send_multicast failed");
	}

	return 0;
}
