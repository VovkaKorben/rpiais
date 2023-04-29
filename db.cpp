#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include "db.h"
#include "mydefs.h"
#include "nmea.h"

MYSQL * mconn = NULL;
MYSQL_STMT * pstmt = NULL;



int get_myint(MYSQL_ROW row, const int index)
{
      if (row[index] == nullptr)
            return 0;
      size_t size = strlen(row[index]);
      char buff[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return atoi(buff);

}
std::string get_mystr(MYSQL_ROW row, const int index)
{

      size_t size = strlen(row[index]);
      char buff[size + 1];
      memcpy(buff, row[index], size);
      buff[size] = '\0';
      return std::string(buff);

}

int load_dicts(MYSQL * conn)
{
      MYSQL_RES * res;
      MYSQL_ROW row;
      int status;
      //      unsigned long * length;
      std::string sq, nmea;

      // load sentences 
      sq = read_file("/home/pi/projects/rpiais/sql/readdicts.sql");
      sentences.clear();
      if (mysql_query(conn, sq.c_str()))
      {
            printf("Query failed: %s\n", mysql_error(conn));
            return 1;
      }

      int dict_index = 0;
      do {
            res = mysql_store_result(conn);
            if (!res)
            {
                  printf("mysql_store_result failed: %s\n", mysql_error(conn));
                  return 1;
            }

            //unsigned int num_fields = mysql_num_fields(res);

            while ((row = mysql_fetch_row(res))) {
                  //length = mysql_fetch_lengths(res);
                  switch (dict_index) {
                  case 0: {
                        std::string  str_id = get_mystr(row, 0);
                        std::transform(str_id.begin(), str_id.end(), str_id.begin(), ::toupper);
                        if (lut.count(str_id) == 0)
                              throw std::runtime_error("no handler for " + str_id);
                        sentences.insert({ str_id, {
                                    get_mystr(row, 1),
                                    get_myint(row, 2),
                                    lut[str_id]
                                    } });
                        break;
                  }
                  case 1: // load talkers
                  {
                        std::string str_id = get_mystr(row, 0);
                        talkers.insert({ str_id, {
                              get_mystr(row, 1),
                              get_myint(row,2),
                              } });
                        break;
                  }
                  case 2: {
                        vdm_length[get_myint(row, 0)] = get_myint(row, 1);
                        break;
                  }
                  case 3: {
                        //int xx = get_myint(row, 5);

                        int msg_id = get_myint(row, 0);
                        if (vdm_defs.count(msg_id) == 0)
                              vdm_defs[msg_id] = {};

                        std::string fieldname = get_mystr(row, 3); // `ref` value
                        std::transform(fieldname.begin(), fieldname.end(), fieldname.begin(), ::toupper);
                        vdm_defs[msg_id].push_back({
                              get_myint(row, 1),// start
                              get_myint(row, 2),// len
                              fieldname,
                              get_myint(row, 4),// type
                              get_myint(row, 5),// def
                              get_myint(row, 6),// exp
                                                   });
                        break;
                  }

                  }
                  putchar('\n');
            }
            mysql_free_result(res);
            dict_index++;
            status = mysql_next_result(conn);
      } while (status == 0);


      return 0;


}

int init_db(const char * host, const char * user, const char * pwd, const char * db)
{
      // Format a MySQL object
      mconn = mysql_init(NULL);

      // pstmt = mysql_con->prepareStatement("select `id`,`data` from `rawdata`  where `id`> ? order by `id`");
       // Establish a MySQL connection
      if (!mysql_real_connect(mconn, host, user, pwd, db, 3306, NULL, 0)) {
            printf("MySQL error: %s.\n", mysql_error(mconn));
            return 1;
      }
      if (mysql_set_server_option(mconn, MYSQL_OPTION_MULTI_STATEMENTS_ON)) {
            printf("Setting Multi-Statement Option failed: %s\n", mysql_error(mconn));
            return 1;
      }

      pstmt = mysql_stmt_init(mconn);
      if (!pstmt)
      {
            printf("MySQL error: %s.\n", mysql_error(mconn));
            return 1;
      }
      std::string pstmt_str = read_file("/home/pi/projects/rpiais/sql/mapread.sql");

      if (mysql_stmt_prepare(pstmt, pstmt_str.c_str(), pstmt_str.length()))
      {
            // sLog.outError("SQL: mysql_stmt_prepare() failed for '%s'", m_szFmt.c_str());
             //sLog.outError("SQL ERROR: %s", mysql_stmt_error(m_stmt));
            return 1;
      }

      if (load_dicts(mconn))
      {
            printf("Error load dicts.\n");
            return 1;
      }
      return 0;

}