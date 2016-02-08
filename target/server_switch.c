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
	int temp_thres;
	int active;
	int switch_pid;
};

enum { REQ_NONE, REQ_ON, REQ_OFF };

#define SHM_KEY 0x1234


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

	config->temp = get_temp(config);

	if (config->requested) {
		if (config->temp > config->temp_thres)
			switch_off(config);
		else
			switch_on(config);
	} else
		switch_off(config);
}

static int proc_switch(struct config *config)
{
	int ret;
	struct timespec timeout = { .tv_sec = 10, .tv_nsec = 0 };
	sigset_t waitset;
	siginfo_t info;

	sigemptyset(&waitset);
	sigaddset(&waitset, SIGUSR1);

#ifdef REAL_TARGET
	gpio_conf(GPIO_SW);
#endif
	config->active = 1;
	switch_off(config);

	if (sigprocmask(SIG_SETMASK, &waitset, NULL) < 0)
		exit(1);

	while (1) {
		ret = sigtimedwait(&waitset, &info, &timeout);
		if (ret < 0) {
			if (errno == EAGAIN) {
				update_switch(config, 0);
				continue;
			} else
				break;
		}
		update_switch(config, info.si_value.sival_int);
	}
	DEBUG("%s done", __func__);
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
			resp.header.len = sizeof(struct status);
			resp.u.status.temp = config->temp;
			resp.u.status.tempThres = config->temp_thres;
			resp.u.status.sw_pos = config->requested;
			break;
		}
		case CMD_TOGGLE_MODE:
			DEBUG("toggle");
			request = config->requested ? REQ_OFF : REQ_ON;
			break;
		case CMD_SET_TEMP:
			DEBUG("set temp_thres %d", cmd.u.temp);
			config->temp_thres = cmd.u.temp;
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
	int sock, s, ret, pid, discover_pid = 0, status, shmid;
	struct sockaddr_in sin;
	struct in_addr in_addr;
	struct config *config = NULL;
	char *buffer;

	if (argc < 2 || argc > 3) {
		printf("usage: %s <name> [<ethernet if>]\n", argv[0]);
		exit(1);
	}

	/* start discover service */
	ret = fork();
	if (ret < 0)
		exit(1);
	if (ret == 0) {
		char *if_name = (argc > 2) ? argv[2] : "wlan0";

		if (discover_service(if_name, argv[1])) {
			ERROR("discover_service failed");
			exit(1);
		}
		exit(0);
	}
	discover_pid = ret;

	/* shared memory between all child processes */
	shmid = shmget(SHM_KEY, sizeof(struct config), 0644 | IPC_CREAT);
	if (shmid < 0) {
		ERROR("shmget failed: %s", strerror(errno));
		goto error;
	}

	config = shmat(shmid, NULL, 0);
	if (config == (void *)(-1)) {
		ERROR("shmat failed");
		goto error;
	}

	memset(config, 0, sizeof(struct config));
	config->temp_thres = DEFAULT_TEMP;

#ifdef REAL_TARGET
	if (ds1820_search(config->dev)) {
		ERROR("Did not find any sensor");
		goto error;
	}
#endif

	ret = fork();
	if (ret < 0)
		goto error;
	if (ret == 0) {
		if (proc_switch(config)) {
			ERROR("proc_switch failed");
			exit(1);
		}
		exit(0);
	}
	config->switch_pid = ret;

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
			handle_sess(s, config);
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

error:
	kill(discover_pid, SIGTERM);
	if (config)
		kill(config->switch_pid, SIGTERM);

	while (1) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid < 0) {
			ERROR("waitpid: %s",  strerror(errno));
			break;
		} else if (pid == 0)
			break;
		DEBUG("proc %d ended", pid);
	}

	if (config && shmdt(config))
		ERROR("shmdt failed: %s", strerror(errno));

	return 0;
}
