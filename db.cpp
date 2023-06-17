#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include  <stdint.h>
#include "db.h"
#include "mydefs.h"
#include "nmea.h"
#include "ais_types.h"



std::string read_text(std::string filename)
{
      std::ifstream f(filename, std::ifstream::binary);
      if (!f) {
            std::cout << "[db:read_text] File not found: " << filename << std::endl;
            return "";
      }
      std::stringstream buffer;
      buffer << f.rdbuf();
      return buffer.str();
}
int mysql_driver::exec_file(std::string filename) {


      std::string query = read_text(filename);
      if (query.empty()) return false;
      return exec(query);

}
int mysql_driver::exec(std::string query) {
      //printf("%s\n", query.c_str());

      if (!connection) return false;



      if (mysql_query(connection, query.c_str()))
      {
            fprintf(stderr, " mysql_query() failed\n");
            fprintf(stderr, " query: %s\n", query.c_str());
            fprintf(stderr, " %s\n", mysql_error(connection));
            last_error_str = mysql_error(connection);
            printf("Query FAILED: %s\n\n", query.c_str());
            return 1;
      }
      //printf("\n\n--- Query OK ---\n\n");
      return 0;

}
bool  mysql_driver::store()
{
      res = mysql_store_result(connection);
      if (!res)
      {
            std::cout << mysql_error(connection) << std::endl;
            last_error_str = mysql_error(connection);
            return false;
      }

      unsigned int num_fields = mysql_num_fields(res);
      MYSQL_FIELD* field;

      fields.clear();
      fields.reserve(num_fields);
      for (unsigned int i = 0; i < num_fields; i++) {
            field = mysql_fetch_field(res);
            fields.push_back(field->name);
      }

      return true;
}
bool  mysql_driver::has_next() {
      if (!connection) return false;


      int status = mysql_next_result(connection);
      return status == 0;
}
void mysql_driver::magic_close() {
      for (; mysql_next_result(connection) == 0;)
            printf("<magic_close>\n");
}
bool mysql_driver::fetch() {
      row = mysql_fetch_row(res);
      return (row != NULL);

}
void mysql_driver::free_result() {
      mysql_free_result(res);
}
my_ulonglong mysql_driver::row_count() {
      return mysql_num_rows(res);
}
bool mysql_driver::prepare(const int index, std::string filename) {
      if (pstmt.count(index) != 0)  return false;
      std::string query = read_text(filename);
      if (query.empty()) return false;
      pstmt.emplace(index, query);
      return true;
}


