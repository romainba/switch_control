#include <stdio.h>
#include "sensor.h"

int main(void)
{
	int ret,t;
	char dev[30];

	ret = ds1820_search(dev);
	if (ret) {
		printf("sensor not found\n");
		return 0;
	}

	printf("sensor %s\n", dev);

	ret = ds1820_get_temp(dev, &t);
	if (ret) {
		printf("sensor read failed\n");
		return 0;
	}

	printf("temp %d\n", t);
	return 0;
}
