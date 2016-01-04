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

#include "util.h"
#include <switch.h>
#include "sensor.h"

#define PERIOD_CHECK 30 /* second */
#define DEFAULT_TEMP 18000 /* 18 degres */

#define GPIO_SW 7

/* led in /sys/class/leds/ */
#define LED2 "tp-link:green:wps"  /* gpio 26 */
#define LED3 "tp-link:green:3g"   /* gpio 27 */
#define LED4 "tp-link:green:wlan" /* gpio 0 */
#define LED5 "tp-link:green:lan"  /* gpio 17 */

struct config {
	char dev[20];
	int forced_pid;
	int temp;
};

#define SHM_KEY 0x1234

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

static void switch_off(struct config *config)
{
	DEBUG("%s", __func__);

	if (config->forced_pid) {
		int ret = kill(config->forced_pid, SIGTERM);
		if (ret < 0) {
			ERROR("kill failed: %s", strerror(errno));
		}
	}

	led_set(LED2, 0);
	gpio_set(GPIO_SW, 0);
	config->forced_pid = 0;
}

static void switch_on(struct config *config)
{
	DEBUG("%s", __func__);

	config->forced_pid = getpid();
	led_set(LED2, 255);
	gpio_set(GPIO_SW, 255);

	while (1) {
		int temp = 0;
		if (ds1820_get_temp(config->dev, &temp))
			ERROR("sensor reading failed");

		if (temp > config->temp)
			gpio_set(GPIO_SW, 0);
		else
			gpio_set(GPIO_SW, 255);

		sleep(PERIOD_CHECK);
	}
}

static int handle_sess(int s, struct config *config)
{
	while (1) {
		struct resp resp;
		struct cmd cmd;
		int ret, req = -1, sw_pos = config->forced_pid ? 1 : 0;

		ret = read(s, &cmd, sizeof(struct cmd));
		if (ret < 0) {
			ERROR("read error %s", strerror(errno));
			break;
		} else if (ret == 0)
			break; /* connection closed */

		DEBUG("received cmd %d len %d", cmd.header.id, cmd.header.len);

		resp.header.id = cmd.header.id;
		resp.header.status = STATUS_OK;
		resp.header.len = 0;

		switch (cmd.header.id) {
		case CMD_SET_SW_POS:
			req = cmd.u.sw_pos; /* 0 off, 1 on */
			break;
		case CMD_READ_TEMP: {
			int temp;
			if (ds1820_get_temp(config->dev, &temp)) {
				resp.header.status = STATUS_CMD_FAILED;
				ERROR("Sensor reading failed");
				break;
			}

			resp.header.len = sizeof(int);
			resp.u.temp = temp;
			DEBUG("temp %d.%d", temp/1000, temp % 1000);
			break;
		}
		case CMD_TOGGLE_MODE:
			req = !sw_pos;
			break;
		case CMD_SET_TEMP:
			DEBUG("set_temp %d", cmd.u.temp);
			config->temp = cmd.u.temp;
			break;
		case CMD_GET_SW_POS:
			resp.header.len = sizeof(int);
			resp.u.sw_pos = sw_pos;
			break;
		default:
			resp.header.status = STATUS_CMD_INVALID;
		}

		ret = write(s, &resp, sizeof(resp));
		if (ret < 0) {
			ERROR("write error %s", strerror(errno));
			break;
		}

		switch (req) {
		case 0:
			if (sw_pos == 1)
				switch_off(config);
			break;
		case 1:
			if (sw_pos == 0)
				switch_on(config);
			break;
		}
	}
	close(s);
	return 0;
}

int main(int argc, char *argv[])
{
	int sock, s, ret, pid, status, shmid;
	struct sockaddr_in sin;
	struct config *config;

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

	if (ds1820_search(config->dev)) {
		ERROR("Did not find any sensor");
		return 0;
	}

	gpio_conf(GPIO_SW);

	switch_off(config);

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

		pid = fork();
		if (pid < 0)
			ERROR("fork: %s",  strerror(errno));
		else if (pid == 0) {
			DEBUG("handle sess");
			handle_sess(s, config);
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

	if (shmdt(config))
		ERROR("shmdt failed: %s", strerror(errno));

	return 0;
}
