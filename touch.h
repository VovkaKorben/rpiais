#pragma once
#ifndef __TOUCH_H
#define __TOUCH_H

//#include <pthread.h>
#include <vector>
#include "mydefs.h"
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <utility>

#ifndef NDEBUG 
#include "video.h" // 
#endif

#define TOUCH_GROUP_ZOOM 0
#define TOUCH_GROUP_SHIPSHAPE 1
#define TOUCH_GROUP_SHIPLIST 2
#define TOUCH_GROUP_INFO 3
#define TOUCH_GROUP_INFOWINDOW 4

#define TOUCH_TYPE_RECT 0
#define TOUCH_TYPE_CIRCLE 1

struct touches_c
{
      int32 x, y, p;
};
struct touches_coords
{
      touches_c raw, adjusted;
};
class touchscreen
{
private:
      int fd;

      //pthread_t thid;      pthread_mutex_t mtx;
      std::mutex m;
      std::thread * t;
      void t_func();


      std::vector<touches_coords> touches;

      //IntRect phys;
      //int screen_w, screen_h;
      float kx, ky, kp;
      int get_details(int w, int h);
      char devname[256];
      static void * read_func(void * arg);
      // long getTouchSample(int * x, int * y, int * p);
public:
      size_t count();
      bool pop(touches_coords & t);
      // const char * get_name() { return devname; }
      touchscreen(const char * devname, int32 w, int32 h);
      ~touchscreen();
};

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
struct touch_coords
{
      int32 type;
      std::string name;
      int32 coords[4];
      int check_pt(int x, int y)
      {
            switch (type)
            {
                  case TOUCH_TYPE_RECT: {
                        if (x < coords[0] ||
                            y < coords[1] ||
                            x > coords[2] ||
                            y > coords[3]) return 0;
                        return 1;
                  }
                  case TOUCH_TYPE_CIRCLE: {
                        x -= coords[0];
                        y -= coords[1];
                        int d = (int)sqrt(x * x + y * y);
                        return d <= coords[2];
                  }
                  default: return 0;
            }
      }
};
struct touch_group_params {
      int32 priority, active;
};
typedef int32 touch_group_id;
typedef std::pair<touch_group_id, touch_group_params> touch_group_info;
class touch_manager
{
private:
      std::vector <touch_group_info> groups;
      std::unordered_map<int, std::vector <touch_coords>> areas;
      int check_exists(int group_index);
      void sort_by_priority();
public:
      touch_manager();
      ~touch_manager();
      int set_group_active(int32 group_index, int32 active);
      int add_group(int32 group_index, int32 priority, int32 active = 1);
      int clear_group(int32 group_index);
      int add_rect(int32 group_index, std::string shapename, IntRect rct);
      int add_point(int32 group_index, std::string shapename, IntCircle circle);
      void dump();
      int check_point(const int32 x, const int32 y, int32 & group_index, std::string & shapename);
      //int check_point(const int32 x, const int32 y, int32 & group_index, int32 & area_index);
#ifndef NDEBUG
      void debug(video_driver * screen);
#endif // !1




};

#endif