#pragma once
#ifndef __DB_H
#define __DB_H
#include <mysql.h>
extern MYSQL * mconn;
extern std::string sql_nmea, sql_map;

int get_myint(MYSQL_ROW row, const int index);
std::string get_mystr(MYSQL_ROW row, const int index);
double get_myfloat(MYSQL_ROW row, const int index);

int init_db(const char * host, const char * user, const char * pwd, const char * db);

#endif