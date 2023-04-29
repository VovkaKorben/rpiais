#pragma once
#ifndef __MYDEFS_H
#define __MYDEFS_H
#include <string>
#include <vector>
#include<cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <memory>
#include <mysql.h>

#ifdef __clang__ //clang_compiler
#elif __GNUC__ //GNU_C_compiler

#define PRAGMA(X) _Pragma(#X)
#define WARN_OFF(n)     PRAGMA(pragma GCC diagnostic push)\
PRAGMA(pragma GCC diagnostic ignored n)
#define WARN_RESTORE       PRAGMA(pragma GCC diagnostic pop)


#elif _MSC_VER // MSVC
#elif __BORLANDC__ //borland 
#elif __MINGW32__ // mingw 
#endif
/*
#define #pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"


// Macro to define cursor lines
#define CURSOR(top, bottom) (((top) << 8) | (bottom))

// Macro to get a random integer with a specified range
#define getrandom(min, max) \
    ((rand()%(int)(((max) + 1)-(min)))+ (min))

    */





    //#include <mysql.h>
      using namespace std;
//#include <mysql/jdbc.h>

//typedef void (*frame_func)();



const int VIEW_WIDTH = 480;
const int VIEW_HEIGHT = 320;


inline int imin(int v1, int v2)
{
      if (v1 < v2) return v1; else return v2;
}
inline int imax(int v1, int v2)
{
      if (v1 > v2) return v1; else return v2;
}





#define PI 3.1415926535897932384626433832795
#define PI180 1/PI*180.0
#define RAD 180/PI

#define NOSIZE_DIMENSION 10.0
typedef signed char int8;
typedef int8 * pint8;
typedef unsigned char uint8;
typedef uint8 * puint8;
typedef char * pchar;

typedef signed int int32;
typedef unsigned int uint32;
typedef unsigned int * puint32;
typedef signed int * pint32;

typedef unsigned short uint16;
typedef signed short int16;

typedef unsigned int BGRA;
//typedef unsigned long * argb_ptr;
using namespace std;
typedef vector<string> StringArray;
typedef vector<StringArray> StringArrayBulk;

inline bool is_zero(double v1) { return abs(v1) < 0.000001; }
inline int isign(double v) { if (v > 0.0)            return 1;      else if (v < 0.0)            return -1;      else return 0; };
inline double dsign(double v) { if (v > 0.0)            return 1.0;      else if (v < 0.0)            return -1.0;      else return 0.0; };

