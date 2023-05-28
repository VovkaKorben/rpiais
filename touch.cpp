#include <linux/input.h>
#include "touch.h"
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include <stdlib.h>     /* abs */
#include <errno.h>
//#include <pthread.h>
#include <mutex>
#include <map>
#include <set>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)
#define testbit(bit, var)	((var[LONG(bit)] >> OFF(bit)) & 1)
size_t touchscreen::count()
{
      m.lock();
      size_t r = touches.size();
      m.unlock();
      return r;
}
bool touchscreen::pop(touches_coords & t)
{
      bool r;
      m.lock();
      if (touches.size())
      {
            r = true;
            t = touches[0];
            touches.erase(touches.begin());
      }
      else
            r = false;
      m.unlock();
      return r;
}
void touchscreen::t_func()
{
      // this is calibrate coefficients from my ADS7846 Touchscreen
      const float kminx = 254.9156485,
            kminy = 176.2642045,
            kx = 7.800794173,
            ky = 11.72346591;



      std::vector <touches_c> median;
      int x = 0, y = 0, p = 0;
      const  ssize_t evsz = sizeof(struct input_event);
      struct input_event ev[64];
      ssize_t rb;
      int started = 0;
      while (true)
      {
            // printf("t_func\n");
            rb = read(fd, ev, evsz * 64);
            //printf("t_func readed: %d\n", rb);
            if (rb >= 0)
                  for (ssize_t i = 0; i < (rb / evsz); i++) {
                        if (ev[i].type == EV_SYN)
                        { // event sync
                              if (started)
                              {
                                    median.push_back({ x, y, p });
                                    //printf("push_back\n");
                              }
                        }
                        else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
                        { // touch start
                              median.clear();
                              //printf("started\n");
                              started = 1;
                        }

                        else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
                        {// touch end
                              //printf("touch end\n");
                              started = 0;
                              int32 sample_cnt = (int32)median.size();
                              if (sample_cnt)
                              {
                                    touches_c median_val = { 0,0,0 },
                                          adjusted_val = { 0,0,0 };
                                    for (int32 n = 0; n < sample_cnt; n++)
                                    {
                                          median_val.x += median[n].x;
                                          median_val.y += median[n].y;
                                          median_val.p += median[n].p;
                                    }
                                    median_val.x /= sample_cnt;
                                    median_val.y /= sample_cnt;
                                    median_val.p /= sample_cnt;
                                    adjusted_val.x = (median_val.x - kminx) / kx;
                                    adjusted_val.y = (median_val.y - kminy) / ky;

                                    m.lock();
                                    //printf("Touch Finished\n");
                                    touches.push_back({ median_val,adjusted_val });
                                    m.unlock();
                              }


                        }
                        else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0) {
                              //printf("\tY: %d\n", ev[i].value);
                              //*y = (int)((float)ev[i].value * ky);
                             //y = (int)((float)ev[i].value * ky);
                              y = ev[i].value;
                        }
                        else if (ev[i].type == EV_ABS && ev[i].code == 1 && ev[i].value > 0) {
                              //printf("\tX: %d\n", ev[i].value);
                              //*x = (int)((float)ev[i].value * kx);
                              //*x = ev[i].value * kx;
                              //x = (int)((float)ev[i].value * kx);
                              x = ev[i].value;
                        }
                        else if (ev[i].type == EV_ABS && ev[i].code == 24 && ev[i].value > 0) {
                              //printf("\tP: %d\n", ev[i].value);
                              //p = (int)((float)ev[i].value * kp);
                              p = ev[i].value;
                        }

                  }



            usleep(50000);
      }
}
touchscreen::touchscreen(const char * devname, int32 w, int32 h)
{
      fd = open(devname, O_RDONLY);
      if (fd < 0) { printf("Error opening device: %s\n", devname);            return; }
      if (get_details(w, h)) { printf("Error get details for %s\n", devname);            return; }

      t = new std::thread([this]() {t_func(); });
}
touchscreen::~touchscreen()
{
      if (t != nullptr)
      {
            t->join();
            delete t;
      }
}
int touchscreen::get_details(int w, int h)
{
      //uint32 types[EV_MAX];
      unsigned long bit[2][NBITS(KEY_MAX)];
      int abs[6] = { 0 };
      //int coords[4];

      // get device name
      ioctl(fd, EVIOCGNAME(sizeof(devname)), devname);
      printf("Device: %s\n", devname);

      // get device capabilites
      memset(bit, 0, sizeof(bit));
      ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
      if (!test_bit(EV_ABS, bit[0]))
      {
            printf("device not support EV_ABS\n");
            return 1;
      }

      // get capabilites of ABS-subsystem
      ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), bit[1]);
      int d;

      if (test_bit(0, bit[1]))
      { // has X abs
            ioctl(fd, EVIOCGABS(0), abs);
            d = abs[2] - abs[1];
            if (d == 0) { printf("error while get device ABS X bounds\n"); return 1; }
            kx = (float)w / (float)d;
      }
      if (test_bit(1, bit[1]))
      { // has Y abs
            ioctl(fd, EVIOCGABS(1), abs);
            d = abs[2] - abs[1];
            if (d == 0) { printf("error while get device ABS Y bounds\n"); return 1; }
            ky = (float)h / (float)d;
      }
      if (test_bit(24, bit[1]))
      { // has Y abs
            ioctl(fd, EVIOCGABS(24), abs);
            d = abs[2] - abs[1];
            if (d == 0) { printf("error while get device ABS P bounds\n"); return 1; }
            kp = 100.0f / (float)d;
      }
      return 0;
}

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

