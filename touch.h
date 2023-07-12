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
#include <SimpleIni.h>
#include <linux/input.h>

#ifndef NDEBUG 
#include "video.h" // 
#endif

#define TOUCH_GROUP_ZOOM 0
#define TOUCH_GROUP_SHIPSHAPE 1
#define TOUCH_GROUP_SHIPLIST 2
#define TOUCH_GROUP_INFOLINE 3
#define TOUCH_GROUP_INFOWINDOW 4

#define TOUCH_TYPE_RECT 0
#define TOUCH_TYPE_CIRCLE 1

#define TOUCH_AREA_ZOOMIN 1
#define TOUCH_AREA_ZOOMOUT 0

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)
#define testbit(bit, var)	((var[LONG(bit)] >> OFF(bit)) & 1)



struct touches_coords_s
{
      int32 raw[3];
      int32 adjusted[3];
};
struct devinfo_s
{
      int32 fd;
      char devpath[256], devname[256];
      unsigned long bit[2][NBITS(KEY_MAX)];
      int abs[3][6] = { 0 };

};
class touchscreen
{
private:
      devinfo_s di;
      int32 get_devinfo(int32 index, devinfo_s* devinfo);
      std::mutex m;
      std::thread* t;
      void t_func();
      std::vector<touches_coords_s> touches;
      float min_correction[3], minval[3], maxval[3], zoom[3];
      int invert[3];
      int32 max_axes;
     
public:
      size_t count();
      bool pop(touches_coords_s& t);
      touchscreen(CSimpleIniA* ini, int32 w, int32 h);
      ~touchscreen();
      void simulate_click(int32 x, int32 y);
    
};

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
struct touch_coords
{
      int32 type, id;
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
struct touch_group {
      int32 id, priority, active;
      std::vector <touch_coords> areas;
};
//typedef int32 touch_group_id;
//typedef std::pair<touch_group_id, touch_group_params> touch_group_info;
class touch_manager
{
private:
      std::vector <touch_group> groups;
      //std::unordered_map<int, std::vector <touch_coords>> areas;
      touch_group* get_group(int32 group_index);
      void sort_by_priority();
      int32 enabled;
public:
      touch_manager();
      ~touch_manager();
      int32 set_group_active(int32 group_id, int32 active);
      void set_groups_active(int32 active);
      int32 add_group(int32 group_id, int32 priority, int32 active = 1);
      int32 clear_group(int32 group_id);
      int32 add_rect(int32 group_id, int32 area_id, IntRect rct);
      int32 add_circle(int32 group_id, int32 area_id, IntCircle circle);
      int32 add_circle(int32 group_id, int32 area_id, IntPoint center, uint32 radius);
      void dump();
      int32 check_point(const int32 x, const int32 y, int32& group_id, int32& area_id);
      
      void set_enabled(int32 mode);
      //int check_point(const int32 x, const int32 y, int32 & group_index, int32 & area_index);
#ifndef NDEBUG
      void debug(video_driver* screen);
#endif // !1




};

#endif