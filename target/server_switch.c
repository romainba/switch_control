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
#include <fcntl.h>
#include <semaphore.h>

#include "util.h"
#include <switch.h>
#include "discover.h"

#define MEASURE_PERIOD 5
#define TEMPTHRES 25
#define DEFAULT_TEMP (25 * 1000) /* degres */

/*
 * Radiator 1
 */
#ifdef CONFIG_RADIATOR1
#include "ds1820.h"
#define GPIO_SW 7

/* led in /sys/class/leds/ */
#define LED2 "tp-link:green:wps"  /* gpio 26 */
#define LED3 "tp-link:green:3g"   /* gpio 27 */
#define LED4 "tp-link:green:wlan" /* gpio 0 */
#define LED5 "tp-link:green:lan"  /* gpio 17 */
#endif

/*
 * Radiator 2
 */
#ifdef CONFIG_RADIATOR2
#include "RPi_SHT1x.h"
//#define GPIO_SW  17
#define GPIO_LED 2
#define GPIO_BUTTON 17
#endif

/*
 * Common structure
 */
struct config {
#ifdef CONFIG_RADIATOR1
	char dev[20];
#endif
#ifdef CONFIG_RADIATOR2
	int humidity;
#endif
	int requested; /* switch on requested */
	int temp;
	int temp_thres;
	int active; /* switch state */
	int switch_pid;
	sem_t mutex;
};

enum { REQ_NONE, REQ_ON, REQ_OFF };

#define SHM_KEY 0x1234


static inline void switch_set(int on)
{
	DEBUG("%s %d", __func__, on);

#if (defined CONFIG_RADIATOR1)
	led_set(LED2, on ? 255 : 0);
#endif
#if (defined CONFIG_RADIATOR2)
	gpio_set(GPIO_LED, on ? 1 : 0);
#endif
#ifdef GPIO_SW
	gpio_set(GPIO_SW, on ? 255 : 0);
#endif
}

static void send_switch_req(struct config *config, int req)
{
	union sigval sv;

	sv.sival_int = req;
	if (sigqueue(config->switch_pid, SIGUSR1, sv))
		ERROR("sigqueue pid %d error %s", config->switch_pid,
		      strerror(errno));
}

static void update_switch(struct config *config, int req)
{
	int new_active = 0;

	//printf("sem_wait\n");
	sem_wait(&config->mutex);

	if (req)
		config->requested = (req == REQ_ON);
	
	if (config->requested)
		new_active = config->temp <= config->temp_thres;

	DEBUG("requested %d active %d\n", config->requested, new_active);
 	
	if (config->active != new_active) {
		switch_set(new_active);
		config->active = new_active;
	}

	sem_post(&config->mutex);
	//printf("sem_post\n");
}

static void init_switch(struct config *config)
{
#ifdef CONFIG_RADIATOR1
	if (ds1820_search(config->dev)) {
		ERROR("Did not find any sensor");
		goto error;
	}
#endif

#ifdef CONFIG_RADIATOR2
	SHT1x_InitPins();
	SHT1x_Reset();
	gpio_conf(GPIO_LED, GPIO_MODE_OUT, NULL);
#endif

#ifdef GPIO_SW
	gpio_conf(GPIO_SW, GPIO_MODE_OUT, NULL);
#endif

	config->active = 1;
	sem_init(&config->mutex, 0, 1);
	update_switch(config, REQ_OFF);
}

static int proc_measure(struct config *config)
{
	config->temp = 23 * 1000;

	while (1) {

#ifdef CONFIG_RADIATOR1
		if (ds1820_get_temp(config->dev, &config->temp)) {
			ERROR("sensor reading failed");
			return 1;
		}
#endif

#ifdef CONFIG_RADIATOR2
		unsigned short int val;
		float humi, temp;

		if (!SHT1x_Measure_Start(SHT1x_MEAS_T)) {
			ERROR("SHT1 measure start failed");
			return 1;
		}

		if (!SHT1x_Get_Measure_Value(&val)) {
			ERROR("SHT1 measure get failed");
			return 1;
		}
		temp = (float)val;

		// Request Humidity Measurement
		if (!SHT1x_Measure_Start(SHT1x_MEAS_RH)) {
			ERROR("SHT1 measure start failed");
			return 1;
		}

		// Read Humidity measurement
		if (!SHT1x_Get_Measure_Value(&val)) {
			ERROR("SHT1 measure get failed");
			return 1;
		}
		humi = (float)val;

		SHT1x_Calc(&humi, &temp);
		printf("temp %0.2f, humidity %0.2f\n", temp, humi);

		config->temp = (int)(temp * 1000.0);
		config->humidity = (int)(humi * 1000.0);
#endif
		update_switch(config, 0);
		
		sleep(MEASURE_PERIOD);
	}
	return 0;
}



static int proc_switch(struct config *config)
{
	int ret;
	sigset_t waitset;
	siginfo_t info;

	sigemptyset(&waitset);
	sigaddset(&waitset, SIGUSR1);

	DEBUG("proc_switch %d\n", getpid());
	
	if (sigprocmask(SIG_SETMASK, &waitset, NULL) < 0) {
		ERROR("sigprocmask failed\n");
		return 1;
	}

	DEBUG("proc_switch loop\n");
	
	while (1) {
		ret = sigwaitinfo(&waitset, &info);
		if (ret < 0) {
			if (errno == EAGAIN) {
				/* timeout */
				DEBUG("proc_switch eagain");
				continue;
			} else {
				ERROR("proc_switch: sigwaitinfo failed %s",
				      strerror(errno));
				break;
			}
		}
		update_switch(config, info.si_value.sival_int);
	}
	DEBUG("%s done", __func__);
	return 0;
}

