#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

const char path[] = "/sys/bus/w1/devices";

int ds1820_search(char *dev)
{
	DIR *dir;
	struct dirent *dirent;

	dir = opendir(path);
	if (!dir) {
		perror ("Couldn't open the w1 devices directory");
		return 1;
	}
	*dev = 0;

	while ((dirent = readdir (dir))) {
		// 1-wire devices are links beginning with 28-
		if (dirent->d_type == DT_LNK &&
		    strstr(dirent->d_name, "10-") != NULL) {
			strcpy(dev, dirent->d_name);
			break;
		}
	}
	closedir (dir);

	return 0;
}

int ds1820_get_temp(char *dev, int *temp)
{
	char devPath[128]; // Path to device
	ssize_t numRead;
	char buf[256];     // Data from device
	char tmpData[6];   // Temp C * 1000 reported by device

	sprintf(devPath, "%s/%s/w1_slave", path, dev);

	// Read temp continuously
	// Opening the device's file triggers new reading
	int fd = open(devPath, O_RDONLY);
	if (fd < 0) {
		perror("open failed");
		return 1;
	}
	numRead = read(fd, buf, 256);
	if (numRead < 0) {
		perror ("read failed");
		return 1;
	} else if (numRead == 0) {
		printf("eof");
		return 1;
	}

	strncpy(tmpData, strstr(buf, "t=") + 2, 5);
	*temp = atoi(tmpData);

	close(fd);

	return 0;
}
