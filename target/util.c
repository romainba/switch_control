#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef SYSLOG
#include <syslog.h>
#endif
#include <signal.h>

#include "util.h"

int file_exists(char *file)
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

void gpio_dir(int gpio, int mode)
{
	char str[50];
	int ret;
	
	sprintf(str, "echo %s > /sys/class/gpio/gpio%d/direction",
		mode == GPIO_MODE_IN ? "in" : "out", gpio);
	ret = system(str);
	if (ret)
	  ERROR("'%s' failed %d\n", str, ret);
}

void gpio_edge(int gpio, char *edge)
{
	char str[50];
	int ret;
	
	sprintf(str, "echo %s > /sys/class/gpio/gpio%d/edge",
		edge, gpio);
	ret = system(str);
	if (ret)
	  ERROR("'%s' failed %d\n", str, ret);
}


void gpio_conf(int gpio, int mode, char *edge)
{
	char str[50];
	int ret;
	
	sprintf(str, "/sys/class/gpio/gpio%d", gpio);
	if (!file_exists(str)) {
		sprintf(str, "echo %d > /sys/class/gpio/export", gpio);
		ret = system(str);
		if (ret)
		  ERROR("'%s' failed %d\n", str, ret);
	}

	usleep(100000);

	gpio_dir(gpio, mode);
	if (edge)
	  gpio_edge(gpio, edge);
}

void gpio_set(int gpio, int value)
{
	char str[50];
	int ret;
	
	sprintf(str, "echo %d > /sys/class/gpio/gpio%d/value", value, gpio);
	ret = system(str);
	if (ret)
	  ERROR("'%s' failed %d\n", str, ret);
}

int gpio_get(int gpio)
{
	char str[50];
	int fd, n, ret = 0;

	sprintf(str, "/sys/class/gpio/gpio%d/value", gpio);
	fd = open(str, O_RDONLY);
	n = read(fd, str, sizeof(str));
	if (n < 0) {
		printf("read gpio failed\n");
		ret = -1;
		goto error;
	}

	ret = str[0] - '0';
	if (ret < 0 || ret > 1) {
		ret = -1;
		goto error;
	}

error:
	close(fd);
	return ret;
}


void led_set(char *led, int value)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/leds/%s/brightness", value, led);
	if (system(str))
		ERROR("system %s failed", str);
}

int led_get(char *led)
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

void logger_init(void)
{
#ifdef SYSLOG
  openlog("switch", 0, LOG_USER);
#endif
}


void logger(int priority, const char *file, int line,
	    const char *func,const char *fmt, ...)
{
	char str[301];
	va_list args;
	char level = ' ';

	switch (priority) {
	case LOG_ERR: level = 'E'; break;
	case LOG_WARNING: level = 'W'; break;
	case LOG_DEBUG: level = 'D'; break;
	case LOG_INFO: level = 'I'; break;
	default: break;
	}

	//sprintf(str, "%c %s:%d:%s", level, file, line, func);
	sprintf(str, "%c %d %s:%d", level, getpid(), file, line);
	if (*fmt) {
		strcat(str, " ");
		va_start(args, fmt);
		vsnprintf(str + strlen(str), 300 - strlen(str), fmt, args);
		va_end(args);
	}
	if (str[strlen(str)-1] != '\n')
		strcat(str, "\n");

#ifdef SYSLOG
	syslog(priority, str);
#else
	printf("%s", str);
#endif
}

static char pid_file_name[] = "switch-control.pid";
static int pid_fd = -1;

/**
 * \brief Callback function for handling signals.
 * \param	sig	identifier of signal
 */
void handle_signal(int sig)
{
	if (sig == SIGINT) {
		INFO("Debug: stopping daemon ...\n");
		/* Unlock and close lockfile */
		if (pid_fd != -1) {
			lockf(pid_fd, F_ULOCK, 0);
			close(pid_fd);
		}
		/* Try to delete lockfile */
		if (pid_file_name != NULL) {
			unlink(pid_file_name);
		}
		//running = 0;
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
	} else if (sig == SIGHUP) {
		INFO("Debug: reloading daemon config file ...\n");
		//read_conf_file(1);
	} else if (sig == SIGCHLD) {
	  	INFO("Debug: received SIGCHLD signal\n");
	}
}

/**
 * \brief This function will daemonize this app
 */
void daemonize()
{
	pid_t pid = fork();
	int fd;

	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* On success: The child process becomes session leader */
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	/* Ignore signal sent from child to parent process */
	signal(SIGCHLD, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Set new file permissions */
	//umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
		close(fd);
	}

	/* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	/* Try to write PID of daemon to lockfile */
	if (pid_file_name != NULL)
	{
		char str[256];
		pid_fd = open(pid_file_name, O_RDWR|O_CREAT, 0640);
		if (pid_fd < 0) {
			/* Can't open lockfile */
			exit(EXIT_FAILURE);
		}
		if (lockf(pid_fd, F_TLOCK, 0) < 0) {
			/* Can't lock file */
			exit(EXIT_FAILURE);
		}
		/* Get current PID */
		sprintf(str, "%d\n", getpid());
		/* Write PID to lockfile */
		write(pid_fd, str, strlen(str));
	}
}