typedef std::vector<int>::iterator IntIter;
typedef std::pair<int, std::vector <touch_coords>> ddd;

touch_manager::touch_manager() {
}
touch_manager::~touch_manager()
{

}
void touch_manager::sort_by_priority() {
      struct prio_key
      {
            inline bool operator() (const touch_group_info & struct1, const touch_group_info & struct2)
            {
                  return (struct1.second.priority > struct2.second.priority);
            }
      };
      std::sort(groups.begin(), groups.end(), prio_key());

}

int touch_manager::check_exists(int group_index) {
      for (touch_group_info tgi : groups)
            if (tgi.first == group_index)
                  return 1;
      return 0;
}
int touch_manager::set_group_active(int32 group_index, int32 active) {
      if (!check_exists(group_index)) return 1;
      groups[group_index].second.active = active;
      return 0;
}
int touch_manager::add_group(int32 group_index, int32 priority, int32 active) {
      if (check_exists(group_index)) return 1;
      groups.push_back({ group_index,{priority,active} });
      areas[group_index] = {};
      sort_by_priority();
      return 0;
}
int touch_manager::clear_group(int32 group_index) {
      if (!check_exists(group_index)) return 1;
      areas[group_index].clear();
      return 0;
}
int touch_manager::add_rect(int32 group_index, std::string shapename, IntRect rct)
{
      if (!check_exists(group_index)) return 1;
      touch_coords figure;
      figure.type = TOUCH_TYPE_RECT;
      figure.coords[0] = rct.left();
      figure.coords[1] = rct.bottom();
      figure.coords[2] = rct.right();
      figure.coords[3] = rct.top();
      figure.name = shapename;
      areas[group_index].push_back(figure);
      return 0;
}
int touch_manager::add_point(int32 group_index, std::string shapename, IntCircle circle) {
      if (!check_exists(group_index)) return 1;
      touch_coords figure;
      figure.type = TOUCH_TYPE_CIRCLE;
      figure.coords[0] = circle.x;
      figure.coords[1] = circle.y;
      figure.coords[2] = circle.r;
      figure.name = shapename;
      areas[group_index].push_back(figure);
      return 0;
}
void touch_manager::dump()
{
      printf("touch_manager::dump()\n-----------------------------------\n");
      for (touch_group_info tgi : groups)
      {
            printf("Group #%d (prio: %d, active: %d, count: %d)\n",
                   tgi.first, tgi.second.priority, tgi.second.active, areas[tgi.first].size());
            for (touch_coords tmp : areas[tgi.first])
                  switch (tmp.type) {
                        case TOUCH_TYPE_RECT: {
                              printf("\t%s:rect %d,%d,%d,%d\n", tmp.name.c_str(), tmp.coords[0], tmp.coords[1], tmp.coords[2], tmp.coords[3]); break;
                        }
                        case TOUCH_TYPE_CIRCLE: {
                              printf("\t%s:circle %d,%d,%d\n", tmp.name.c_str(), tmp.coords[0], tmp.coords[1], tmp.coords[2]); break;
                        }
                        default: {
                              printf("\tUnknown type: %d\n", tmp.type); break;
                        }
                  }
      }
}
int touch_manager::check_point(const int32 x, const int32 y, int32 & group_index, std::string & shapename) {
      for (touch_group_info tgi : groups) // iterate groups
            if (tgi.second.active)
                  for (touch_coords tmp : areas[tgi.first]) {
                        if (tmp.check_pt(x, y)) {
                              group_index = tgi.first;
                              shapename = tmp.name;
                              return 1;
                        }
                  }
      return 0;
}

#ifndef NDEBUG
void touch_manager::debug(video_driver * scr) {

      // printf("touch_manager::dump()\n-----------------------------------\n");
      ARGB clr;
      for (touch_group_info tgi : groups)
      {
            clr = tgi.second.active ? 0xFF0000 : 0xA0FF0000;
            //printf("Group #%d (prio: %d, active: %d, count: %d)\n",                   tgi.first, tgi.second.priority, tgi.second.active, areas[tgi.first].size());
            for (touch_coords tmp : areas[tgi.first])
            {
                  switch (tmp.type) {
                        case TOUCH_TYPE_RECT: {
                              scr->rectangle({ tmp.coords[0], tmp.coords[1], tmp.coords[2], tmp.coords[3] }, clr);
                              break;
                              //                          printf("\t%s:rect %d,%d,%d,%d\n", tmp.name.c_str(), tmp.coords[0], tmp.coords[1], tmp.coords[2], tmp.coords[3]); break;
                        }
                        case TOUCH_TYPE_CIRCLE: {
                              scr->circle({ tmp.coords[0], tmp.coords[1], tmp.coords[2] }, clr, clNone);
                              break;
                              //                           printf("\t%s:circle %d,%d,%d\n", tmp.name.c_str(), tmp.coords[0], tmp.coords[1], tmp.coords[2]); break;
                        }
                        default: {
                              //printf("\tUnknown type: %d\n", tmp.type); break;
                              break;
                        }
                  }
                  //                  scr->flip();
            }
      }

}
#endif // !1

