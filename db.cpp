#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include  <stdint.h>
#include "db.h"
#include "mydefs.h"
#include "nmea.h"
#include "ais_types.h"
#include <regex>


int32 gps_session_id;
mysql_driver* mysql;
bool getFirstWord(const std::string& input, std::string& word) {
      std::regex word_regex("(\\w+)");
      std::smatch matches;

      if (std::regex_search(input, matches, word_regex)) {
            word = matches.str(1);
            for (char& c : word) {
                  c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }

            return true;
      }
      return false;
}
StringArray split_query(std::string query)
{
      query = std::regex_replace(query, std::regex("--.+($|\\r|\\n)"), ""); //remove comments
      query = std::regex_replace(query, std::regex("\\t|\\r|\\n"), " "); // remove all control characters
      query = std::regex_replace(query, std::regex("\\s{2,}"), " "); // convert 2+ spaces to one

      StringArray r;
      std::string delimiter = ";", substr;
      size_t pos = 0, found_at, t;

      while (true)
      {
            t = found_at = query.find(delimiter, pos);
            if (t == std::string::npos)
                  t = query.length();

            substr = query.substr(pos, t - pos);
            substr = std::regex_replace(substr, std::regex("^\\s+|\\s+$"), ""); // trim spaces
            if (!substr.empty()) {
                  r.push_back(substr);
            }

            if (found_at == std::string::npos)
                  break;
            pos = found_at + 1;
      }

      //std::string token = s.substr(pos, s.find(delimiter));
      return r;
}
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
int32 mysql_driver::exec_file(std::string filename) {


      std::string query = read_text(filename);
      if (query.empty()) return false;
      return exec(query);


}



bool mysql_driver::prepare(const int index, std::string filename) {
      if (pstmt.count(index) != 0)  return false;
      std::string query = read_text(filename);
      if (query.empty()) return false;
      //query = BeautifyQuery(query);

      pstmt.emplace(index, query);
      return true;
}


int32 mysql_driver::get_int(const std::string field_name)
{
#ifdef USE_JDBC
      return       res->getInt(field_name);
#else
      return get_int(field_by_name(field_name));
#endif  
}
int32 mysql_driver::get_int(const int32 field_index)
{
#ifdef USE_JDBC
      return       res->getInt(field_index);
#else
      if (field_index < 0 || field_index >= num_fields)
            return 0.0;
      return row[field_index] ? atoi(row[field_index]) : 0;
#endif  
}
std::string mysql_driver::get_str(const std::string field_name)
{
#ifdef USE_JDBC
      return       res->getString(field_name);
#else
      return get_str(field_by_name(field_name));
#endif 

}
std::string mysql_driver::get_str(const int32 field_index)
{
#ifdef USE_JDBC

      return     res->getString(field_index);
#else
      if (field_index < 0 || field_index >= num_fields)
            return "";
      return row[field_index] ? row[field_index] : "";
#endif 

}
double mysql_driver::get_double(const std::string field_name)
{
#ifdef USE_JDBC
      return     (double)(res->getDouble(field_name));
#else
      return get_double(field_by_name(field_name));
#endif 
}
double mysql_driver::get_double(const int32 field_index)
{
#ifdef USE_JDBC
      return     (double)(res->getDouble(field_index));
#else
      if (field_index < 0 || field_index >= num_fields)
            return 0.0;
      return row[field_index] ? atof(row[field_index]) : 0.0;
#endif 

}

int32 load_dicts(mysql_driver* driver)
{
      sentences.clear(); talkers.clear(); vdm_defs.clear(); vdm_length.clear(); mid_list.clear(); mid_country.clear();
      driver->exec_file(data_path("/sql/dict/sentences.sql"));
      while (driver->fetch()) {
            std::string  str_id = driver->get_str("id");
            std::transform(str_id.begin(), str_id.end(), str_id.begin(), ::toupper);
            if (lut.count(str_id) == 0)
                  throw std::runtime_error("no handler for " + str_id);
            sentences.insert({ str_id, {
                        driver->get_str("description"),
                        driver->get_int("grouped"),
                        lut[str_id]
                        } });
      }

      driver->exec_file(data_path("/sql/dict/talkers.sql"));
      while (driver->fetch()) {
            std::string str_id = driver->get_str("id");
            talkers.insert({ str_id, {
                  driver->get_str("description"),
                  driver->get_int("obsolete"),
                  } });
      }

      driver->exec_file(data_path("/sql/dict/vdm_types.sql"));
      while (driver->fetch()) {
            vdm_length[driver->get_int("id")] = driver->get_int("len");
      }

      driver->exec_file(data_path("/sql/dict/vdm_defs.sql"));
      while (driver->fetch()) {
            int32 msg_id = driver->get_int("msg_id");
            if (vdm_defs.count(msg_id) == 0)
                  vdm_defs[msg_id] = {};

            std::string fieldname = driver->get_str("ref"); // `ref` value
            std::transform(fieldname.begin(), fieldname.end(), fieldname.begin(), ::toupper);
            vdm_defs[msg_id].push_back({
                  driver->get_int("start"),// start
                  driver->get_int("len"),// len
                  fieldname,
                  driver->get_int("type"),// type
                  driver->get_int("def"),// def
                  driver->get_int("exp"),// exp
                  });
      }

      driver->exec_file(data_path("/sql/dict/mid_codes.sql"));
      while (driver->fetch()) {
            int32 mid = driver->get_int("mid");
            mid_struct_s mid_s;
            mid_s.code = driver->get_str("abbr");
            mid_s.country = driver->get_str("country");
            mid_list.insert({ mid, mid_s });
            //img_minus = new image();
            std::string filename = string_format(data_path("/img/flags/%s.png"), mid_s.code.c_str());
            if (file_exists(filename))
                  mid_country.insert({ mid_s.code, image(filename) });
            else
                  printf("[i] loading MID codes, file not found: %s", filename.c_str());

      }

      return 0;


}
int32 init_db(mysql_driver* driver)
{
      driver->exec_file(data_path("/sql/map/map_init.sql"));

      driver->prepare(PREPARED_MAP_QUERY, data_path("/sql/map/map_query.sql"));
      driver->prepare(PREPARED_MAP_READ_BOUNDS, data_path("/sql/map/map_bounds.sql"));
      driver->prepare(PREPARED_MAP_READ_POINTS, data_path("/sql/map/map_points.sql"));

      driver->prepare(PREPARED_MAP_GARBAGE, data_path("/sql/map/map_garbage.sql"));
      driver->prepare(PREPARED_GPS, data_path("/sql/gps/set_gps_pos.sql"));
      driver->prepare(PREPARED_GPS_TOTAL, data_path("/sql/gps/get_gps_total.sql"));

      driver->prepare(PREPARED_DIST_STAT, data_path("/sql/dist/dist_stats.sql"));


      // read last gps session ID
      if (driver->exec_file(data_path("/sql/gps/get_gps_session.sql")))
            return 1;
      driver->fetch();
      gps_session_id = driver->get_int("sessid") + 1;


      // load NMEA tables
      if (load_dicts(driver))
      {
            printf("Error load dicts.\n");
            return 1;
      }
      return 0;

}

