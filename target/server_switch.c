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
#include <sys/stat.h>
#include <sys/shm.h>
#include <poll.h>

#include "util.h"
#include <switch.h>
#include "sensor.h"
#include "discover.h"

#define PERIOD_CHECK 30 /* second */
#define DEFAULT_TEMP (25 * 1000) /* degres */

#define GPIO_SW 7

/* led in /sys/class/leds/ */
#define LED2 "tp-link:green:wps"  /* gpio 26 */
#define LED3 "tp-link:green:3g"   /* gpio 27 */
#define LED4 "tp-link:green:wlan" /* gpio 0 */
#define LED5 "tp-link:green:lan"  /* gpio 17 */

struct config {
	char dev[20];
	int req;
	int active;
	int temp;
};

#define SHM_KEY 0x1235

static int file_exists(char *file)
{
	struct stat s;
	int err = stat(file, &s);
	if(err < 0) {
		if(errno == ENOENT) {
			return 0;
		} else {
			ERROR("stat %s", strerror(errno));
			return -1;
		}
	}
	return 1; /* S_ISDIR(s.st_mode) */
}

static void gpio_conf(int gpio)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/gpio/export", gpio);
	if (!file_exists(str))
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

static void led_set(char *led, int value)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/leds/%s/brightness", value, led);
	system(str);
}

static int led_get(char *led)
{
	FILE *pipe;
	char str[50], buf[20];
	int ret;

	sprintf(str, "cat /sys/class/leds/%s/brightness", led);
	pipe = popen(str, "r");
	ret = fread(buf, 1, sizeof(buf), pipe);
	if (ret < 0) {
		ERROR("%s fread failed: %s", strerror(errno));
		return -1;
	} else if (ret == 0) {
		ERROR("%s fread empty");
		return -1;
	}
	pclose(pipe);
	return atoi(buf);
}

static inline void switch_off(struct config *config)
{
	if (!config->active)
		return;

	DEBUG("%s", __func__);

#ifdef REAL_TARGET
	led_set(LED2, 0);
	gpio_set(GPIO_SW, 0);
#endif
	config->active = 0;
}

static inline void switch_on(struct config *config)
{
	if (config->active)
		return;

	DEBUG("%s", __func__);

#ifdef REAL_TARGET
	led_set(LED2, 255);
	gpio_set(GPIO_SW, 255);
#endif
	config->active = 1;
}

static inline int get_temp(struct config *config)
{
	int temp = 0;
#ifdef REAL_TARGET
	if (ds1820_get_temp(config->dev, &temp))
		ERROR("sensor reading failed");
#else
	temp = 23 * 1000;
#endif
	return temp;
}

static int handle_sess(int s, struct config *config)
{
	int new_req = 0;
	struct pollfd fds = { .fd = s, .events = POLLIN };
	struct resp resp;
	struct cmd cmd;
	int ret, sw_on;

	while (1) {
		ret = poll(&fds, 1, 500);
		if (ret < 0) {
			ERROR("poll error %s", strerror(errno));
			break;
		} else if (ret == 0) {
			/* timeout */
			if (new_req != config->req) {
				config->req = new_req;
				DEBUG("req %d\n", new_req);
			}
			if (config->req) {
				if (get_temp(config) > config->temp)
					switch_off(config);
				else
					switch_on(config);
			} else
				switch_off(config);
			continue;
		}

		ret = read(s, &cmd, sizeof(struct cmd));
		if (ret < 0) {
			ERROR("read error %s", strerror(errno));
			break;
		} else if (ret == 0)
			break; /* connection closed */

		DEBUG("received cmd %d len %d data %d", cmd.header.id, cmd.header.len,
		      cmd.header.len ? cmd.u.sw_pos : -1);

		resp.header.id = cmd.header.id;
		resp.header.status = STATUS_OK;
		resp.header.len = 0;

		switch (cmd.header.id) {
		case CMD_SET_SW_POS:
			new_req = cmd.u.sw_pos; /* 0 off, 1 on */
			break;
		case CMD_GET_STATUS: {
			int temp = get_temp(config);

			resp.header.len = sizeof(struct status);
			resp.u.status.temp = temp;
			resp.u.status.tempThres = config->temp;
			resp.u.status.sw_pos = config->req;
			DEBUG("temp %d.%d", temp/1000, temp % 1000);
			break;
		}
		case CMD_TOGGLE_MODE:
			DEBUG("toggle");
			new_req = !config->req;
			break;
		case CMD_SET_TEMP:
			DEBUG("set_temp %d", cmd.u.temp);
			config->temp = cmd.u.temp;
			break;
		default:
			resp.header.status = STATUS_CMD_INVALID;
		}

		ret = write(s, &resp, resp.header.len + sizeof(struct resp_header));
		if (ret < 0) {
			ERROR("write error %s", strerror(errno));
			break;
		}
		DEBUG("send cmd %d len %d\n", resp.header.id, resp.header.len);
	}
	close(s);
	return 0;
}



int main(int argc, char *argv[])
{
	int sock, s, ret, pid, discover_pid, status, shmid;
	struct sockaddr_in sin;
	struct in_addr in_addr;
	struct config *config;
	char *buffer;

	/* shared memory between all child processes */
	shmid = shmget(SHM_KEY, sizeof(struct config), 0644 | IPC_CREAT);
	if (shmid < 0) {
		ERROR("shmget failed: %s", strerror(errno));
		return 0;
	}

	config = shmat(shmid, (void *)0, 0);
	if (config == (void *)(-1)) {
		ERROR("shmat failed");
		return 0;
	}

	memset(config, 0, sizeof(struct config));
	config->temp = DEFAULT_TEMP;

#ifdef REAL_TARGET
	if (ds1820_search(config->dev)) {
		ERROR("Did not find any sensor");
		return 0;
	}
	gpio_conf(GPIO_SW);
#endif
	switch_off(config);

	/* start discover service */
	pid = fork();
	if (pid < 0) {
		ERROR("fork failed: %s",  strerror(errno));
		return 0;
	} else if (pid == 0) {
		char *if_name = (argc > 1) ? argv[1] : "wlan0";

		if (discover_service(if_name))
			ERROR("discover_service failed");
		return 0;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);
	bind(sock, (struct sockaddr *) &sin, sizeof(sin));

	listen(sock, 5);
	printf("listening port %d\n", PORT);

	while (1) {

		s = accept(sock, NULL, NULL);
		if (s < 0) {
			ERROR("accept error %s", strerror(errno));
			break;
		}

		DEBUG("new socket");

		pid = fork();
		if (pid < 0)
			ERROR("fork: %s",  strerror(errno));
		else if (pid == 0) {
			DEBUG("new proc %d", getpid());
			handle_sess(s, config);
			DEBUG("proc %d done", getpid());
			exit(0);
		}

		while (1) {
			pid = waitpid(-1, &status, WNOHANG);
			if (pid < 0) {
				ERROR("waitpid: %s",  strerror(errno));
				break;
			} else if (pid == 0)
				break;
			DEBUG("proc %d ended", pid);
		}

		close(s);
		DEBUG("closed socket");
	}

	close(sock);

	while (1) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid < 0) {
			ERROR("waitpid: %s",  strerror(errno));
			break;
		} else if (pid == 0)
			break;
		DEBUG("proc %d ended", pid);
	}

	if (shmdt(config))
		ERROR("shmdt failed: %s", strerror(errno));

	return 0;
}
