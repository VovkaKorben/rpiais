#pragma once
#ifndef __DB_H
#define __DB_H
#include <vector>
#include <map>
#include <mysql.h>

#define PREPARED_NMEA 0
#define PREPARED_MAP 1

class  mysql_driver
{
private:
      MYSQL_ROW row;
      MYSQL * connection;
      MYSQL_RES * res;
      
      std::map<int, MYSQL_STMT *> pstmt;
      std::vector<std::string> fields;

      const char * last_error_str;
      int field_index(std::string field_name);
public:
      MYSQL * get_connection() { return connection; };
      const char * get_last_error_str() { return last_error_str; };

      int get_myint(const int index);
      int get_myint(const std::string field_name);
      std::string get_mystr(const int index);
      std::string get_mystr(const std::string field_name);
      double get_myfloat(const int index);
      double get_myfloat(const std::string field_name);

      bool exec_file(const std::string filename);
      bool exec(const std::string query);

      // template<typename ... Args> inline std::string string_format(const std::string & format, Args ... args)

      template<typename ... Args> bool exec_prepared(const int index, Args ... args) {};
      bool store();
      bool  has_next();
      bool fetch();
      void  free_result();
      int row_count();

      bool prepare(const int index, const char * filename);

      mysql_driver(const char * host, const char * user, const char * pwd, const char * db);
      ~mysql_driver();

};
bool load_dicts(mysql_driver * driver);





#endif