#include <linux/input.h>
#include "touch.h"
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include <stdlib.h>    
#include <errno.h>
#include <mutex>
#include <map>
#include <set>
#include <SimpleIni.h>
#include<numeric>

std::map<int32, int32> evmap = { {ABS_X,0},{ABS_Y,1},{ABS_PRESSURE,2} };

size_t touchscreen::count()
{
      m.lock();
      size_t r = touches.size();
      m.unlock();
      return r;
}
bool touchscreen::pop(touches_coords_s& t)
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
      /*kminx = 254.9156485f,
            kminy = 176.2642045f,
            kx = 7.800794173f,
            ky = 11.72346591f;
            */

      std::vector<int32> collect[3];
      touches_coords_s touches_coords;
      // int32 last_values[3] = { 0 };      touches_c median_val = { 0,0,0 },            adjusted_val = { 0,0,0 };
       // std::vector <touches_c> median;

      const  ssize_t evsz = sizeof(struct input_event);
      struct input_event ev[64];
      ssize_t rb;
      int started = 0;
      while (true) {
            rb = read(di.fd, ev, evsz * 64);
            if (rb >= 0)
                  for (ssize_t i = 0; i < (rb / evsz); i++) {

                        switch (ev[i].type) {
                              case EV_SYN: {
                                    if (started) {
                                          //median.push_back({ x, y, p });
                                    }
                                    break;
                              }
                              case EV_KEY: {
                                    switch (ev[i].code) {
                                          case BTN_LEFT:
                                          case BTN_RIGHT:
                                          case BTN_TOUCH:
                                                if (ev[i].value == 1) { // pressed
                                                      for (int c = 0; c < max_axes; c++)
                                                            collect[c].clear();
                                                      //median.clear();
                                                      started = 1;
                                                }
                                                else {// released
                                                      float f;
                                                      for (int c = 0; c < max_axes; c++)
                                                      {
                                                            if (collect[c].size())
                                                                  touches_coords.raw[c] = std::accumulate(collect[c].begin(), collect[c].end(), 0) / (int32)collect[c].size();

                                                            // convert raw to adjusted
                                                            f = (float)touches_coords.raw[c];
                                                            //touches_coords.adjusted[c] = touches_coords.raw[c];
                                                            if (invert[c])
                                                                  f = (maxval[c] - minval[c]) - f;

                                                            f -= min_correction[c];
                                                            f *= zoom[c];
                                                            touches_coords.adjusted[c] = (int32)f;


                                                      }




                                                      m.lock();
                                                      //printf("Touch Finished\n");
                                                      touches.push_back(touches_coords);
                                                      m.unlock();
                                                      /*
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
                        adjusted_val.x = (int)(((float)median_val.x - kminx) / kx);
                        adjusted_val.y = (int)(((float)median_val.y - kminy) / ky);

                        m.lock();
                        //printf("Touch Finished\n");
                        touches.push_back({ median_val,adjusted_val });
                        m.unlock();
                  }
                                                      */
                                                }
                                                break;
                                    }
                                    break;
                              }
                              case EV_ABS: {
                                    if (evmap.count(ev[i].code)) // check if event allowed
                                    {
                                          int32 axis_index = evmap[ev[i].code];
                                          if (started)
                                                collect[axis_index].push_back(ev[i].value);
                                          touches_coords.raw[axis_index] = ev[i].value;

                                    }
                                    break;
                              }
                        }
                  }
            usleep(50000);
      }
}


int32 touchscreen::get_devinfo(int32 index, devinfo_s* devinfo)
{
      // return:
      // 0 ok
      // -1 dev not exists
      // -2 dev not support ABS
      // -3 error while get capabilities
      sprintf(devinfo->devpath, "/dev/input/event%d\0", index);
      devinfo->fd = open(devinfo->devpath, O_RDONLY);
      if (devinfo->fd < 0) {
            return -1;
      }
      // get device name
      ioctl(devinfo->fd, EVIOCGNAME(sizeof(devinfo->devname)), devinfo->devname);

      // get device capabilites
      memset(devinfo->bit, 0, sizeof(devinfo->bit));
      ioctl(devinfo->fd, EVIOCGBIT(0, EV_MAX), devinfo->bit[0]);
      if (!test_bit(EV_ABS, devinfo->bit[0]))
      {
            close(devinfo->fd);
            return -2;
      }

      // get capabilites of ABS-subsystem
      ioctl(devinfo->fd, EVIOCGBIT(EV_ABS, KEY_MAX), devinfo->bit[1]);
      //int32 bitlist[3] = { ABS_X,ABS_Y,ABS_PRESSURE };
      std::map<int, int>::iterator it = evmap.begin();
      for (int c = 0; c < max_axes; c++)
      {
            if (test_bit(it->first, devinfo->bit[1]))
            {
                  if (ioctl(devinfo->fd, EVIOCGABS(it->first), devinfo->abs[c]))
                  {
                        close(devinfo->fd);
                        return -3;
                  }
            }
            it++;
      }
      return 0;
}

