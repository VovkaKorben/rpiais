


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
#include  <algorithm>
#include  <cstring>
#include <sys/stat.h>

#define PI 3.141592653589793238462643383279
#define PI180 PI/180.0
#define RAD 180/PI
#define EARTH_RADIUS 6371008.8 // used by haversine
#define TO_RAD(x) (x)*PI180

typedef signed char int8;
typedef int8* pint8;
typedef unsigned char uint8;
typedef uint8* puint8;
typedef char* pchar;

typedef signed int int32;
typedef unsigned int uint32;
typedef unsigned int* puint32;
typedef signed int* pint32;

typedef unsigned short uint16;
typedef unsigned short* puint16;
typedef signed short int16;
typedef signed short* pint16;

typedef signed long long int64;
typedef unsigned long long uint64;

typedef unsigned int ARGB;


#ifdef __clang__ //clang_compiler
#elif __GNUC__ //GNU_C_compiler

#define WARN_FLOATCONVERSION_OFF _Pragma("GCC diagnostic push");\
_Pragma("GCC diagnostic ignored \"-Wfloat-conversion\"");
#define WARN_CONVERSION_OFF _Pragma("GCC diagnostic push");\
_Pragma("GCC diagnostic ignored \"-Wconversion\"");
#define WARN_RESTORE _Pragma("GCC diagnostic pop")

#elif _MSC_VER // MSVC
#define WARN_FLOATCONVERSION_OFF _Pragma("warning(disable : 4244)");
#define WARN_CONVERSION_OFF _Pragma("warning(disable : 4267)");
#define WARN_RESTORE _Pragma("warning(default  : 4244)");\
_Pragma("warning(default  : 4267)");
#elif __BORLANDC__ //borland 
#elif __MINGW32__ // mingw 
#endif



inline  std::string data_path(std::string rel_path)
{
      std::string root;
      root = "/home/pi/data";
      //#ifdef WIN
      //      root = "C:\\ais\\rpiais\\data";
      //      const char WINDOWS_FILE_PATH_SEPARATOR = '\\';
      //      const char UNIX_FILE_PATH_SEPARATOR = '/';
      //      std::replace(rel_path.begin(), rel_path.end(), UNIX_FILE_PATH_SEPARATOR, WINDOWS_FILE_PATH_SEPARATOR);
      //#endif
      return root + rel_path;

}

inline uint32 swap32(uint32 v)
{
      //  std::cout << std::hex << v << std::endl;

        //v=
      return     (v >> 24) |
            ((v >> 8) & 0x0000FF00) |
            ((v << 8) & 0x00FF0000) |
            (v << 24);
      //std::cout << std::hex << v << std::endl;
      //return v;
}

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
typedef std::vector<std::string>* PStringArray;
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


inline uint64 utc_ms() {
      struct timespec t;
      clock_gettime(CLOCK_REALTIME, &t);
      return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}


inline bool file_exists(const std::string& name) {
      struct stat buffer;
      return (stat(name.c_str(), &buffer) == 0);
}
//struct  FloatPoint;
struct  IntPoint
{
      int32 x, y;
      void offset_add(IntPoint t)
      {
            x += t.x;
            y += t.y;
      }
      void offset_add(int32 _x, int32 _y)
      {
            x += _x;
            y += _y;
      }
      void offset_remove(IntPoint t)
      {
            x -= t.x;
            y -= t.y;
      }
      IntPoint offset(int32 _x, int32 _y)
      {
            IntPoint r;
            r.x = x + _x;
            r.y = y + _y;
            return r;
      }
      bool is_equal(IntPoint t)
      {
            return (t.x == x && t.y == y);
      }

};
struct  FloatPoint
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
      inline void offset_remove(FloatPoint fp)
      {
            x -= fp.x;
            y -= fp.y;
      }
      int haversine(FloatPoint fp)
      {
            double lat_delta = TO_RAD(fp.y - this->y),
                  lon_delta = TO_RAD(fp.x - this->x),
                  converted_lat1 = TO_RAD(this->y),
                  converted_lat2 = TO_RAD(fp.y);

            double a =
                  pow(sin(lat_delta / 2), 2) + cos(converted_lat1) * cos(converted_lat2) * pow(sin(lon_delta / 2), 2);

            double c = 2 * atan2(sqrt(a), sqrt(1 - a));
            //double d = EARTH_RADIUS * c;

            return int(EARTH_RADIUS * c);


      }
      inline IntPoint to_int()
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
struct IntCircle
{
      int32 x, y, r;
};
struct IntRect {
private:
      int x1, y1, x2, y2;
public:

