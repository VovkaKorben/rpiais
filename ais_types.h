#pragma once
#ifndef __AISTYPES_H
#define __AISTYPES_H
#include <map>
#include <vector>
#include "mydefs.h"


struct poly {

      int points_count;
      FloatPoint* origin;
      IntPoint* work;
      uint64_t last_access;

      int path_count;
      frect bounds;
      int* pathindex;

      poly()
      {
            points_count = 0;
      }
};


enum class mid_types
{
      diverradio, ships, groupships, coaststation, saraircraft, aidtonav, auxship, aissart, mobdevice, epirb
};

struct vessel :poly
{
public:
      uint32  imo, accuracy, status, heading, shiptype, top, bottom, left, right, mid;
      mid_types mid_type;
      int distance;
      double  turn, speed, course;
      std::string shipname, callsign;
      poly figure;
      bool size_ok, pos_ok, angle_ok;
      FloatPoint gps;
      void eval_mid(uint32 mmsi)
      {
            switch (mmsi / 100000000) // check first digit
            {
                  case 0:mid_type = mid_types::groupships;                  mid = mmsi / 100000;                  break;
                  case 8:mid_type = mid_types::diverradio;                  mid = (mmsi / 100000) % 1000;                  break;
                  default:switch (mmsi / 10000000) // check 2 first
                  {
                        case 00:mid_type = mid_types::coaststation;                  mid = (mmsi / 10000) % 1000;                  break;
                        case 98:mid_type = mid_types::auxship;                  mid = (mmsi / 10000) % 1000;                  break;
                        case 99:mid_type = mid_types::aidtonav;                  mid = (mmsi / 10000) % 1000;                  break;
                        default:switch (mmsi / 1000000) // check 3 first
                        {
                              case 111:mid_type = mid_types::saraircraft;                  mid = (mmsi / 1000) % 1000;                  break;
                              case 970:mid_type = mid_types::aissart;                  mid = (mmsi / 1000) % 1000;                  break;
                              case 972:mid_type = mid_types::mobdevice;                  mid = (mmsi / 1000) % 1000;                  break;
                              case 974:mid_type = mid_types::epirb;                  mid = (mmsi / 1000) % 1000;                  break;
                              default:mid_type = mid_types::ships;                  mid = mmsi / 1000000;                  break;
                        }
                  }
            }

      }
      vessel()
      {
            size_ok = false;
            pos_ok = false;

            shipname = "";


#ifdef __GNUC__ //GNU_C_compiler



            last_access = utc_ms();
#endif

            points_count = 5;
            origin = new FloatPoint[5];
            work = new IntPoint[5];

            path_count = 1;
            pathindex = new int[2];
            pathindex[0] = 0;
            pathindex[1] = 5;
      }
      void size_changed() {
            double l = left, r = right, t = top, b = bottom;
            origin[0] = { t,(l + r) / 2 - r }; // bow center
            origin[1] = { (t + b) * 0.75 - b,l }; // bow left connector
            origin[2] = { -b, l }; // left bottom point
            origin[3] = { -b, -r }; // right bottom point
            origin[4] = { (t + b) * 0.75 - b,-r };// bow right connector
      }
};

enum  position_type_e
{
      //unknown = 0,
      previous, gga, rmc, gns, gll, POS_MAX
};
enum gnss_type
{
      gps, glonass, waas
};
struct own_vessel_class
{
private:
      std::vector<FloatPoint> pos_arr;

      position_type_e pos_priority;
      int32 heading;
      bool heading_set, relative;
      FloatPoint gps, meters;

public:
      char sel_mode, mode;

      /////////////////////////////////////////////////////////////////////////
      inline FloatPoint get_gps()
      {
            return pos_arr[(int32)pos_priority];
      };
      inline FloatPoint get_meters()
      {
            return meters;
      };
      //bool pos_ok()      {            return pos_priority >= 0;      };
      void set_pos(FloatPoint position_gps, position_type_e priority)
      {
            //if (priority < 0 || priority >= pos_count)                  throw "own_vessel_class: priority index error";
            pos_arr[(int)priority] = position_gps;

            if (priority >= pos_priority) {
                  pos_priority = priority;
                  meters = position_gps;
                  meters.latlon2meter();
            }
      };
      /////////////////////////////////////////////////////////////////////////
      inline int  get_heading() { return heading; }
      void set_heading(int32 h)
      {
            heading = h;
            heading_set = true;
      };
      /////////////////////////////////////////////////////////////////////////
      inline bool  get_relative() { return relative; }
      void set_relative()
      {
            if (heading_set)
                  relative = !relative;
            else relative = false;
      }
      /////////////////////////////////////////////////////////////////////////
      own_vessel_class()
      {
            pos_arr.resize((int32)position_type_e::POS_MAX);
            pos_priority = position_type_e::previous;

            heading_set = false;
            relative = false;


      }



};

struct satellite
{
      int32 snr, elevation, azimuth;
      gnss_type gnss;
      int32 used;
      uint64_t last_access;
      int32 is_active()
      { // satellite remains active, if it accessed last 60 seconds
            return ((utc_ms() - last_access) < 60000) ? 1 : 0;

      }
};


class satellites
{

public:
      std::map<int32, satellite> list; // map [prn] = params
      satellites()
      {
            int32 c;
            satellite s;
            s.last_access = 0;
            s.used = 0;
            s.gnss = gnss_type::gps;
            for (c = 0; c <= 32; c++)
            {
                  list.emplace(c, s);
            }
            s.gnss = gnss_type::waas;
            for (c = 33; c <= 64; c++)
            {
                  list.emplace(c, s);
            }
            s.gnss = gnss_type::glonass;
            for (c = 65; c <= 89; c++)
            {
                  list.emplace(c, s);
            }

      }
      std::vector<gnss_type> get_gnss_type(IntVec* sat_list)
      {
            std::vector<gnss_type> r;
            r.clear();
            gnss_type gnss;
            for (const int32 prn : *sat_list)
            {
                  if (list.count(prn) == 0)
                  {
                        printf("[!] Satellite with PRN %d not found\n", prn);
                        continue;
                  }
                  gnss = list[prn].gnss;
                  std::vector<gnss_type>::iterator it = std::find(r.begin(), r.end(), gnss);
                  if (it == r.end())
                        r.push_back(gnss);
            }
            return r;
      }
      void reset_used(gnss_type gnss) // reset used flag for entire GNSS type
      {
            for (auto s : list)
            {
                  if (s.second.gnss == gnss)
                        s.second.used = 0;
            }
      }
      void set_used(IntVec* used_list) // set used flag for selected PRNs
      {
            for (int32 prn : *used_list)
            {
                  list[prn].used = 1;
            }

      }
      int32 get_used_count()
      {
            int32 r = 0;
            for (auto const& s : list)
            {
                  if (s.second.used)
                        r++;
            }
            return r;
      }
      int32 get_active_count()
      {
            int32 r = 0;
            for (auto& s : list)
            {
                  if (s.second.is_active())
                        r++;
            }
            return r;
      }
};


//extern 
extern own_vessel_class own_vessel;
extern std::map<int, vessel> vessels;
extern satellites sat;

#endif