touchscreen::touchscreen(CSimpleIniA* ini, int32 w, int32 h)
{
      int32 dev_search = 0;
      int32 devindex = (int32)ini->GetLongValue("touch", "device", -1);
      max_axes = (int32)ini->GetLongValue("touch", "axes", 2);
      if (devindex == -1) // autodetect
            dev_search = 1;
      else if (get_devinfo(devindex, &di) != 0) // dev not exists or not supported
            dev_search = 1;

      if (dev_search)
      { // specified device not exists or autodetect selected
            dev_search = 0;
            devindex = 0;
            while (true) {
                  int r = get_devinfo(devindex, &di);
                  if (r == -1)
                        break;
                  else if (r == 0)
                  {
                        dev_search = 1;
                        break;
                  }
                  devindex++;
            }
      }

      if (dev_search == 0)
      {
            printf("Cannot find device with ABS capabilities");
            return;
      }
      printf("Touch device: %s\n", di.devname);


      // check INI file for coordinates corrections
      int32 delta,
            bounds[3] = { w,h,100 };
      char key[256];
      for (int c = 0; c < max_axes; c++)
      {

            // float minval[3], zoom[3];            int invert[3];
            sprintf(key, "invert%d\0", c);
            invert[c] = (int32)ini->GetLongValue("touch", key, 0);

            sprintf(key, "minval%d\0", c);
            min_correction[c] = (float)ini->GetDoubleValue("touch", key, 0.0);
            minval[c] = (float)di.abs[c][1];
            maxval[c] = (float)di.abs[c][2];
            sprintf(key, "zoom%d\0", c);
            if (ini->KeyExists("touch", key))
                  zoom[c] = (float)ini->GetDoubleValue("touch", key, 1.0);
            else
            {
                  delta = di.abs[c][2] - di.abs[c][1];
                  if (delta == 0) {
                        printf("error while get device ABS bounds with index %d\n", c);
                        return;
                  }
                  zoom[c] = (float)bounds[c] / (float)delta;
                  //            kx = (float)w / (float)delta;
      //                        kp = 100.0f / (float)delta;
            }
      }
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
void touchscreen::simulate_click(int32 x, int32 y) {
      touches_coords_s tc = { { 0,0,0 },  { x,y,255 } };
      //touches_c median_val = ;
      m.lock();
      touches.push_back(tc);
      m.unlock();
}
/*
int touchscreen::get_details(int w, int h)
{
      unsigned long bit[2][NBITS(KEY_MAX)];
      int abs[6] = { 0 };

      // get device name
      ioctl(fd, EVIOCGNAME(sizeof(devname)), devname);
      printf("Touch device: %s\n", devname);

      // get device capabilites
      memset(bit, 0, sizeof(bit));
      ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
      if (!test_bit(EV_ABS, bit[0]))
      {
            printf("Touch device not support EV_ABS\n");
            return 1;
      }

      // get capabilites of ABS-subsystem
      ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), bit[1]);
      int d;

      if (test_bit(0, bit[1]))
      { // has X abs
            ioctl(fd, EVIOCGABS(0), abs);
            kminx = abs[1];
            d = abs[2] - kminx;
            if (d == 0) { printf("error while get device ABS X bounds\n"); return 1; }
            kx = (float)w / (float)d;
      }
      if (test_bit(1, bit[1]))
      { // has Y abs
            ioctl(fd, EVIOCGABS(1), abs);
            kminy = abs[1];
            d = abs[2] - kminy;
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
*/
/////////////////////////////////////

touch_manager::touch_manager() {
}
touch_manager::~touch_manager()
{

}
void touch_manager::sort_by_priority() {
      struct prio_key
      {
            inline bool operator() (const touch_group& struct1, const touch_group& struct2)
            {
                  return (struct1.priority > struct2.priority);
            }
      };
      std::sort(groups.begin(), groups.end(), prio_key());

}

touch_group* touch_manager::get_group(int group_id) {
      for (size_t n = 0; n < groups.size(); n++)
            //for (touch_group group : groups)
            if (groups[n].id == group_id)
                  //if (group.id == group_id)
                  return &groups[n];
      return nullptr;
}
int32 touch_manager::set_group_active(int32 group_id, int32 active) {
      touch_group* group = get_group(group_id);
      if (group == nullptr) return 1;
      group->active = active;
      return 0;
}
void touch_manager::touch_manager::set_groups_active(int32 active) {
      for (size_t n = 0; n < groups.size(); n++)
            groups[n].active = active;
}
int touch_manager::add_group(int32 group_id, int32 priority, int32 active) {
      if (get_group(group_id) != nullptr) return 1;
      groups.push_back({ group_id,priority,active });
      sort_by_priority();
      return 0;
}
int touch_manager::clear_group(int32 group_id) {
      touch_group* group = get_group(group_id);
      if (group == nullptr) return 1;
      group->areas.clear();
      return 0;
}
int32 touch_manager::add_rect(int32 group_id, int32 area_id, IntRect rct)
{
      touch_group* group = get_group(group_id);
      if (group == nullptr) return 1;
      touch_coords figure;
      figure.type = TOUCH_TYPE_RECT;
      figure.coords[0] = rct.left();
      figure.coords[1] = rct.bottom();
      figure.coords[2] = rct.right();
      figure.coords[3] = rct.top();
      figure.id = area_id;
      group->areas.push_back(figure);
      return 0;
}
int32 touch_manager::add_circle(int32 group_id, int32 area_id, IntCircle circle) {
      touch_group* group = get_group(group_id);
      if (group == nullptr) return 1;
      touch_coords figure;
      figure.type = TOUCH_TYPE_CIRCLE;
      figure.coords[0] = circle.x;
      figure.coords[1] = circle.y;
      figure.coords[2] = circle.r;
      figure.id = area_id;
      group->areas.push_back(figure);
      return 0;
}
int32 touch_manager::add_circle(int32 group_id, int32 area_id, IntPoint center, int32 radius)
{
      return add_circle(group_id, area_id, { center.x,center.y,radius });

}
void touch_manager::dump()
{
      printf("touch_manager::dump()\n-----------------------------------\n");
      for (const touch_group group : groups)
      {
            printf("Group #%d (prio: %d, active: %d, count: %d)\n",
                  group.id, group.priority, group.active, group.areas.size());
            for (touch_coords tmp : group.areas)
                  switch (tmp.type) {
                        case TOUCH_TYPE_RECT: {
                              printf("\t%d:rect %d,%d,%d,%d\n", tmp.id, tmp.coords[0], tmp.coords[1], tmp.coords[2], tmp.coords[3]); break;
                        }
                        case TOUCH_TYPE_CIRCLE: {
                              printf("\t%d:circle %d,%d,%d\n", tmp.id, tmp.coords[0], tmp.coords[1], tmp.coords[2]); break;
                        }
                        default: {
                              printf("\tUnknown type: %d\n", tmp.type); break;
                        }
                  }
      }
}
int touch_manager::check_point(const int32 x, const int32 y, int32& group_id, int32& area_id) {
      if (!enabled)
            return 0;
      for (const touch_group group : groups)
            if (group.active)
                  for (touch_coords tmp : group.areas) {
                        if (tmp.check_pt(x, y)) {
                              group_id = group.id;
                              area_id = tmp.id;
                              return 1;
                        }
                  }
      return 0;
}


void touch_manager::set_enabled(int32 mode)
{
      enabled = mode;
}
#ifndef NDEBUG
void touch_manager::debug(video_driver* scr) {

      ARGB clr;
      for (const touch_group group : groups)
      {
            clr = group.active ? 0xFF0000 : 0x00FF00;
            if (!enabled)
                  clr |= clTransparency75;

            for (touch_coords tmp : group.areas)
            {
                  switch (tmp.type) {
                        case TOUCH_TYPE_RECT: {
                              scr->rectangle({ tmp.coords[0], tmp.coords[1], tmp.coords[2], tmp.coords[3] }, clr);
                              break;
                        }
                        case TOUCH_TYPE_CIRCLE: {
                              scr->draw_circle({ tmp.coords[0], tmp.coords[1],tmp.coords[2] }, clr, clNone);
                              break;
                        }
                  }
            }
      }
}
#endif // NDEBUG

