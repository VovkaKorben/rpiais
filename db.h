#pragma once
#ifndef __DB_H
#define __DB_H
#include <vector>
#include <map>
#include <mysql.h>
#include "mydefs.h"

#define PREPARED_NMEA 0
#define PREPARED_MAP1 1
#define PREPARED_MAP2 2
#define PREPARED_GPS 3



class  mysql_driver
{
private:
      MYSQL_ROW row;
      MYSQL * connection;
      MYSQL_RES * res;

      std::map<int, std::string> pstmt;
      std::vector<std::string> fields;

      const char * last_error_str;
      size_t field_index(std::string field_name);
public:
      MYSQL * get_connection() { return connection; };
      const char * get_last_error_str() { return last_error_str; };

      int get_myint(const size_t index);
      int get_myint(const std::string field_name);
      std::string get_mystr(const size_t index);
      std::string get_mystr(const std::string field_name);
      double get_myfloat(const size_t index);
      double get_myfloat(const std::string field_name);

      int exec_file(const std::string filename);
      int exec(const std::string query);

      bool prepare(const int index, std::string filename);
      template<typename ... Args> bool exec_prepared(const int index, Args ... args) {
            if (pstmt.count(index) == 0)  return false;
            std::string sq = pstmt[index];
            if (sizeof...(args) > 0)
                  sq = string_format(sq, args ...);
            return exec(sq);
      };
      bool store();
      bool  has_next();
      bool fetch();
      void  free_result();
      void magic_close();
      my_ulonglong row_count();



      mysql_driver(const char * host, const char * user, const char * pwd, const char * db);
      ~mysql_driver();

};
//bool load_dicts(mysql_driver * driver);
int init_db(mysql_driver * driver);


extern mysql_driver* mysql;

#endif