


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
#include <queue>
#include <chrono>
//#include "ais_types.h"

#define PI 3.141592653589793238462643383279

#define RAD0 0.0
#define RAD90 PI/2
#define RAD180 PI
#define RAD270 PI*1.5
#define RAD360 PI*2

//#define PIM2 PI*2#define PID2 PI/2
#define PI180 PI/180.0
#define RAD 180/PI
#define EARTH_RADIUS 6371008.8 // used by haversine
#define DEG2RAD(x) (static_cast<double>(x)*PI/180.0)
#define RAD2DEG(x) (static_cast<double>(x)*180.0/PI)

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

#define nline void pstr(std::string)
#define PRINT_STRING(message) printf("%s",message.c_str())



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
      return     (v >> 24) |
            ((v >> 8) & 0x0000FF00) |
            ((v << 8) & 0x00FF0000) |
            (v << 24);
}
inline int imin(int v1, int v2)
{
      if (v1 < v2) return v1; else return v2;
}
inline int imax(int v1, int v2)
{
      if (v1 > v2) return v1; else return v2;
}









//using namespace std;
typedef std::queue<std::string> StringQueue;
typedef std::vector<std::string> StringArray;
typedef std::vector<std::string>* PStringArray;
typedef std::vector<StringArray> StringArrayBulk;
typedef std::vector<int32> IntVec;
typedef std::vector<int32>::iterator IntVecIter;

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
/*
inline pchar bigint2str(uint64_t v)
{
      char buff[50];
      double ld = std::log10(v);
      int32 li = (int32)std::ceil(ld);
      int32 li = (int32)ld;



}*/



inline bool file_exists(const std::string& name) {
      struct stat buffer;
      return (stat(name.c_str(), &buffer) == 0);
}
template<typename ... Args> inline std::string string_format(const std::string& format, Args ... args)
{
      int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
      if (size_s <= 0) { throw std::runtime_error("string_format: error during formatting.\n"); }
      auto size = static_cast<size_t>(size_s);
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, format.c_str(), args ...);
      return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
template<typename ... Args> inline std::string string_format2(const std::string& format, Args ... args)
{
      size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
      if (size <= 0) {
            throw std::runtime_error("string_format: error during formatting.\n");
      }
      char buff[size];
      std::snprintf(buff, size, format.c_str(), args ...);
      std::string r = std::string(buff, size - 1);
      return r;
      //   return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
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
inline int32 length_constchararray(const char* str)
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
inline int32 search_chararray(pchar hash, pchar needle, int32 end, int32 start = 0) {
      int32 needle_len = length_chararray(needle);
      end -= needle_len;

      int32  found_count, c;
      while (start <= end)
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
inline void copy_chararray(pchar src, pchar dst, int32  start, int32 cnt) {
      pchar t_src = src, t_dst = dst;

      t_src += start;
      while (cnt--)
      {
            *t_dst = *t_src;
            t_dst++;
            t_src++;
      }
      *t_dst = 0;
}


inline std::string ReplaceString(std::string subject, const std::string& search,
      const std::string& replace) {
      size_t pos = 0;
      while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
      }
      return subject;
}
inline uint64_t utc_ms() {
      /*struct timespec t;
      clock_gettime(CLOCK_REALTIME, &t);
      return t.tv_sec * 1000 + t.tv_nsec / 1000000;
      */
      using namespace std::chrono;
      uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      /* auto sec_since_epoch = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

       std::cout << "seconds since epoch: " << sec_since_epoch << std::endl;
       std::cout << "milliseconds since epoch: " << millisec_since_epoch << std::endl;
       uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
       */
      return ms;
      //std::cout << ms << " milliseconds since the Epoch\n";
}
inline std::string time_diff(uint64_t sec)
{
      return string_format("%d:%0.2d:%0.2d", sec / 3600, (sec % 3600) / 60, sec % 60);

      /*if (sec < 60)
            return string_format("%ds", sec);
      else
      {
            sec /= 60;
            if (sec < 60)
                  return string_format("%dm", sec);
      }*/
}

inline std::string deg2dms(double degrees) {
      int32 deg = int32(degrees); // deg = 120
      double remainder = degrees - deg; // remainder = 0.123456
      double minutes_float = remainder * 60; // minutes_float = 7.40736
      int32 minutes = int32(minutes_float); // minutes = 7
      double remainder_minutes = minutes_float - minutes; // remainder_minutes = 0.40736
      double seconds_float = remainder_minutes * 60; // seconds_float = 24.4416
      int32 seconds = int32(seconds_float); // seconds = 24
      int32 two_digit_remainder = int32((seconds_float - seconds) * 100); // two_digit_remainder = 44
      return string_format("%+d\x86%.2d\"%.2d.%.2d\'", deg, minutes, seconds, two_digit_remainder);
}
inline std::string format_thousand(double v) {

      // float part
      std::string r = string_format(".%02d", ((int32)(v * 100)) % 100);

      int32 i = (int32)v;
      if (i >= 1000)
            r = string_format("%2d\x87%3d", i / 1000, i % 1000) + r;
      else
            r = string_format("  \x87%3d", i % 1000) + r;
      return r;
}


inline int32 write_log(std::string msg)
{
      static std::string filename = string_format("/home/pi/logs/%ld.log", utc_ms());
      std::ofstream outFile(filename, std::ios::app);
      if (!outFile.is_open()) {
            std::cerr << "Error opening file " << filename << std::endl;
            std::cerr << strerror(errno) << std::endl; // Print system error message
            perror("Error");
            return 1;
      }

      // Write some content to the file
      outFile << msg << std::endl;

      // Close the file
      outFile.close();
      return 0;
}
/*
inline int32 calcSolarTime(int32 c){
      return 0;
}
*/
/*
inline int32 calcSolarTime(FloatPoint coords, int32 dayOfYear) {

      return 0;
}
*/

#endif