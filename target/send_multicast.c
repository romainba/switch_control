/* Send Multicast Datagram code example. */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util.h"


int send_multicast(char *addr, int port, char *local_addr, char *data)
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
	localInterface.s_addr = inet_addr(local_addr);

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
