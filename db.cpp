#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include "db.h"
#include "mydefs.h"
#include "nmea.h"



std::string read_text(std::string filename)
{
      ifstream f(filename, std::ifstream::binary);
      if (!f) return "";
      std::stringstream buffer;
      buffer << f.rdbuf();
      return buffer.str();
}
bool mysql_driver::exec_file(std::string filename) {


      std::string query = read_text(filename);
      if (query.empty()) return false;
      exec(query);

}
bool mysql_driver::exec(std::string query) {
      if (!connection) return false;

      if (mysql_query(connection, query.c_str()))
      {
            last_error_str = mysql_error(connection);
            return false;
      }
      return true;

}
bool  mysql_driver::store()
{
      res = mysql_store_result(connection);
      if (!res)
      {
            last_error_str = mysql_error(connection);
            return false;
      }

      unsigned int num_fields = mysql_num_fields(res);
      MYSQL_FIELD * field;

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
bool mysql_driver::fetch() {
      row = mysql_fetch_row(res);
      return (row != NULL);

}
void mysql_driver::free_result() {
      mysql_free_result(res);
}
int mysql_driver::row_count() {
      return mysql_num_rows(res);
}
bool mysql_driver::prepare(const int index, const char * filename) {
      std::string query = read_text(filename);
      if (query.empty()) return false;
      //
      if (pstmt.count(index)!=0)  return false;
      //std::map<int, MYSQL_STMT *> pstmt;
      MYSQL_STMT * ps = mysql_stmt_init(connection);
}


mysql_driver::mysql_driver(const char * host, const char * user, const char * pwd, const char * db) {

      last_error_str = NULL;
      MYSQL * xx = mysql_init(NULL);
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
int mysql_driver::field_index(const std::string field_name)
{
      auto it = find(fields.begin(), fields.end(), field_name);

      // If element was found
      if (it == fields.end())
            return -1;

      return it - fields.begin();
}
int mysql_driver::get_myint(const int index)
{
      if (row[index] == nullptr)
            return 0;
      size_t size = strlen(row[index]);
      char buff[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return atoi(buff);

}
int mysql_driver::get_myint(const std::string field_name)
{
      int index = field_index(field_name);
      if (index == -1)
            throw std::runtime_error("field not found: " + field_name);
      return get_myint(index);
}
std::string mysql_driver::get_mystr(const int index)
{

      size_t size = strlen(row[index]);
      char buff[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return std::string(buff);

}
std::string mysql_driver::get_mystr(const std::string field_name)
{
      int index = field_index(field_name);
      if (index == -1)
            throw std::runtime_error("field not found: " + field_name);
      return get_mystr(index);
}
double mysql_driver::get_myfloat(const int index)
{
      if (row[index] == nullptr)
            return 0;
      size_t size = strlen(row[index]);
      char buff[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return std::stod(buff);

}
double mysql_driver::get_myfloat(const std::string field_name)
{
      int index = field_index(field_name);
      if (index == -1)
            throw std::runtime_error("field not found: " + field_name);
      return get_myfloat(index);
}

bool load_dicts(mysql_driver * driver)
{

      int status;
      //      unsigned long * length;
      std::string sq, nmea;

      // load sentences 

      sentences.clear();
      driver->exec_file("/home/pi/projects/rpiais/sql/readdicts.sql");

      int dict_index = 0;
      do {
            if (!driver->store())
                  return false;


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
                        //int xx = get_myint(row, 5);

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

                  }
                  putchar('\n');
            }
            driver->free_result();

            dict_index++;

      } while (driver->has_next());


      return true;


}
int init_db(mysql_driver * driver)
{
      std::string sq;

      driver->prepare(PREPARED_NMEA, "/home/pi/projects/rpiais/sql/nmearead.sql");
      driver->prepare(PREPARED_MAP, "/home/pi/projects/rpiais/sql/mapread.sql");

      driver->exec_file("/home/pi/projects/rpiais/sql/mapread_init.sql");
      driver->has_next();

      // read last gps position
      driver->exec_file("/home/pi/projects/rpiais/sql/gpsread.sql");
      driver->store();

      sq = read_file("/home/pi/projects/rpiais/sql/gpsread.sql");
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