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
#include <sys/shm.h>
#include <poll.h>
#include <signal.h>

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
	int requested;
	int temp;
	int active;
	int switch_pid;
};

enum { REQ_NONE, REQ_ON, REQ_OFF };

#define SHM_KEY 0x1235


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

static void update_switch(struct config *config, int request)
{
	if (request) {
		int v = (request == REQ_ON);
		if (v != config->requested)
			config->requested = v;
	}

	if (config->requested) {
		if (get_temp(config) > config->temp)
			switch_off(config);
		else
			switch_on(config);
	} else
		switch_off(config);
}

static int proc_switch(struct config *config)
{
	int ret;
	struct timespec timeout = { .tv_nsec = 1, .tv_nsec = 0 };
	sigset_t waitset;
	siginfo_t info;

	sigemptyset(&waitset);
	sigaddset(&waitset, SIGUSR1);

	ret = fork();
	if (ret)
		return ret;

	config->active = 1;
	switch_off(config);

	while (1) {
		if (sigtimedwait(&waitset, &info, &timeout) < 0) {
			if (errno == EAGAIN) {
				DEBUG("%s timeout", __func__);
				update_switch(config, 0);
				continue;
			} else
				break;
		}
		update_switch(config, info.si_value.sival_int);
	}
	DEBUG("%s done");
	return 0;
}

static int handle_sess(int s, struct config *config)
{
	struct pollfd fds = { .fd = s, .events = POLLIN | POLLERR };
	struct resp resp;
	struct cmd cmd;
	int ret, request;
	union sigval sv;

	while (1) {
		ret = poll(&fds, 1, -1);
		if (ret < 0) {
			ERROR("poll error %s", strerror(errno));
			break;
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

		request = REQ_NONE;

		switch (cmd.header.id) {
		case CMD_SET_SW_POS:
			request = cmd.u.sw_pos /* 0 off, 1 on */ ? REQ_ON : REQ_OFF;
			break;
		case CMD_GET_STATUS: {
			int temp = get_temp(config);

			resp.header.len = sizeof(struct status);
			resp.u.status.temp = temp;
			resp.u.status.tempThres = config->temp;
			resp.u.status.sw_pos = config->requested;
			DEBUG("temp %d.%d", temp/1000, temp % 1000);
			break;
		}
		case CMD_TOGGLE_MODE:
			DEBUG("toggle");
			request = config->requested ? REQ_OFF : REQ_ON;
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

		sv.sival_int = request;
		if (sigqueue(config->switch_pid, SIGUSR1, sv))
			ERROR("sigqueue error %s", strerror(errno));

		DEBUG("send resp %d len %d, request %d\n", resp.header.id, resp.header.len,
			request);
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

	/* start discover service */
	char *if_name = (argc > 1) ? argv[1] : "wlan0";

	if (discover_service(if_name)) {
		ERROR("discover_service failed");
		exit(1);
	}

	/* shared memory between all child processes */
	shmid = shmget(SHM_KEY, sizeof(struct config), 0644 | IPC_CREAT);
	if (shmid < 0) {
		ERROR("shmget failed: %s", strerror(errno));
		exit(1);
	}

	config = shmat(shmid, NULL, 0);
	if (config == (void *)(-1)) {
		ERROR("shmat failed");
		exit(1);
	}

	memset(config, 0, sizeof(struct config));
	config->temp = DEFAULT_TEMP;

#ifdef REAL_TARGET
	if (ds1820_search(config->dev)) {
		ERROR("Did not find any sensor");
		exit(1);
	}
	gpio_conf(GPIO_SW);
#endif

	if (proc_switch(config)) {
		ERROR("proc_switch failed");
		exit(1);
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
