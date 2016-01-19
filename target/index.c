
#include <stdio.h>
#include "sensor.h"

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

void haut(char *);
void bas(void);


int main(void)
{

	printf("Content-Type: text/html;\n\n");

haut("Ma page en C !");


printf("Radiator temperature %d°C", get_temp() / 1000);


     bas();

     return 0;

}


/* On sépare le squelette HTML du reste du code */

void haut(char *title) {

     printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"fr\" >\n\t<head>");

     printf("\t\t<title>%s</title>", title);

     printf("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n\t</head>\n\t<body>");

}


void bas() {

     printf("\t</body>\n</html>");

}

