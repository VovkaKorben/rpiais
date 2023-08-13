#pragma once
#ifndef __AISTYPES_H
#define __AISTYPES_H
#include <map>
#include <vector>
#include "mydefs.h"
#include "db.h"
#include <cmath>

struct  IntPoint;
struct  FloatPoint;

// points
struct PolarPoint
{
      double angle, dist;
      void from_float(const FloatPoint* fp);
      FloatPoint to_float();
      void add(PolarPoint pp);
      IntPoint to_int();
      void zoom(double z);
      void rotate(double a);
      PolarPoint(double a = 0.0, double d = 1.0) : angle(a), dist(d) { }
      std::string dbg();
      
      
};
struct  FloatPoint {
      std::string dbg();
      double x, y;
      void latlon2meter();
      IntPoint to_int();
      void add(FloatPoint* fp);
      void sub(FloatPoint* fp);
      PolarPoint to_polar();
      //int32 haversine(FloatPoint fp);
      int32 haversine(FloatPoint* fp);
      void transform(const PolarPoint* transform_value);
      FloatPoint(double _x = 0.0, double _y = 0.0) :x(_x), y(_y) {};


};
struct IntPoint
{
      int32 x, y;
      PolarPoint to_polar();

      void transform(const IntPoint& center, const PolarPoint& transform_value);
      void transform(const PolarPoint& transform_value);

      void add(const IntPoint* pt);
      void sub(const IntPoint* pt);
      void add(int32 x, int32 y);
      std::string dbg() const;

      IntPoint(int32 _x = 0, int32 _y = 0) :x(_x), y(_y) {};

};

//rects
struct IntRect
{
      std::string dbg();
      //private:
      int32 l, b, r, t;
      //int32 _get_coord(int32 index);
      IntPoint get_corner(int32 index);
      //public:
      int32 left() { return l; }
      void left(int32 _left) { l = _left; }
      int32 bottom() { return b; }
      int32 right() { return r; }
      void right(int32 _right) { r = _right; }
      int32 top() { return t; }
      int32 width() { return r - l; }
      int32 height() { return t - b; }

      void init(IntPoint* pt);
      void modify(IntPoint* pt);

      //void init(int32 x, int32 y);      void modify(int32 x, int32 y);
      bool is_intersect(const IntRect& rct);
      //IntRect transform_bounds(const IntPoint& center, const PolarPoint& transform_value);
      void transform_bounds(const PolarPoint& transform_value);
      //IntRect transform(const PolarPoint& transform_value);

      //IntRect() {};
      //IntRect(int32 l, int32 b, int32 r, int32 t);
      void collapse(int32 x, int32 y);
      void zoom(double z);
      void add(IntPoint o);
      void add(int32 x, int32 y);
      void sub(IntPoint o);
      void sub(int32 x, int32 y);
      void sub(FloatPoint o);
      void sub(double x, double y);

      IntPoint center();

};
struct FloatRect
{
private:
      double _get_coord(int32 index);
public:
      FloatRect transform(const PolarPoint& transform_value);
      std::string dbg();
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

//#define max_vertices 500
struct bucket {

      float x, slope;
      int32 ye;
};
struct bucketset {
      int32 cnt;
      bucket* b;
};
class poly_driver_class
{
private:
      int32 h, maxv;
      bucketset aet, * et;
public:
      void reset() {
            aet.cnt = 0;
            for (int32 i = 0; i < h; i++)
                  et[i].cnt = 0;
      }


      poly_driver_class(int32 height, int32 max_vertices) {
            h = height;
            maxv = max_vertices;
            aet.b = new bucket[maxv];
            //aet = new bucketset(maxv);.barr = new bucket[maxv];
            et = new bucketset[h];
            for (int32 y = 0; y < h; y++)
                  et[y].b = new bucket[maxv];


            printf("poly_driver created, height:%d, vertices: %d\n", h, maxv);
      }
      ~poly_driver_class() {}
      bucket* get_inserted(float x) {

            if (aet.cnt == maxv)
                  throw "poly_driver_class: get_inserted maxv overcount";

            // search insertion place
            int32      left = 0, right = aet.cnt - 1, mid;
            while (left <= right) {
                  mid = left + (right - left) / 2;
                  if (aet.b[mid].x < x)
                        left = mid + 1;
                  else
                        right = mid - 1;
            }

            // move end of array to insert one element
            for (int32 c = aet.cnt; c > left; c--) {
                  aet.b[c] = aet.b[c - 1];
            }
            aet.cnt++;

            //return address to new element
            return &aet.b[left];
      }
      void store_vertex(int32 ys, int32 ye, float slope, float x) {

            //int32 cnt = et[ys].cnt;
            if (et[ys].cnt == maxv)
                  throw "poly_driver_class: store_vertex maxv overcount";
            et[ys].b[et[ys].cnt++] = { x,slope,ye };

      }
      // Функция для разбиения массива
      int32 partition(int32 low, int32 high) {
            double pivot = aet.b[high].x; // Опорный элемент
            int32 i = low - 1; // Индекс элемента меньше опорного

            for (int32 j = low; j < high; j++) {
                  if (aet.b[j].x < pivot) {
                        i++;
                        std::swap(aet.b[i], aet.b[j]);
                  }
            }

            std::swap(aet.b[i + 1], aet.b[high]);
            return i + 1;
      }
      // Функция быстрой сортировки
      void quickSort(int32 low, int32 high) {
            if (low < high) {
                  int32 pi = partition(low, high); // Индекс опорного элемента
                  quickSort(low, pi - 1); // Сортировка элементов до опорного
                  quickSort(pi + 1, high); // Сортировка элементов после опорного
            }
      }

