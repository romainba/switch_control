#ifndef __SERVER_SWITCH_H__
#define __SERVER_SWITCH_H__

#define PORT 8998

enum {
	CMD_SET_SW,
	CMD_READ_TEMP,
	CMD_NUM
};

struct cmd {
	int id;
	int len;
	union {
		int sw_pos;
	} u;
};

enum {
	STATUS_OK,
	STATUS_CMD_INVALID,
	STATUS_CMD_FAILED
};

struct resp {
	int id;
	int status;
	int len;
	union {
		int temp;
	} u;
};

#endif
