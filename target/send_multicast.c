/* Send Multicast Datagram code example. */

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

#include "util.h"

char *get_local_ipaddr(char *if_name)
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char *host = malloc(100);

	if (getifaddrs(&ifaddr)) {
		ERROR("getifaddrs failed: %s", strerror(errno));
		return NULL;
	}

	/* Walk through linked list, maintaining head pointer so we
	   can free list later */

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		if (family != AF_INET)
			continue;

		printf("%-8s %s (%d)\n",
		       ifa->ifa_name,
		       (family == AF_PACKET) ? "AF_PACKET" :
		       (family == AF_INET) ? "AF_INET" :
		       (family == AF_INET6) ? "AF_INET6" : "???",
		       family);

		if (strcmp(if_name, ifa->ifa_name))
			continue;

		/* For an AF_INET* interface address, display the address */

		s = getnameinfo(ifa->ifa_addr,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
				sizeof(struct sockaddr_in6),
				host, NI_MAXHOST,
				NULL, 0, NI_NUMERICHOST);
		if (s != 0) {
			printf("getnameinfo() failed: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}

		printf("\t\taddress: <%s>\n", host);
	}

	freeifaddrs(ifaddr);
	return host;
}


int send_multicast(char *addr, int port, char *data)
{
	struct sockaddr_in groupSock;
	struct in_addr localInterface;

	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0) {
		ERROR("socket failed: %s", strerror(errno));
		return 1;
	}

	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr(addr);
	groupSock.sin_port = htons(port);

#if 0
	/* Disable loopback so you do not receive your own datagrams. */
	{
		char loopch = 0;
		if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP,
			      (char *)&loopch, sizeof(loopch)) < 0)
		{
			ERROR("Setting IP_MULTICAST_LOOP error");
			close(sd);
			return 1;
		}
		DEBUG("Disabled the loopback");
	}
#endif

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

	if (sendto(sd, data, strlen(data), 0, (struct sockaddr *)&groupSock,
		   sizeof(groupSock)) < 0) {
		ERROR("Sending datagram message error");
	}

	DEBUG("Sent datagram message: %s", data);

	return 0;

}