void mysql_driver::_print_error(std::string msg)
{
      /*   printf("[E][MySQL][%s] code: %d, state: %s, error: %s\n",
               msg.c_str(),
               mysql_errno(connection),
               mysql_sqlstate(connection),
               mysql_error(connection));
   */
}

std::map< std::string, int32> commands = {
      {"UNKNOWN",0},

      {"DROP",1},
      {"INSERT",1},
      {"UPDATE",1},
      {"DELETE",1},
      {"CREATE",1},

      {"SELECT",2},
};



int32 mysql_driver::exec(std::string query)
{
      try {

#ifdef USE_JDBC
            if (!con->isValid())
#else
            if (mysql_ping(con) != 0)
#endif
            {
                  printf("Connection is not valid\n");
                  return 100;
            }



            StringArray query_list = split_query(query);
            std::string command;
#ifdef USE_JDBC
            //sql::Statement* stmt = conn->createStatement();
            std::unique_ptr< sql::Statement > stmt(con->createStatement());
#endif
            for (const std::string& single_query : query_list)
            {
#ifdef QUERY_LOG
                  printf("[i] Query:\n\t%s\n", single_query.c_str());
#endif
                  if (!getFirstWord(single_query, command)) {
                        printf("[E] error while get query command\n");
                        continue;
                  }
                  if (commands.count(command) == 0) {
                        printf("[E] command not found: %s\n", command.c_str());
                        continue;
                  }
                  switch (commands[command])
                  {
                        case 0: printf("[E] this type of command don't supported\n");
                              break;


                        case 1: { //   DROP / INSERT / UPDATE / DELETE / CREATE

#ifdef USE_JDBC
                              if (!stmt->execute(single_query))
                              {
                                    
                                    std::cout << "Query execution failed." << std::endl;
                                    // Получение информации об ошибке
                                    const sql::SQLWarning *e = con->getWarnings();
                                    std::cout << "Error code: " << e->getErrorCode() << std::endl;
                                    std::cout << "SQL State: " << e->getSQLState() << std::endl;
                                    //std::cout << "Message: " << e->what() << std::endl;
                                    
                                    const sql::SQLWarning* warning = stmt->getWarnings();
                                    if (warning != nullptr) {
                                          printf("[E] execute %s warn: %s\n", command.c_str(), warning->getMessage().c_str());
                                    }
                                    return 200;
                              }
#else
                              if (mysql_query(con, single_query.c_str()) != 0) {
                                    fprintf(stderr, "CREATE TABLE failed: %s\n", mysql_error(con));
                                    return 200;
                              }
#endif
                              break;
                        }


                        case 2: { // SELECT
#ifdef USE_JDBC
                              res = stmt->executeQuery(single_query);
#else
                              if (mysql_query(con, single_query.c_str()) != 0) {
                                    fprintf(stderr, "Query execution failed: %s\n", mysql_error(con));
                                    return 300;
                              }

                              res = mysql_store_result(con);
                              if (res == NULL) {
                                    fprintf(stderr, "mysql_store_result() failed\n");
                                    return 400;
                              }

                              num_fields = mysql_num_fields(res);
                              fields = mysql_fetch_fields(res);
#endif


#ifdef QUERY_LOG
#ifdef USE_JDBC
                              // log for JDBC
                              sql::ResultSetMetaData* metaData = res->getMetaData();
                              printf("\t%d col(s)\n", metaData->getColumnCount());
                              res->last();
                              printf("\t%d row(s)\n", res->getRow());
                              res->beforeFirst();
#else
                              // log for MySQL C connector
#endif
#endif

                              break;
                        }
                  }


            }
            // delete stmt;
      }
#ifdef USE_JDBC
      catch (sql::SQLException& e) {
            std::cout << "SQL Exception:" << std::endl;
            std::cout << "Error Code: " << e.getErrorCode() << std::endl;
            std::cout << "SQL State: " << e.getSQLState() << std::endl;
            std::cout << "Message: " << e.what() << std::endl;
      }
#else
      catch (...)
      {
            //       catch (sql::SQLException& e) {
              //     std::cout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
                //   std::cout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
      }
#endif

      return 0;

}

