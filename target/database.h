#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "data.h"

int db_connect(void);
void db_disconnect(void);
int db_measure_insert(struct data *d);

#endif