      int left() { return x1; }
      int right() { return x2; }
      int bottom() { return y1; }
      int top() { return y2; }

      void set_left(int v) { x1 = v; }
      void set_right(int v) { x2 = v; }
      void set_bottom(int v) { y1 = v; }
      void set_top(int v) { y2 = v; }
      IntPoint center()
      {
            return { x1 + (x2 - x1) / 2,y1 + (y2 - y1) / 2 };
            //            IntPoint r;
      }
      IntRect() {};
      IntRect(int l, int b, int r, int t)
            //void assign_pos(int _x1, int _y1, int _x2, int _y2)
      {
            x1 = l;
            y1 = b;
            x2 = r;
            y2 = t;
      }
      IntRect(int lr, int tb) {
            x1 = x2 = lr;
            y1 = y2 = tb;
      }
      inline bool is_intersect(IntRect rct)
      {
            return ((x1 < rct.x2) && (rct.x1 < x2) && (y1 < rct.y2) && (rct.y1 < y2));
      }
      int get_width() { return x2 - x1; }
      int get_height() { return y2 - y1; }
      int hcenter() { return x1 + (x2 - x1) / 2; }
      int vcenter() { return y1 + (y2 - y1) / 2; }
      void minmax_expand(int x, int y)
      {
            x1 = imin(x1, x);
            y1 = imin(y1, y);
            x2 = imax(x2, x);
            y2 = imax(y2, y);
      }
      void minmax_expand(IntPoint pt)
      {
            minmax_expand(pt.x, pt.y);
      }
      void collapse(int w, int h)
      {
            x1 += w; x2 -= w;
            y1 += h; y2 -= h;
      }
      /* void offset(int x, int y)
       {
       }
       void offset(IntPoint pt)
       {
             (pt.x, pt.y);
       }
       */
};
struct frect {
      double x1, y1, x2, y2;
      FloatPoint get_corner(int index)
      {
            switch (index) {

                  case 1:	return FloatPoint{ x2,y1 };
                  case 2:	return FloatPoint{ x2,y2 };
                  case 3:	return FloatPoint{ x1,y2 };
                  case 0:
                  default:
                        return FloatPoint{ x1, y1 };
            }
      }
};

template<typename ... Args> inline std::string string_format(const std::string& format, Args ... args)
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
inline std::vector<std::string> str_split(const char* str, char c = ',')
{
      std::vector<std::string> result;

      do
      {
            const char* begin = str;

            while (*str != c && *str)
                  str++;

            result.push_back(std::string(begin, str));
      } while (0 != *str++);

      return result;
}


inline unsigned long long tm_diff(timespec c, timespec p)
{
      unsigned long long s = c.tv_sec - p.tv_sec, n = c.tv_nsec - p.tv_nsec;
      return s * 1000000 + n / 1000;

}
inline int32 length_chararray(pchar str)
{
      int32 result = 0;
      while (str[result])
            result++;
      return result;

}
inline int32 length_constchararray(const char * str)
{
      int32 result = 0;
      while (str[result])
            result++;
      return result;

}
inline void lowercase_chararray(pchar v, int32 length)
{
      int32 pos = 0;
      while (pos < length)
      {
            if (!v[pos])
                  break;
            v[pos] = (char)tolower(v[pos]);
            pos++;
      }

}
inline int32 search_chararray(pchar hash, pchar needle, int32 end, int32 start = 0)
{
      int32 needle_len = length_chararray(needle);
      end -= needle_len;

      int32  found_count, c;
      while (start < end)
      {
            found_count = 0;
            for (c = 0; c < needle_len; c++)
                  if (hash[start + c] == needle[c])
                        found_count++;
                  else
                        break;
            if (found_count == needle_len)
                  return start;
            start++;
      }
      return -1;
}



#endif