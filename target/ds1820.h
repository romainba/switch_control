#ifndef __SENSOR_H__
#define __SENSOR_H__

int ds1820_search(char *dev);
int ds1820_get_temp(char *dev, int *temp);

#endif
