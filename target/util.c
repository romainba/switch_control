
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
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

void gpio_conf(int gpio)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/gpio/export", gpio);
	if (!file_exists(str) && system(str)) {
		ERROR("system %s failed", str);
		return;
	}

	sprintf(str, "echo out > /sys/class/gpio/gpio%d/direction", gpio);
	if (system(str))
		ERROR("system %s failed", str);
}

void gpio_set(int gpio, int value)
{
	char str[50];
	sprintf(str, "echo %d > /sys/class/gpio/gpio%d/value", value, gpio);
	if (system(str))
		ERROR("system %s failed", str);
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
