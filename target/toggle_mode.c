/*
 * toggle_mode
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include "util.h"
#include <switch.h>

int main(int argc, char *argv[])
{
	int sock, ret;
	struct sockaddr_in sin;
	struct resp resp;
	struct cmd cmd;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = connect(sock, (struct sockaddr *) &sin, sizeof(sin));
	if (ret < 0) {
		ERROR("connect error %s", strerror(errno));
		return 0;
	}

	cmd.header.id = CMD_TOGGLE_MODE;
	cmd.header.len = 0;

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

	//DEBUG("resp cmd %d status %d", resp.header.id, resp.header.status);

	close(sock);
	return 0;
}
