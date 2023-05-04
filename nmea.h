#pragma once
#ifndef __NMEA_H
#define __NMEA_H

#include <string>
//#include <mysql/jdbc.h>
#include <string>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <chrono>
#include <map>
#include "mydefs.h"
using namespace std;
struct vdm_field
{
      int start, len;
      string field_name;
      int type, def, exp;
};
typedef int (*FnPtr)(StringArrayBulk * data);
struct nmea_sentence
{
      string description;
      int  grouped;
      FnPtr handler;
};
struct nmea_talker
{
      string description;
      int obsolete;

};
extern map<string, nmea_talker> talkers;
extern map<string, nmea_sentence> sentences;
extern map<int, vector<vdm_field> > vdm_defs;
extern map<int, int> vdm_length;
extern map<string, FnPtr> lut;



struct own_vessel_class
{
private:
      // gps 2
      // glonass 1
      // prev pos 0
      vector<floatpt> pos_arr;

      int pos_priority;
      void eval_pos() {
            pos = pos_arr[pos_priority];
            pos.latlon2meter();

      }
public:
      floatpt pos;
      bool pos_ok()
      {
            return pos_priority >= 0;
      };
      void set_pos(floatpt position, int priority)
      {
            pos_arr[priority] = position;
            if (priority >= pos_priority)
            {
                  pos_priority = priority;
                  eval_pos();
            }

      };

      int heading;
      bool relative;
      own_vessel_class()
      {
            pos_arr.resize(3);
            heading = 0;
            relative = false;
            pos_priority = -1;

      }


};
extern own_vessel_class own_vessel;

#define NMEA_GOOD 0x0000
#define NMEA_EMPTY 0x0001
#define NMEA_WRONG_HEADER 0x0002
#define NMEA_NO_ASTERISK 3
#define NMEA_NO_CHECKSUM 4
#define NMEA_BAD_CHECKSUM 5
#define NMEA_NO_DATA 6
#define NMEA_UNKNOWN_TALKER 7
#define NMEA_UNKNOWN_SENTENCE -1


#define LAT_DEFAULT 0x3412140 
#define LON_DEFAULT 0x6791AC0 



extern map<int,vessel> vessels;

struct satellite
{
      int snr, elevation, azimuth;
      uint64_t last_access;
};
extern map<int, satellite> sattelites;









unsigned int parse_nmea(string nmea_str);
bool parse_char(const string & s, char & c);
bool parse_int(const string & s, int & i);
bool parse_double(const string & s, double & f);

#define bitcollector_buff_len 150
#define bitcollector_buff_bits bitcollector_buff_len*8
#define bitcollector_vdm_max_payload 56
class bitcollector
{

private:

      unsigned char buff[bitcollector_buff_len];
      int length;
      int get_char_index(char c);
public:
      void add_bits(uint32 data, int32 data_len);
      void add_string(string data, size_t str_len);
      int get_len();
      uint32 get_raw(const int32 start, const int32 len);
      int32 get_int(const vdm_field * f);
      double get_double(const vdm_field * f);
      std::string get_string(const vdm_field * f);
      void add_vdm_str(std::string data, int pad);
      void clear();
      StringArray create_vdm(int & group_id);
      bitcollector();
};




#endif