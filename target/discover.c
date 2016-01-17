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

	/* Set local interface for outbound multicast datagrams.
	 * The IP address specified must be associated with a local,
	 * multicast capable interface.
	 */
#if 0
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
	return 0;
}

int discover_service(char *if_name)
{
	char *buffer, buf[100], msg[50];
	int sock, s, cnt;
	unsigned int sin_len;
	struct sockaddr_in sin;
	struct ip_mreq mreq;

	buffer = get_local_ipaddr(if_name);
	if (!buffer) {
		DEBUG("%s interface not found", if_name);
		return 1;
	}
	sprintf(buf, "%s:%s:%d", APP_NAME, buffer, PORT);
	free(buffer);

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(MULTICAST_PORT);
	sin_len = sizeof(sin);

	bind(sock, (struct sockaddr *) &sin, sizeof(sin));

	mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &mreq, sizeof(mreq)) < 0) {
		ERROR("setsockopt failed: %s", strerror(errno));
		return 1;
	}

	printf("listening port %d\n", MULTICAST_PORT);

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
			DEBUG("not expected msg");
			continue;
		}

		if (send_multicast(MULTICAST_ADDR, MULTICAST_PORT + 1,
				   buf, strlen(buf)))
			ERROR("send_multicast failed");
	}

	return 0;
}
