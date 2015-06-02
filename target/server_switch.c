/*
 * server_switch.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"
#include <switch.h>
#include "sensor.h"

#define GPIO_SW 26 //7

static void gpio_conf(int gpio)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/gpio/export", gpio);
	system(str);

	sprintf(str, "echo out > /sys/class/gpio/gpio%d/direction", gpio);
	system(str);
}

static void gpio_set(int gpio, int value)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/gpio/gpio%d/value", value, gpio);
	system(str);
}

int handle_sess(int s, char *dev)
{
	struct resp resp;
	struct cmd cmd;
	int ret;

	while (1) {
		ret = read(s, &cmd, sizeof(struct cmd));
		if (ret < 0) {
			ERROR("read error %s", strerror(errno));
			break;
		} else if (ret == 0) {
			DEBUG("connection closed");
			break;
		}

		DEBUG("received cmd %d len %d", cmd.header.id, cmd.header.len);

		resp.header.id = cmd.header.id;
		resp.header.status = STATUS_OK;
		resp.header.len = 0;

		switch (cmd.header.id) {
		case CMD_SET_SW:
			DEBUG("switch %d", cmd.u.sw_pos);
			gpio_set(GPIO_SW, cmd.u.sw_pos);
			break;
		case CMD_READ_TEMP: {
			int temp;
			if (ds1820_get_temp(dev, &temp)) {
				resp.header.status = STATUS_CMD_FAILED;
				ERROR("Sensor reading failed");
				break;
			}

			resp.header.len = sizeof(int);
			resp.u.temp = temp;
			DEBUG("temp %d.%d", temp/1000, temp % 1000);
			break;
		}
		default:
			resp.header.status = STATUS_CMD_INVALID;
		}

		ret = write(s, &resp, sizeof(resp));
		if (ret < 0) {
			ERROR("write error %s", strerror(errno));
			break;
		}
	}
	close(s);
	return 0;
}

int main(int argc, char *argv[])
{
	int sock, s, ret, pid, status;
	struct sockaddr_in sin;
	char dev[20];

	if (ds1820_search(dev)) {
		ERROR("Did not find any sensor");
		return 0;
	}

	gpio_conf(GPIO_SW);

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

		pid = fork();
		if (pid < 0)
			ERROR("fork: %s",  strerror(errno));
		else if (pid == 0) {
			handle_sess(s, dev);
			exit(0);
		}

		while (1) {
			pid = waitpid(0, &status, WNOHANG);
			if (pid < 0) {
				ERROR("waitpid: %s",  strerror(errno));
				break;
			} else if (pid == 0)
				break;
			DEBUG("process %d ended", pid);
		}
		close(s);
	}

	close(sock);
	return 0;
}
