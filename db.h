#pragma once
#ifndef __DB_H
#define __DB_H
#include <mysql.h>
extern MYSQL * mconn;
int init_db(const char * host, const char * user, const char * pwd, const char * db);

#endif