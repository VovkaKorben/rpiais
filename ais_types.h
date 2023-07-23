#pragma once
#ifndef __AISTYPES_H
#define __AISTYPES_H
#include <map>
#include <vector>
#include "mydefs.h"
#include "db.h"

//  forward declarations
struct  IntPoint;
struct  FloatPoint;

// points
struct PolarPoint
{
      double angle, dist;
      void from_float(FloatPoint* fp);
      void add(PolarPoint pp);
      IntPoint to_int();
      void zoom(double z);
      void rotate(double a);
};
struct  FloatPoint {
      double x, y;
      void latlon2meter();
      IntPoint to_int();
      void substract(FloatPoint fp);
      PolarPoint to_polar();
      int32 haversine(FloatPoint fp);
};
struct IntPoint
{
      int32 x, y;
      PolarPoint to_polar();
      IntPoint transform(PolarPoint pp);
      void add(IntPoint pt);
      void add(int32 x, int32 y);
};

//rects
struct IntRect
{
      //private:
      int32 l, b, r, t;
      //int32 _get_coord(int32 index);
      //public:
      int32 left() { return l; }
      int32 bottom() { return b; }
      int32 right() { return r; }
      int32 top() { return t; }
      int32 width() { return r - l; }
      int32 height() { return t - b; }

      void init(int32 x, int32 y);
      void modify(int32 x, int32 y);
      //void make(int32 left, int32 bottom, int32 right, int32 top);
      bool is_intersect(IntRect* rct);
      IntRect transform(PolarPoint pp);
      //IntRect() {};
      //IntRect(int32 l, int32 b, int32 r, int32 t);
      void collapse(int32 x, int32 y);
      void zoom(double z);
      void offset(IntPoint o);
      void offset(int32 x, int32 y);
      IntPoint center();

};
struct FloatRect
{
      double l, b, r, t;
      double left() { return l; }
      double bottom() { return b; }
      double right() { return r; }
      double top() { return t; }
      void zoom(double z);
      void offset(FloatPoint o);
      FloatRect() {}
      FloatRect(IntRect rct);
};

#define max_vertices 500
struct bucket {
      int y, ix;
      double fx, slope;
};
struct bucketset {
      int cnt;
      bucket barr[max_vertices];
};


struct Poly {
private:
      std::vector<FloatPoint> origin;
      std::vector<IntPoint> work;
      std::vector<int32> path_ptr;
      void edge_tables_reset();
      void edge_store_tuple_float(bucketset* b, int y_end, double  x_start, double  slope);
      void edge_store_tuple_int(bucketset* b, int y_end, double  x_start, double  slope);
      void edge_store_table(IntPoint pt1, IntPoint pt2);
      void edge_update_slope(bucketset* b);
      void edge_remove_byY(bucketset* b, int scanline_no);
      void calc_fill();
      void draw_fill(const ARGB color);
      //FloatRect float_bounds;
      IntRect bounds;
      //int32 points_count;
public:
     
      void add_point(double x, double y);
      void add_point(FloatPoint fp);
      void add_path();
      void load_finished();

      void clear();
      IntRect get_bounds() const;
      //IntRect get_bounds(PolarPoint pp);
      //IntRect get_bounds();
      //IntRect transform_bounds(PolarPoint pp);
      void transform(PolarPoint pp);
      Poly();
};


struct IntCircle {
      int32 x, y, r;
};

enum class mid_types
{
      diverradio, ships, groupships, coaststation, saraircraft, aidtonav, auxship, aissart, mobdevice, epirb
};

struct vessel
{
public:
      uint32  imo, accuracy, status, heading, shiptype, top, bottom, left, right, mid;
      mid_types mid_type;
      uint64_t last_access;
      int distance;
      double  turn, speed, course;
      std::string shipname, callsign;
      Poly shape;
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
            last_access = utc_ms();

            /*points_count = 5;
            origin = new FloatPoint2[5];
            work = new IntPoint2[5];

            path_count = 1;
            pathindex = new int[2];
            pathindex[0] = 0;
            pathindex[1] = 5;*/
      }
      void size_changed() {
            // boat direction to 0 degrees forward
           // double l = left, r = right, t = top, b = bottom;
            shape.add_path();
            shape.add_point(top, (left + right) / 2 - right); // bow center
            shape.add_point((top + bottom) * 0.75 - bottom, left); // bow left connector
            shape.add_point(-bottom, left); // left bottom point
            shape.add_point(-bottom, -right); // right bottom point
            shape.add_point((top + bottom) * 0.75 - bottom, -right);// bow right connector
      }
};

enum  position_type_e
{
      //unknown = 0,
      gga, rmc, gns, gll
};
enum gnss_type
{
      gps, glonass, waas
};
extern int32 gps_session_id;

struct own_vessel_class
{
private:
      FloatPoint position[4], meters[4];

      int32 pos_index;
      int32 heading, _heading_set, relative;

      //      FloatPoint get_gps();      meters;

public:
      char sel_mode, mode;

      /////////////////////////////////////////////////////////////////////////
      FloatPoint get_pos()
      {
            if (pos_index >= 0)
                  return  position[pos_index];
            else return { 0.0,0.0 };
      };
      FloatPoint get_meters()
      {
            if (pos_index >= 0)
                  return  meters[pos_index];
            else return { 0.0,0.0 };
      };
      void set_pos(FloatPoint position_gps, position_type_e priority)
      {
            int32 int_prio = (int32)priority;
            position[int_prio] = position_gps;
            meters[int_prio] = position_gps;
            meters[int_prio].latlon2meter();

            if (int_prio >= pos_index)
            {
                  //pchar xxx = bigint2str(utc_ms());
                 // std::string stringValue = std::to_string(utc_ms());
                  pos_index = int_prio;
                  mysql->exec_prepared(PREPARED_GPS, std::to_string(utc_ms()).c_str(), gps_session_id, position_gps.x, position_gps.y);

            }
      };
      /////////////////////////////////////////////////////////////////////////
      inline int32  get_heading() { return heading; }
      void set_heading(int32 h)
      {
            heading = h;
            _heading_set = 1;
      };
      /////////////////////////////////////////////////////////////////////////
      inline int32  is_relative() { return relative; }
      void set_relative()
      {
            relative ^= 1;
      }
      /////////////////////////////////////////////////////////////////////////
      own_vessel_class()
      {
            pos_index = -1;
            _heading_set = 0;
            relative = 0;
      }
      int32 get_pos_index() {
            return pos_index;
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
            for (c = 65; c <= 96; c++)
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
            for (auto& s : list)
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

struct map_shape {
      uint64_t last_access;
      Poly shape;
};

extern std::map <int32, map_shape>  map_shapes;
extern own_vessel_class own_vessel;
extern std::map<int32, vessel> vessels;
extern satellites sat;

#endif