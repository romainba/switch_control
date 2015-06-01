/*
 * client_switch.c
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include "util.h"
#include "server_switch.h"


int main(int argc, char *argv[])
{
	int sock, ret;
	struct sockaddr_in sin;
	struct resp resp = { .status = STATUS_OK };
	struct cmd cmd;

	if (argc != 2) {
		printf("usage: %s <ip address>\n", argv[0]);
		return 0;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	if(inet_pton(AF_INET, argv[1], &sin.sin_addr) <= 0) {
		ERROR("inet_pton error %s", strerror(errno));
		return 0;
	}

	ret = connect(sock, (struct sockaddr *) &sin, sizeof(sin));
	if (ret < 0) {
		ERROR("connect error %s", strerror(errno));
		return 0;
	}

	DEBUG("client connected");

	cmd.id = CMD_SET_SW;
	cmd.u.sw_pos = 1;

	ret = write(sock, &cmd, sizeof(struct cmd));
	if (ret < 0) {
		ERROR("write error %s", strerror(errno));
		return 0;
	}

	ret = read(sock, &resp, sizeof(struct resp));
	if (ret < 0) {
		ERROR("read error %s", strerror(errno));
		return 0;
	}

	DEBUG("resp cmd %d status %d", resp.id, resp.status);


	cmd.id = CMD_READ_TEMP;

	ret = write(sock, &cmd, sizeof(struct cmd));
	if (ret < 0) {
		ERROR("write error %s", strerror(errno));
		return 0;
	}

	ret = read(sock, &resp, sizeof(struct resp));
	if (ret < 0) {
		ERROR("read error %s", strerror(errno));
		return 0;
	}

	DEBUG("resp cmd %d status %d", resp.id, resp.status);

	close(sock);
	return 0;
}
