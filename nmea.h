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
#include "ais_types.h"
#include "pixfont.h"
//inline double dm_s2deg(double dm_s)
//{
//      int	int_part = (int)trunc(dm_s);
//      return (int_part / 100 + (int_part % 100 + (dm_s - int_part)) / 60);
//}

//struct coordinate
//{
//      int32 d, m;
//      double s;
//      char side;
//};
//double convert_ddmm_to_deg(double c, int32 is_lat)
//{
//      double r = c / 100;
//      c = remainder(c, 100.0);
//      r += c / 60.0;
//      return r;
//
//}
//
//coordinate convert_ddmm_to_dms(double c, int32 is_lat)
//{
//      coordinate r;
//      r.d = c / 100;
//      double m = remainder(c, 100.0);
//      r.m = (int32)trunc(c) % 100;
//
//
//}

struct vdm_field
{
      int start, len;
      std::string field_name;
      int type, def, exp;
};
typedef int32(*FnPtr)(StringArrayBulk* data, std::string talker);
struct nmea_sentence
{
      std::string description;
      int  grouped;
      FnPtr handler;
};
struct nmea_talker
{
      std::string description;
      int obsolete;

};
extern std::map<std::string, nmea_talker> talkers;
extern std::map<std::string, nmea_sentence> sentences;
extern std::map<int, std::vector<vdm_field> > vdm_defs;
extern std::map<int, int> vdm_length;
extern std::map<std::string, FnPtr> lut;

struct mid_struct_s
{
      std::string code, country;
};

extern std::map<int, mid_struct_s> mid_list;
extern std::map<std::string, image> mid_country;

#define NMEA_GOOD 0x0000
#define NMEA_EMPTY 0x0001
#define NMEA_WRONG_HEADER 0x0002
#define NMEA_NO_ASTERISK 3
#define NMEA_NO_CHECKSUM 4
#define NMEA_BAD_CHECKSUM 5
#define NMEA_NO_DATA 6
#define NMEA_UNKNOWN_TALKER 7
#define NMEA_UNKNOWN_SENTENCE -1





int32 parse_nmea(std::string nmea_str);
bool parse_char(const std::string& s, char& c);
bool parse_int(const std::string& s, int& i);
bool parse_double(const std::string& s, double& f);

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
      void add_string(std::string data, size_t str_len);
      int get_len();
      uint32 get_raw(const int32 start, const int32 len);
      int32 get_int(const vdm_field* f);
      double get_double(const vdm_field* f);
      std::string get_string(const vdm_field* f);
      void add_vdm_str(std::string data, int pad);
      void clear();
      StringArray create_vdm(int& group_id);
      bitcollector();
};




#endif