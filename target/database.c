#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <mysql/mysql.h>
#include "util.h"
#include "configs.h"
#include "database.h"

#define DB_NAME     "switch_control"
#define DB_IP       "localhost"
#define DB_USERNAME "root"
#if (MODULE_ID==0)
#define DB_PASSWORD "abilis"
#else
#define DB_PASSWORD "toto1234"
#endif
#define DB_TABLE    "measures"

static MYSQL *mysql = NULL;

int db_connect(void)
{
	char s[200];
	MYSQL_RES *result;

	mysql = mysql_init(NULL);
	if (!mysql) {
		ERROR("mysql_init failed");
		return -1;
	}

	if (!mysql_real_connect(mysql, DB_IP, DB_USERNAME, DB_PASSWORD, NULL,
				0, NULL, 0))
		goto error;

	if (mysql_select_db(mysql, DB_NAME)) {
		DEBUG("mysql_select_db failed: %s\n", mysql_error(mysql));
		INFO("creating database");
		snprintf(s, sizeof(s), "CREATE DATABASE %s", DB_NAME);
		if (mysql_query(mysql, s))
			goto error;
	}

	if (mysql_query(mysql, "SHOW TABLES LIKE 'measures'"))
		goto error;

	result = mysql_store_result(mysql);
	if (!mysql_num_rows(result)) {
		INFO("creating table 'measures'");
		if (mysql_query(mysql,"CREATE TABLE measures("
			"Id INT PRIMARY KEY AUTO_INCREMENT, "
			"module INT, date DATETIME, temp INT, "
			"humidity INT, state INT)"))
			goto error;
	}
	mysql_free_result(result);
	return 0;

error:
	ERROR("mysql failed: %s", mysql_error(mysql));
	mysql_close(mysql);
	return -1;
}

int db_measure_insert(struct data *data)
{
	char s[200];
	char date_str[32];
	time_t t = time(NULL);
	struct tm *to = localtime(&t);

	if (!data || !mysql)
		return -1;

	strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", to);

	snprintf(s, sizeof(s),
		 "INSERT INTO measures(module, date, temp, humidity, state) "
		 "VALUES(%d, '%s', %d, %d, %d)",
		 MODULE_ID, date_str, data->temp, data->humidity, data->active);
	if (mysql_query(mysql, s)) {
		ERROR("mysql_query failed: %s", mysql_error(mysql));
		return -1;
	}
	return 0;
}

void db_disconnect(void)
{
	if (mysql)
		mysql_close(mysql);
	mysql = NULL;
}

#if 0
int db_fetch(char *query)
{
	if (mysql_query(mysql, query)) {
		DEBUG("mysql_query failed: %s\n", mysql_error(mysql));
		return -1;
	}

	MYSQL_RES *result = mysql_store_result(mysql);
	if (result == NULL)
		return 0;

	int num_rows = mysql_num_rows(result);
	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;   //An array of strings
	while ((row = mysql_fetch_row(result))) {
		if (num_fields >= 2) {
			char *value_int = row[0];
			char *value_string = row[1];

			printf( "Got value %s\n", value_string);
		}
	}
	mysql_free_result(result);
	return 0;
}
#endif
