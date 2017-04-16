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

static int fd_write(char *gpio, char *value, const char *func)
{
	int ret, fd = open(gpio, O_WRONLY);
	if (fd < 0) {
		ERROR("%s open failed: %s\n", func, strerror(errno));
		return -1;
	}
	ret = write(fd, value, strlen(value));
	if (ret < 0)
		ERROR("%s write failed: %s\n", func, strerror(errno));
	close(fd);

	return ret;
}

static int fd_read(char *gpio, char *str, int len, const char *func)
{
	int ret, fd = open(gpio, O_RDONLY);
	if (fd < 0) {
		ERROR("%s open failed: %s\n", func, strerror(errno));
		return -1;
	}
	ret = read(fd, str, len);
	if (ret < 0)
		ERROR("%s write failed: %s\n", func, strerror(errno));

	close(fd);
	return ret;
}
void gpio_dir(int gpio, int mode)
{
	char str[50];
	sprintf(str, "/sys/class/gpio/gpio%d/direction", gpio);
	fd_write(str,  mode == GPIO_MODE_IN ? "in" : "out", __func__);
}

void gpio_edge(int gpio, char *edge)
{
	char str[50];
	sprintf(str, "/sys/class/gpio/gpio%d/edge", gpio);
	fd_write(str, edge, __func__);
}

void gpio_conf(int gpio, int mode, char *edge)
{
	char str[50];
	int ret;

	sprintf(str, "/sys/class/gpio/gpio%d", gpio);
	if (!file_exists(str)) {
		sprintf(str, "%d", gpio);
		ret = fd_write("/sys/class/gpio/export", str, __func__);
		if (ret < 0)
			return;
	}

	usleep(100000);

	gpio_dir(gpio, mode);
	if (edge)
		gpio_edge(gpio, edge);
}

void gpio_set(int gpio, int value)
{
	char str[50], buf[10];
	sprintf(str, "/sys/class/gpio/gpio%d/value", gpio);
	sprintf(buf, "%d", value);
	fd_write(str, buf, __func__);
}

int gpio_get(int gpio)
{
	char str[50], buf[10];
	int ret;

	sprintf(str, "/sys/class/gpio/gpio%d/value", gpio);
	ret = fd_read(str, buf, sizeof(buf), __func__);
	if (ret != 1)
		return -1;

	ret = atoi(buf);
	if (ret < 0 || ret > 1)
		ret = -1;

	return ret;
}

void led_set(char *led, int value)
{
	char str[50], buf[10];
	sprintf(str, "/sys/class/leds/%s/brightness", led);
	sprintf(buf, "%d", value);
	fd_write(str, buf, __func__);
}

int led_get(char *led)
{
	char str[50], buf[10];
	int ret;

	sprintf(str, "/sys/class/leds/%s/brightness", led);
	ret = fd_read(str, buf, sizeof(buf), __func__);
	if (ret <= 0)
		return -1;

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