      void update_slope() {
            for (int32 i = 0; i < aet.cnt; i++)
                  aet.b[i].x += aet.b[i].slope;
            // sort
            quickSort(0, aet.cnt - 1);
      }
      void update_aet(int32 scanline_no) {
            bucket* inserted;
            for (int32 vert_no = 0; vert_no < et[scanline_no].cnt; vert_no++)
            {
                  inserted = get_inserted(et[scanline_no].b[vert_no].x);
                  *inserted = et[scanline_no].b[vert_no];

            }
      }
      void cleanup_aet(int32 scanline_no) {
            int32 i = 0, j;
            while (i < aet.cnt)
            {
                  if (aet.b[i].ye == scanline_no)
                  {
                        for (j = i; j < aet.cnt - 1; j++)
                        {
                              aet.b[j] = aet.b[j + 1];

                        }
                        aet.cnt--;
                  }
                  else
                        i++;
            }

      }
      void add_segment(IntPoint* pt1, IntPoint* pt2) {
            int32 dx, dy;
            float slope;

            // if both points lies below or above viewable rect - edge skipped
            if ((pt1->y < 0 && pt2->y < 0) || (pt1->y >= h && pt2->y >= h))
                  return;
            dy = pt1->y - pt2->y;

            // horizontal lines are not stored in edge table
            if (dy == 0)
                  return;
            dx = pt1->x - pt2->x;

            if (dx == 0)
                  slope = 0.0;
            else
                  slope = (float)dx / (float)dy;

            //check if one point lies below view rect - recalculate intersection with zero scanline
            float x;
            int32 ys, ye;
            if (pt1->y < 0) {
                  x = (float)pt1->x - (float)pt1->y * slope;
                  ys = 0;
                  ye = pt2->y;
            }
            else if (pt2->y < 0) {
                  x = (float)pt2->x - (float)pt2->y * slope;
                  ys = 0;
                  ye = pt1->y;
            }
            else if (dy > 0) {
                  x = (float)pt2->x;
                  ys = pt2->y;
                  ye = pt1->y;
            }
            else {
                  x = (float)pt1->x;
                  ys = pt1->y;
                  ye = pt2->y;
            }
            store_vertex(ys, ye, slope, x);

      }
      friend class video_driver;
};
struct Poly {
private:
public:
      IntRect bounds;
      std::vector<FloatPoint> origin;
      std::vector<IntPoint> work;
      std::vector<int32> path_ptr;
      void transform_points(const PolarPoint* transform_value);
      //void transform_points(const PolarPoint& transform_value, FloatPoint& premod, IntPoint& postmod);

      //void transform_points_add(const PolarPoint& transform_value, const IntPoint& pt);
      //void transform_points_sub(const PolarPoint& transform_value, const IntPoint& pt);
      /*
      void copy_origin_to_work();
      void sub(const IntPoint& pt);
      void add(const IntPoint& pt);

      void transform_points(const PolarPoint& transform_value);
      */
      friend class video_driver;
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

            // each vessel has 5 points in 1 path
            shape.origin.resize(5);
            shape.work.resize(5);
            shape.path_ptr.resize(1);
            shape.path_ptr[0] = 5;

            /*points_count = 5;
            origin = new FloatPoint2[5];
            work = new IntPoint2[5];

            path_count = 1;
            pathindex = new int[2];
            pathindex[0] = 0;
            pathindex[1] = 5;*/
      }
      void size_changed() {
            double l = static_cast<double>(left),
                  r = static_cast<double>(right),
                  b = static_cast<double>(bottom),
                  t = static_cast<double>(top);
            shape.origin[0] = FloatPoint(t, (l + r) / 2 - r);// bow center
            shape.origin[1] = FloatPoint((t + b) * 0.75 - b, l);// bow left connector
            shape.origin[2] = FloatPoint(-b, l); // left bottom point
            shape.origin[3] = FloatPoint(-b, -r);// right bottom point
            shape.origin[4] = FloatPoint((t + b) * 0.75 - b, -r);// bow right connector


      }
};
enum  position_type_e
{
      gga, rmc, gns, gll
};
enum gnss_type
{
      gps, glonass, waas
};

struct own_vessel_class
{
private:
      FloatPoint position[4], meters[4];
      double dist1, dist2;
      int32 pos_index, _heading_set, _relative;
      double _heading;

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
      double get_dist1() { return dist1; }
      double get_dist2() { return dist2; }

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


                  //вынести эту хрень чтобы обновляла dist1 / dist2 когда у нас пишется очередные координаты гпс(~1гц)
                    //    а не как сейчас, каждое обновление экрана(~20гц)
                  if (!mysql->exec_prepared(PREPARED_DIST_STAT))
                  {
                        mysql->fetch();
                        dist1 = mysql->get_double(1);
                        mysql->fetch();
                        dist2 = mysql->get_double(1);
                  }
                  mysql->close_res();
            }
      };
      /////////////////////////////////////////////////////////////////////////
      double  heading() { return _heading; }
      void heading(double value)
      {
            _heading = value;
            _heading_set = 1;
      };
      /////////////////////////////////////////////////////////////////////////
      int32  relative() { return _relative; }
      void relative(int32 value)
      {
            _relative = value;
      }
      /////////////////////////////////////////////////////////////////////////
      own_vessel_class()
      {
            pos_index = -1;
            _heading_set = 0;
            _relative = 0;
            dist1 = NAN;
            dist2 = NAN;
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

// EXTERN //////////////////////////////////////////////////////////
extern poly_driver_class* polydriver;
extern std::map <int32, map_shape>  map_shapes;
extern own_vessel_class own_vessel;
extern std::map<int32, vessel> vessels;
extern satellites sat;

#endif