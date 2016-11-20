#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include "util.h"
#include "sensor.h"
#include "switch.h"

#if 0
static inline int get_temp(void)
{
	int temp = 0;
	char dev[20];

	if (ds1820_search(dev)) {
		printf("Did not find any sensor");
		return 0;
	}

	if (ds1820_get_temp(dev, &temp))
		printf("sensor reading failed");

	return temp;
}
#endif

int endian(int v)
{
	return (((v & 0xff) << 24) |
		((v & 0xff00) << 8) |
		((v & 0xff0000) >> 8) |
		((v & 0xff000000) >> 24));
}

static int get_status(struct status *s)
{
	int sock, ret;
	struct sockaddr_in sin;
	struct resp resp;
	struct cmd cmd;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = connect(sock, (struct sockaddr *) &sin, sizeof(sin));
	if (ret < 0) {
		ERROR("connect error %s", strerror(errno));
		return -1;
	}

	cmd.header.id = CMD_GET_STATUS;
	cmd.header.len = 0;

	ret = write(sock, &cmd, sizeof(struct cmd_header));
	if (ret < 0) {
		ERROR("write error %s", strerror(errno));
		return -1;
	}

	ret = read(sock, &resp, sizeof(struct resp_header));
	if (ret < 0) {
		ERROR("read error %s", strerror(errno));
		return -1;
	}

	if (resp.header.status) {
		ERROR("read error status %d", resp.header.status);
		return -1;
	}
	ret = read(sock, s, sizeof(struct status));
	if (ret < 0) {
		ERROR("read error %s", strerror(errno));
		return -1;
	}

	close(sock);
	return 0;
}

void haut(char *);
void bas(void);

int main(void)
{
	struct status s;

	printf("Content-Type: text/html;\n\n");
	haut("Radiateur");

	if (get_status(&s)) {
		printf("error\n");
		goto bas;
	}

	printf("<br></br>");

	printf("<br>Radiateur %s</br>", s.sw_pos ? "ON" : "OFF");
	printf("<br>Temperature %d.%d deg</br>",  s.temp / 1000, (s.temp / 100) % 10);
	printf("<br>Seuil %d.%d deg</br>",  s.tempThres / 1000, (s.tempThres / 100) % 10);

bas:
	bas();

	return 0;

}


void haut(char *title) {
	printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"fr\" >\n\t<head>");

	printf("\t\t<title>%s</title>", title);

	printf("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n\t</head>\n\t<body>");
}


void bas() {
	printf("\t</body>\n</html>");
}