inline uint64_t utc_ms() {
      struct timespec t;
      clock_gettime(CLOCK_REALTIME, &t);
      return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
struct  intpt
{
      int x, y;
      void offset_add(intpt t)
      {
            x += t.x;
            y += t.y;
      }
      void offset_add(int _x, int _y)
      {
            x += _x;
            y += _y;
      }
      void offset_remove(intpt t)
      {
            x -= t.x;
            y -= t.y;
      }
};
struct  floatpt
{
      double x, y;
      inline  void to_polar()
      {
            double ta;
            if (x == 0)
            {
                  if (y < 0)
                        ta = 270.0;
                  else
                        ta = 90.0;
            }
            else
            {
                  ta = atan(y / x) / PI * 180.0;
                  if (x < 0)
                        ta += 180.0;
                  else if (y < 0)
                        ta += 360.0;

            }
            y = sqrt(x * x + y * y);
            x = ta;
      }
      inline void to_decart()
      {
            double ta = x * PI / 180;
            double td = y;
            x = td * cos(ta);
            y = td * sin(ta);
      }
      inline void latlon2meter() // in format(lon, lat)
      {
            x = (x * 20037508.34) / 180;
            if (abs(y) >= 85.051129)
                  // The value 85.051129° is the latitude at which the full projected map becomes a square
                  y = dsign(y) * abs(y) * 111.132952777;
            else
                  y = log(tan(((90 + y) * PI) / 360)) / (PI / 180);
            y = (y * 20037508.34) / 180;
      }
      inline void offset_remove(floatpt fp)
      {
            x -= fp.x;
            y -= fp.y;
      }
      inline intpt to_int()
      {
            return { (int)round(x),(int)round(y) };
      }

};

struct irect {
      int x1, y1, x2, y2;
      void assign_pos(int _x1, int _y1, int _x2, int _y2)
      {
            x1 = _x1;
            y1 = _y1;
            x2 = _x2;
            y2 = _y2;
      }
      void assign_pos(int _x, int _y)
      {
            x1 = _x;
            y1 = _y;
            x2 = _x;
            y2 = _y;
      }
      inline bool is_intersect(irect rct)
      {
            return ((x1 < rct.x2) && (rct.x1 < x2) && (y1 < rct.y2) && (rct.y1 < y2));
      }
      int width() { return x2 - x1; }
      int hcenter() { return x1 + (x2 - x1) / 2; }
      int vcenter() { return y1 + (y2 - y1) / 2; }

};
struct frect {
      double x1, y1, x2, y2;
      floatpt get_corner(int index)
      {
            switch (index) {
            case 0:	return floatpt{ x1, y1 };
            case 1:	return floatpt{ x2,y1 };
            case 2:	return floatpt{ x2,y2 };
            case 3:	return floatpt{ x1,y2 };
            }
      }
};

template<typename ... Args> inline std::string string_format(const std::string & format, Args ... args)
{
      int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
      if (size_s <= 0) { throw std::runtime_error("string_format: error during formatting."); }
      auto size = static_cast<size_t>(size_s);
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, format.c_str(), args ...);
      return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
inline string read_file(string filename) {
      ifstream f(filename); //taking file as inputstream
      string str;
      if (f) {
            ostringstream ss;
            ss << f.rdbuf(); // reading data
            return ss.str();
      }
      else
      {
            //cout << "File not found:" << filename << endl;
            return NULL;
      }
}
inline vector<string> str_split(const char * str, char c = ',')
{
      vector<string> result;

      do
      {
            const char * begin = str;

            while (*str != c && *str)
                  str++;

            result.push_back(string(begin, str));
      } while (0 != *str++);

      return result;
}
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
struct myshape :poly
{
      unsigned int id;


};


// FLAGS

const uint32 VESSEL_X = 0x01;
const uint32 VESSEL_Y = 0x02;
const uint32 VESSEL_L = 0x04;
const uint32 VESSEL_R = 0x08;
const uint32 VESSEL_T = 0x10;
const uint32 VESSEL_B = 0x20;
const uint32 VESSEL_A = 0x40;
const uint32 VESSEL_LRTB = VESSEL_L | VESSEL_R | VESSEL_T | VESSEL_B;
const uint32 VESSEL_LRTBA = VESSEL_L | VESSEL_R | VESSEL_T | VESSEL_B | VESSEL_A;
const uint32 VESSEL_XY = VESSEL_X | VESSEL_Y;
const uint32 VESSEL_XYLRTBA = VESSEL_L | VESSEL_R | VESSEL_T | VESSEL_B | VESSEL_X | VESSEL_Y | VESSEL_A;

struct vessel :poly
{
      uint32 mmsi, imo, accuracy, status, heading, shiptype, T, B, L, R;
      double lat, lon, turn, speed, course;
      string shipname, callsign;
      uint32 flags;
      //bool lat_ok, lon_ok, size_ok;
      vessel(uint32 _mmsi)
      {
            mmsi = _mmsi;
            flags = 0;
            shipname = "";
            last_access = utc_ms();

            points_count = 0;
            path_count = 1;
            pathindex = new int[2];
            pathindex[0] = 0;

            // init size to 3*3 meters
            set_points_count(4);
            origin[0] = { -NOSIZE_DIMENSION / 2,-NOSIZE_DIMENSION / 2 };
            origin[1] = { NOSIZE_DIMENSION / 2,-NOSIZE_DIMENSION / 2 };
            origin[2] = { NOSIZE_DIMENSION / 2, NOSIZE_DIMENSION / 2 };
            origin[3] = { -NOSIZE_DIMENSION / 2, NOSIZE_DIMENSION / 2 };
      }
      void size_changed() {
            if (((flags & VESSEL_LRTB) == VESSEL_LRTB) && T != 0 && B != 0 && L != 0 && R != 0)
            {
                  double l = L, r = R, t = T, b = B;
                  set_points_count(5);
                  origin[0] = { t,(l + r) / 2 - r }; // bow center
                  origin[1] = { (t + b) * 0.75 - b,l }; // bow left connector
                  origin[2] = { -b, l }; // left bottom point
                  origin[3] = { -b, -r }; // right bottom point
                  origin[4] = { (t + b) * 0.75 - b,-r };// bow right connector
            };
      }
      void set_points_count(int cnt)
      {
            if (points_count)
            {
                  delete[]origin;
                  delete[]work;
            };
            points_count = cnt;
            origin = new floatpt[cnt];
            work = new intpt[cnt];
            pathindex[1] = cnt;
      }
};




inline void logtofile(string s, bool c = false) {
      return;
      std::ofstream outfile;

      outfile.open("C:\\ais\\cpp\\AIS\\log.txt", std::ios_base::app); // append instead of overwrite
      outfile << s;
}





#endif