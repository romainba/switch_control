/*
 * server_switch.c
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#include "util.h"
#include "server_switch.h"
#include "sensor.h"

int main(int argc, char *argv[])
{
	int sock, s, ret;
	struct sockaddr_in sin;
	struct resp resp;
	struct cmd cmd;
	char dev[20];

	if (ds1820_search(dev)) {
		ERROR("Did not find any sensor");
		return 0;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);
	bind(sock, (struct sockaddr *) &sin, sizeof(sin));

	listen(sock, 5);
	printf("switch server listening on port %d\n", PORT);

	while (1) {

		s = accept(sock, NULL, NULL);
		if (s < 0) {
			ERROR("accept error %s", strerror(errno));
			break;
		}

		while (1) {
			ret = read(s, &cmd, sizeof(struct cmd));
			if (ret < 0) {
				ERROR("read error %s", strerror(errno));
				break;
			} else if (ret == 0) {
				DEBUG("connection closed");
				break;
			}

			DEBUG("received cmd %d len %d", cmd.id, cmd.len);

			resp.id = cmd.id;
			resp.status = STATUS_OK;
			resp.len = 0;

			switch (cmd.id) {
			case CMD_SET_SW:
				DEBUG("switch %d", cmd.u.sw_pos);
				break;
			case CMD_READ_TEMP: {
				int temp;
				if (ds1820_get_temp(dev, &temp)) {
					resp.status = STATUS_CMD_FAILED;
					ERROR("Sensor reading failed");
					break;
				}

				resp.len = sizeof(int);
				resp.u.temp = temp;
				DEBUG("temp %d.%d", temp/1000, temp % 1000);
				break;
			}
			default:
				resp.status = STATUS_CMD_INVALID;
			}

			ret = write(s, &resp, sizeof(resp));
			if (ret < 0) {
				ERROR("write error %s", strerror(errno));
				break;
			}
		}
		close(s);
	}

	close(sock);
	return 0;
}
