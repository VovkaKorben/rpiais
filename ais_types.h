#pragma once
#ifndef __AISTYPES_H
#define __AISTYPES_H
#include <map>
#include <vector>
#include "mydefs.h"


struct poly {

      int points_count;
      floatpt * origin;
      intpt * work;
      uint64_t last_access;

      int path_count;
      frect bounds;
      int * pathindex;

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
      double  turn, speed, course, distance;
      std::string shipname, callsign;
      poly figure;
      bool size_ok, pos_ok, angle_ok;
      floatpt gps;
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
            origin = new floatpt[5];
            work = new intpt[5];

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
struct own_vessel_class
{
private:
      // gps 2
      // glonass 1
      // prev pos 0
      int pos_count;
      std::vector<floatpt> pos_arr;

      int pos_priority, heading;
      bool heading_set, relative;
      floatpt gps, meters;

public:
      /////////////////////////////////////////////////////////////////////////
      inline floatpt get_gps()
      {
            return pos_arr[pos_priority];
      };
      inline floatpt get_meters()
      {
            return meters;
      };
      bool pos_ok()
      {
            return pos_priority >= 0;
      };
      void set_pos(floatpt position_gps, int priority)
      {
            if (priority < 0 || priority >= pos_count)
                  throw "own_vessel_class: priority index error";
            pos_arr[priority] = position_gps;

            if (priority >= pos_priority) {
                  pos_priority = priority;
                  meters = position_gps;
                  meters.latlon2meter();
            }
      };
      /////////////////////////////////////////////////////////////////////////
      inline int  get_heading() { return heading; }
      void set_heading(int h)
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
            pos_count = 3;
            pos_arr.resize(pos_count);
            pos_priority = -1;

            heading_set = false;
            relative = false;


      }



};
struct satellite
{
      int snr, elevation, azimuth;
      uint64_t last_access;
};


extern std::map<int, satellite> sattelites;
extern own_vessel_class own_vessel;
extern std::map<int, vessel> vessels;


//extern own_vessel_class own_vessel;
//extern map<int, vessel> vessels;
//extern map<int, satellite> sattelites;



#endif