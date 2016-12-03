#ifndef SWITCH_H
#define SWITCH_H

#define DEFAULT_PORT 8998

#define MULTICAST_ADDR "239.255.255.250"
#define MULTICAST_PORT 1602
#define DISCOVER_MSG "discover"
#define APP_NAME "radiator"

enum {
    CMD_SET_SW_POS,
    CMD_TOGGLE_MODE,
    CMD_SET_TEMP,
    CMD_GET_STATUS,
    CMD_NUM
};

struct cmd_header {
    int id;
    int len;
};

struct cmd {
    struct cmd_header header;
    union {
        int sw_pos;
        int temp;
    } u;
};

enum {
    STATUS_OK,
    STATUS_CMD_INVALID,
    STATUS_CMD_FAILED
};

struct resp_header {
    int id;
    int status;
    int len;
};

struct radiator1_status {
    int tempThres;
    int sw_pos;
    int temp;
};

struct radiator2_status {
    int tempThres;
    int sw_pos;
    int temp;
    int humidity;
};

union status {
    struct radiator1_status rad1;
    struct radiator2_status rad2;
};

struct resp {
    struct resp_header header;
    union status status;
};

#define NUM_DEV_TYPE 2

enum dev_type {
    RADIATOR1,
    RADIATOR2
};

static char dev_type_name[NUM_DEV_TYPE][15] = {
    "radiator1", /* struct ds1820_status */
    "radiator2" /* struct sht1x_status */
};

static int dev_type_resp_size[NUM_DEV_TYPE] = {
    sizeof(struct radiator1_status),
    sizeof(struct radiator2_status)
};


#endif // SWITCH_H