#ifdef GPIO_BUTTON
static int proc_button(struct config *config)
{
	struct pollfd fds = { .events = POLLPRI };
	int ret;
	char str[50];
	union sigval sv;

	gpio_conf(GPIO_BUTTON, GPIO_MODE_IN, "rising");

	sprintf(str, "/sys/class/gpio/gpio%d/value", GPIO_BUTTON);
	fds.fd = open(str, O_RDONLY | O_NONBLOCK);
	if (fds.fd < 0) {
		ERROR("gpio open failed\n");
		return -1;
	}

	while (1) {
		ret = poll(&fds, 1, -1);
		if (ret < 0) {
			ERROR("poll error %s", strerror(errno));
			break;
		}

		lseek(fds.fd, 0, SEEK_SET);
		ret = read(fds.fd, str, sizeof(str));

		config->requested = !config->requested;
		); 
	}
	return -1;
}
#endif

static int proc_socket(int s, struct config *config)
{
	struct pollfd fds = { .fd = s, .events = POLLIN | POLLERR };
	struct resp resp;
	struct cmd cmd;
	int ret, req, n, update;

	while (1) {
		/* wait event */
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

		req = REQ_NONE;
		update = 1;

		switch (cmd.header.id) {
		case CMD_SET_SW_POS:
			req = cmd.u.sw_pos /* 0 off, 1 on */ ? REQ_ON : REQ_OFF;
			break;
		case CMD_GET_STATUS: {
#if (defined CONFIG_RADIATOR1) || (defined CONFIG_SIMU)
			resp.header.len = sizeof(struct radiator1_status);
			resp.status.rad1.temp = config->temp;
			resp.status.rad1.tempThres = config->temp_thres;
			resp.status.rad1.sw_pos = config->requested;
#endif
#ifdef CONFIG_RADIATOR2
			resp.header.len = sizeof(struct radiator2_status);
			resp.status.rad2.temp = config->temp;
			resp.status.rad2.tempThres = config->temp_thres;
			resp.status.rad2.sw_pos = config->requested;
			resp.status.rad2.humidity = config->humidity;
#endif
			update = 0;
			break;
		}
		case CMD_TOGGLE_MODE:
			DEBUG("toggle");
			req = config->requested ? REQ_OFF : REQ_ON;
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

		DEBUG("send resp %d len %d update %d req %d\n", resp.header.id,
		      resp.header.len, update, req);

		if (update)
			update_switch(config, req);
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
	int port = DEFAULT_PORT;
	int meas_pid = 0;

	if (argc < 2 || argc > 4) {
		printf("usage: %s <name> [<ethernet if> [<port>]]\n", argv[0]);
		exit(1);
	}

	if (argc == 4)
		port = atoi(argv[3]);

	/* start discover service */
	ret = fork();
	if (ret < 0)
		exit(1);
	if (ret == 0) {
		char *if_name = (argc > 2) ? argv[2] : "wlan0";

		if (discover_service(if_name, argv[1], port)) {
			ERROR("discover_service failed");
			exit(1);
		}
		exit(0);
	}
	discover_pid = ret;

	/* shared memory between all child processes */
	shmid = shmget(SHM_KEY + port, sizeof(struct config), 0644 | IPC_CREAT);
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
	init_switch(config);
	
#ifdef GPIO_BUTTON
	/* lauch proc_button process */
	ret = fork();
	if (ret < 0)
		goto error;
	if (ret == 0) {
		if (proc_button(config)) {
			ERROR("proc_button failed");
			exit(1);
		}
		exit(0);
	}
#endif

	/* lauch proc_measure process */
	ret = fork();
	if (ret < 0)
		goto error;
	if (ret == 0) {
		if (proc_measure(config)) {
			ERROR("proc_measure failed");
			exit(1);
		}
		exit(0);
	}
	meas_pid = ret;

	/* lauch proc_switch process */
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

	ret = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) < 0) {
		ERROR("setsockopt(SO_REUSEADDR) failed");
		goto error;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	bind(sock, (struct sockaddr *) &sin, sizeof(sin));

	listen(sock, 5);
	printf("listening port %d\n", port);

	while (1) {

		s = accept(sock, NULL, NULL);
		if (s < 0) {
			ERROR("accept error %s", strerror(errno));
			break;
		}

		/* lauch proc_socket process */
		pid = fork();
		if (pid < 0)
			ERROR("fork: %s",  strerror(errno));
		else if (pid == 0) {
			proc_socket(s, config);
			exit(0);
		}

		while (1) {
			pid = waitpid(-1, &status, WNOHANG);
			if (pid < 0) {
				ERROR("waitpid: %s",  strerror(errno));
				break;
			} else if (pid == 0)
				break;
			DEBUG("proc %d ended status %d", pid, status);
		}

		close(s);
		DEBUG("closed socket");
	}

	close(sock);

error:
	kill(discover_pid, SIGTERM);
	if (config)
		kill(config->switch_pid, SIGTERM);
	if (meas_pid)
		kill(meas_pid, SIGTERM);
		
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