mysql_driver::mysql_driver(const char* host, const char* user, const char* pwd, const char* db) {

      last_error_str = NULL;
      MYSQL* xx = mysql_init(NULL);
      connection = xx;

      if (!mysql_real_connect(connection, host, user, pwd, db, 3306, NULL, 0))
      {
            last_error_str = mysql_error(connection);
            return;
      }
      if (mysql_set_server_option(connection, MYSQL_OPTION_MULTI_STATEMENTS_ON)) {
            last_error_str = mysql_error(connection);
            return;
      }

}
mysql_driver::~mysql_driver() {
      mysql_close(connection);
}
size_t mysql_driver::field_index(const std::string field_name)
{

      for (size_t i = 0; i < fields.size(); i++)
      {
            if (fields[i] == field_name)
                  return i;

      }
      return SIZE_MAX;

}
int mysql_driver::get_myint(const size_t index)
{
      if (row[index] == nullptr)
            return 0;
      size_t size = strlen(row[index]);
      //char buff[size + 1];
      pchar buff = new char[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return atoi(buff);

}
int mysql_driver::get_myint(const std::string field_name)
{
      size_t index = field_index(field_name);
      if (index == SIZE_MAX)
            throw std::runtime_error("field not found: " + field_name);
      return get_myint(index);
}
std::string mysql_driver::get_mystr(const size_t index)
{

      size_t size = strlen(row[index]);
      pchar buff = new char[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return std::string(buff);

}
std::string mysql_driver::get_mystr(const std::string field_name)
{
      size_t index = field_index(field_name);
      if (index == SIZE_MAX)
            throw std::runtime_error("field not found: " + field_name);
      return get_mystr(index);
}
double mysql_driver::get_myfloat(const size_t index)
{
      if (row[index] == nullptr)
            return 0;
      size_t size = strlen(row[index]);
      pchar buff = new char[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return std::stod(buff);

}
double mysql_driver::get_myfloat(const std::string field_name)
{
      size_t index = field_index(field_name);
      if (index == SIZE_MAX)
            throw std::runtime_error("field not found: " + field_name);
      return get_myfloat(index);
}

int load_dicts(mysql_driver* driver)
{
      sentences.clear(); talkers.clear(); vdm_defs.clear(); vdm_length.clear(); mid_list.clear(); mid_country.clear();
      driver->exec_file(data_path("/sql/readdicts.sql"));

      int dict_index = 0;
      do {
            if (!driver->store())
                  return 1;


            while (driver->fetch()) {
                  switch (dict_index) {
                  case 0: {
                        std::string  str_id = driver->get_mystr("id");
                        std::transform(str_id.begin(), str_id.end(), str_id.begin(), ::toupper);
                        if (lut.count(str_id) == 0)
                              throw std::runtime_error("no handler for " + str_id);
                        sentences.insert({ str_id, {
                                    driver->get_mystr("description"),
                                    driver->get_myint("grouped"),
                                    lut[str_id]
                                    } });
                        break;
                  }
                  case 1: // load talkers
                  {
                        std::string str_id = driver->get_mystr("id");
                        talkers.insert({ str_id, {
                              driver->get_mystr("description"),
                              driver->get_myint("obsolete"),
                              } });
                        break;
                  }
                  case 2: {
                        vdm_length[driver->get_myint("id")] = driver->get_myint("len");
                        break;
                  }
                  case 3: {
                        int msg_id = driver->get_myint("msg_id");
                        if (vdm_defs.count(msg_id) == 0)
                              vdm_defs[msg_id] = {};

                        std::string fieldname = driver->get_mystr("ref"); // `ref` value
                        std::transform(fieldname.begin(), fieldname.end(), fieldname.begin(), ::toupper);
                        vdm_defs[msg_id].push_back({
                              driver->get_myint("start"),// start
                              driver->get_myint("len"),// len
                              fieldname,
                              driver->get_myint("type"),// type
                              driver->get_myint("def"),// def
                              driver->get_myint("exp"),// exp
                              });
                        break;
                  }
                  case 4: { // MID data

                        //std::map<int, mid_struct_s> mid_list;
                        //std::map<std::string, image> mid_country;


                        int mid = driver->get_myint("mid");
                        mid_struct_s mid_s;
                        mid_s.code = driver->get_mystr("abbr");
                        mid_s.country = driver->get_mystr("country");
                        mid_list.insert({ mid, mid_s });
                        //img_minus = new image();
                        std::string filename = string_format(data_path("/img/flags/%s.png"), mid_s.code.c_str());
                        if (file_exists(filename))
                              mid_country.insert({ mid_s.code, image(filename) });
                        else
                              std::cout << "File not found: " << filename.c_str() << std::endl;


                        break;
                  }

                  }
            }
            driver->free_result();

            dict_index++;

      } while (driver->has_next());


      return 0;


}
int init_db(mysql_driver* driver)
{
      driver->exec_file(data_path("/sql/mapread_init.sql"));
      driver->has_next();

      driver->prepare(PREPARED_NMEA, data_path("/sql/nmearead.sql"));
      driver->prepare(PREPARED_MAP1, data_path("/sql/mapread1.sql"));
      driver->prepare(PREPARED_MAP2, data_path("/sql/mapread2.sql"));

      //driver->exec_prepared(PREPARED_NMEA, 1234);

      // read last gps position
      if (driver->exec_file(data_path("/sql/gpsread.sql"))) return 1;
      driver->store();
      // driver->exec_file("/home/pi/projects/rpiais/sql/gpsread.sql");
      if (driver->row_count() == 1)
      {
            driver->fetch();

            own_vessel.set_pos({
                  driver->get_myfloat("lon"),
                  driver->get_myfloat("lat")
                  }, 0);
      }
      driver->has_next();


      // load NMEA tables
      if (load_dicts(driver))
      {
            printf("Error load dicts.\n");
            return 1;
      }
      return 0;

}