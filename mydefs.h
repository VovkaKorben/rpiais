#pragma once
#ifndef __MYDEFS_H
#define __MYDEFS_H
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <memory>
#include <cmath>
//#include <mysql.h>

#define PI 3.1415926535897932384626433832795
#define PI180 PI/180.0
#define RAD 180/PI
#define EARTH_RADIUS 6371008.8 // used by haversine
#define TO_RAD(x) (x)*PI180

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
typedef unsigned short * puint16;
typedef signed short int16;
typedef signed short * pint16;

typedef unsigned int ARGB;


#ifdef __clang__ //clang_compiler
#elif __GNUC__ //GNU_C_compiler

#define WARN_FLOATCONVERSION_OFF _Pragma("GCC diagnostic push");\
_Pragma("GCC diagnostic ignored \"-Wfloat-conversion\"");
#define WARN_CONVERSION_OFF _Pragma("GCC diagnostic push");\
_Pragma("GCC diagnostic ignored \"-Wconversion\"");
#define WARN_RESTORE _Pragma("GCC diagnostic pop")

#elif _MSC_VER // MSVC
#elif __BORLANDC__ //borland 
#elif __MINGW32__ // mingw 
#endif





    
//using namespace std;
inline int imin(int v1, int v2)
{
      if (v1 < v2) return v1; else return v2;
}
inline int imax(int v1, int v2)
{
      if (v1 > v2) return v1; else return v2;
}









//using namespace std;
typedef std::vector<std::string> StringArray;
typedef std::vector<StringArray> StringArrayBulk;

inline bool is_zero(double v1) { return std::abs(v1) < 0.000001; }
inline int isign(double v) { if (v > 0.0)            return 1;      else if (v < 0.0)            return -1;      else return 0; };
inline double dsign(double v) { if (v > 0.0)            return 1.0;      else if (v < 0.0)            return -1.0;      else return 0.0; };

inline std::string dec2bin(uint32 v, int len = 0)
{
      std::string r = "";
      int cnt = 0;
      while (v)
      {
            if (v & 1)
                  r = "1" + r;
            else
                  r = "0" + r;
            v >>= 1;
            cnt++;
      }
      if (len)
      {
            len -= cnt;
            while (len--)
                  r = "0" + r;
      }
      return r;
}

#ifdef __GNUC__ //GNU_C_compiler
inline uint64_t utc_ms() {
      struct timespec t;
      clock_gettime(CLOCK_REALTIME, &t);
      return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

#endif


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
            if (std::abs(y) >= 85.051129)
                  // The value 85.051129° is the latitude at which the full projected map becomes a square
                  y = dsign(y) * std::abs(y) * 111.132952777;
            else
                  y = log(tan(((90 + y) * PI) / 360)) / (PI / 180);
            y = (y * 20037508.34) / 180;
      }
      inline void offset_remove(floatpt fp)
      {
            x -= fp.x;
            y -= fp.y;
      }
      inline double haversine(floatpt fp)
      {
            double lat_delta = TO_RAD(fp.y - this->y),
                  lon_delta = TO_RAD(fp.x - this->x),
                  converted_lat1 = TO_RAD(this->y),
                  converted_lat2 = TO_RAD(fp.y);

            double a =
                  pow(sin(lat_delta / 2), 2) + cos(converted_lat1) * cos(converted_lat2) * pow(sin(lon_delta / 2), 2);

            double c = 2 * atan2(sqrt(a), sqrt(1 - a));
            double d = EARTH_RADIUS * c;

            return d;


      }
      inline intpt to_int()
      {
            return { (int)round(x),(int)round(y) };
      }
      inline int ix()
      {
            return int(round(x));
      }
      inline int iy()
      {
            return int(round(y));
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



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
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

#pragma GCC diagnostic pop

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
inline std::string read_file(std::string filename) {
      std::ifstream f(filename); //taking file as inputstream
      std::string str;
      if (f) {
            std::ostringstream ss;
            ss << f.rdbuf(); // reading data
            return ss.str();
      }
      else
      {
            //cout << "File not found:" << filename << endl;
            return NULL;
      }
}
inline std::vector<std::string> str_split(const char * str, char c = ',')
{
      std::vector<std::string> result;

      do
      {
            const char * begin = str;

            while (*str != c && *str)
                  str++;

            result.push_back(std::string(begin, str));
      } while (0 != *str++);

      return result;
}






#endif