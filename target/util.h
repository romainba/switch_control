#ifndef __UTIL_H__
#define __UTIL_H__

enum {
	LOG_ERR,
	LOG_WARNING,
	LOG_DEBUG,
	LOG_INFO
};

void logger(int priority, const char *file, int line,
	       const char *func,const char *fmt, ...);

#define ERROR(...) logger(LOG_ERR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define WARN(...)  logger(LOG_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG(...) logger(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define INFO(...)  logger(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif
