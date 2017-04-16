#ifndef __UTIL_H__
#define __UTIL_H__

int file_exists(char *file);

#ifdef SYSLOG
#include <syslog.h>
#else
enum {
	LOG_ERR,
	LOG_WARNING,
	LOG_DEBUG,
	LOG_INFO
};
#endif

void logger_init(void);

void logger(int priority, const char *file, int line,
	       const char *func,const char *fmt, ...);

enum {
	GPIO_MODE_IN,
	GPIO_MODE_OUT
};

void gpio_conf(int gpio, int mode, char *edge);
void gpio_dir(int gpio, int mode);
void gpio_set(int gpio, int value);
int gpio_get(int gpio);
void led_set(char *led, int value);
int led_get(char *led);

#define ERROR(...) logger(LOG_ERR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define WARN(...)  logger(LOG_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG(...) logger(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define INFO(...)  logger(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)

#if (defined RADIATOR1)
#define conv32(x) (((x & 0xff) << 24) | ((x & 0xff00) << 8) |		\
		   ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24))
#else
#define conv32(x) (x)
#endif

#endif
