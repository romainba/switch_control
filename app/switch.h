#ifndef SWITCH_H
#define SWITCH_H

#define PORT 8998
#define TEMPTHRES 20

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

struct status {
    int temp;
    int sw_pos;
};

struct resp {
    struct resp_header header;
    union {
        struct status status;
    } u;
};


#endif // SWITCH_H
