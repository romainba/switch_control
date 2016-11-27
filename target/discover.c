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

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);

	if (ioctl(fd, SIOCGIFADDR, &ifr)) {
		DEBUG("SIOCGIFADDR failed: %s", strerror(errno));
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
	int ret, sock, s, cnt, optval = 1, t, i;
	unsigned int sin_len;
	struct sockaddr_in sin;
	struct ip_mreq mreq;

	srand(time(NULL));

	buffer = get_local_ipaddr(if_name);
	if (!buffer) {
		DEBUG("%s interface not found", if_name);
		return 1;
	}
	sprintf(buf, "%s:%s:%d:%s", (char *)(devices_name + dev_type), buffer, port, name);
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

	bind(sock, (struct sockaddr *) &sin, sizeof(sin));

	mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &mreq, sizeof(mreq)) < 0) {
		ERROR("setsockopt failed: %s", strerror(errno));
		return 1;
	}

	printf("listening multicase port %d\n", MULTICAST_PORT);

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
		if (strncmp(msg, APP_NAME, strlen(APP_NAME)) == 0)
			continue;

		if (strncmp(msg, DISCOVER_MSG, strlen(DISCOVER_MSG))) {
			DEBUG("not expected msg: %s", msg);
			continue;
		}

		t = 200000 + rand()/(RAND_MAX/256/1000);
		usleep(t); /* 200 to 455 ms sleep */
		printf("sleep %d ms\n", t / 1000);

		if (send_multicast(MULTICAST_ADDR, MULTICAST_PORT,
				   buf, strlen(buf)))
			ERROR("send_multicast failed");
	}

	return 0;
}
