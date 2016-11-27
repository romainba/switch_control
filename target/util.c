#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

	sprintf(str, "echo %s > /sys/class/gpio/gpio%d/direction",
		mode == GPIO_MODE_IN ? "in" : "out", gpio);
	if (system(str))
		ERROR("gpio_dir %d failed\n", gpio);
}

void gpio_conf(int gpio, int mode)
{
	char str[50];
	struct stat st;

	sprintf(str, "/sys/class/gpio/export/gpio%d", gpio);
	if (stat(str, &st)) {
		sprintf(str, "echo %d > /sys/class/gpio/export", gpio);
		if (system(str))
			ERROR("gpio_conf %d failed\n", gpio);
	}
	gpio_dir(gpio, mode);
}

void gpio_set(int gpio, int value)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/gpio/gpio%d/value", value, gpio);
	if (system(str))
		ERROR("gpio_set %d failed\n", gpio);
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
	sprintf(str, "%c %s:%d", level, file, line);
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
