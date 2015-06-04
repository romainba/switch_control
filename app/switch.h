#ifndef SWITCH_H
#define SWITCH_H

#define PORT 8998

enum {
    CMD_SET_SW,
    CMD_READ_TEMP,
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

struct resp {
    struct resp_header header;
    union {
        int temp;
    } u;
};


#endif // SWITCH_H

