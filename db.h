#ifdef USE_JDBC
#else
#endif


#pragma once
#ifndef __DB_H
#define __DB_H
#include <vector>
#include <map>
#include "mydefs.h"
#ifdef USE_JDBC
#include <mysql/jdbc.h>
#else
#include <mariadb/mysql.h>
//#include <mysql.h>
#endif
//#include "mysql_connection.h"
//#include <cppconn/driver.h>
//#include <cppconn/exception.h>
//#include <cppconn/resultset.h>
//#include <cppconn/statement.h>

//#define PREPARED_NMEA 0
#define PREPARED_MAP_GARBAGE 1
#define PREPARED_MAP_QUERY 200
#define PREPARED_MAP_READ_BOUNDS 201
#define PREPARED_MAP_READ_POINTS 202
#define PREPARED_GPS 3
#define PREPARED_GPS_TOTAL 4

#define PREPARED_DIST_STAT 301



class  mysql_driver
{
private:
#ifdef USE_JDBC
      sql::Connection* con;
      sql::ResultSet* res = nullptr;
      std::vector<std::string> fields;
#else
      int32 num_fields;
      MYSQL* con = nullptr;
      MYSQL_ROW row = nullptr;
      MYSQL_RES* res = nullptr;
      MYSQL_FIELD* fields = nullptr;
      int32 field_by_name(std::string field_name) {
            for (int32 c = 0; c < num_fields; c++) {
                  if (fields[c].name == field_name)
                        return c;
            }
            return -1;
      }
#endif
      std::map<int32, std::string> pstmt;



      size_t field_index(std::string field_name);
      void _print_error(std::string msg);
public:
      void close_res() {
#ifdef USE_JDBC
            if (res != nullptr) {
                  res->close();
                  res = nullptr;
            }
#else
            mysql_free_result(res);
#endif
      }





      int32 get_int(const std::string field_name);
      int32 get_int(const int32 field_index);
      std::string get_str(const std::string field_name);
      std::string get_str(const int32 field_index);
      double get_double(const std::string field_name);
      double get_double(const int32 field_index);


      int32 exec_file(const std::string filename);
      int32 exec(const std::string query);

      bool prepare(const int index, std::string filename);
      template<typename ... Args> bool exec_prepared(const int index, Args ... args) {
            if (pstmt.count(index) == 0)  return false;
            std::string sq = pstmt[index];
            if (sizeof...(args) > 0)
                  sq = string_format2(sq, args ...);
            return exec(sq);
      };

      bool  fetch() {
#ifdef USE_JDBC
            return res->next();
#else
            row = mysql_fetch_row(res);
            return row;
#endif

      }

      mysql_driver(const char* host, const char* user, const char* pwd, const char* db)
      {

#ifdef USE_JDBC
            try {
                  sql::ConnectOptionsMap options;
                  options["hostName"] = host;
                  options["userName"] = user;
                  options["password"] = pwd;
                  options["schema"] = db;
                  options["port"] = 3306;
                  //options["OPT_RECONNECT"] = true;
                  options["CLIENT_MULTI_STATEMENTS"] = true;
                  options["CLIENT_MULTI_RESULTS"] = true;

                  sql::Driver* driver = get_driver_instance();
                  con = driver->connect(options);
            }
            catch (sql::SQLException& e) {
                  std::cerr << "SQL exception in the database: "
                        << e.what() << std::endl;
            }
#else
            con = mysql_init(NULL);
            if (con == NULL) {
                  fprintf(stderr, "mysql_init() failed\n");
                  return;
            }
            if (mysql_real_connect(con, host, user, pwd, db, 0, NULL, CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS) == NULL) {
                  fprintf(stderr, "mysql_real_connect() failed\n");
                  mysql_close(con);
                  return;
            }
            printf("Connected to MariaDB!\n");

#endif  
      }
      ~mysql_driver() {
#ifdef USE_JDBC
#else
            mysql_close(con);
#endif
      }

};
//bool load_dicts(mysql_driver * driver);
int32 init_db(mysql_driver* driver);


extern mysql_driver* mysql;
extern int32 gps_session_id;
#endif