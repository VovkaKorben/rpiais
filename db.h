#pragma once
#ifndef __DB_H
#define __DB_H
#include <vector>
#include <map>
#include "mydefs.h"
#include <mysql/jdbc.h>
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


class  mysql_driver
{
private:
      sql::Connection* conn;
      sql::ResultSet* res = nullptr;

      std::map<int32, std::string> pstmt;
      std::vector<std::string> fields;


      size_t field_index(std::string field_name);
      void _print_error(std::string msg);
public:
      void close_res()
      {
            if (res != nullptr) {
                  res->close();
                  res = nullptr;
            }
      }
      //MYSQL* get_connection() { return connection; };

      int32 get_int(const std::string field_name);
      std::string get_str(const std::string field_name);
      double get_double(const std::string field_name);

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

      bool  fetch();
      //bool fetch();      void  free_result();      void magic_close();
      //my_ulonglong row_count();

      //int32 get_more())

      mysql_driver(const char* host, const char* user, const char* pwd, const char* db);
      ~mysql_driver();

};
//bool load_dicts(mysql_driver * driver);
int init_db(mysql_driver* driver);


extern mysql_driver* mysql;

#endif