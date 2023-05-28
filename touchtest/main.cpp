/*
~/projects/touch_tests/bin/ARM64/Debug/touch_tests.out
/home/pi/projects/touch_tests/bin/ARM64/Debug/touch_tests.out
/home/dbg/projects/touch_tests/bin/ARM64/Debug/touch_tests.out
/home/dbg/projects/touch_tests/bin/ARM64/Release/touch_tests.out

*/
#include "../touch.h"
#include <sys/types.h>
#include <sys/wait.h>

#include<unistd.h>
#include <mutex>
#include <csignal>

//#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <atomic>
touchscreen * touchscr;
touch_manager * touchman;

int running = 1;
void handle_int(int num) { running = 0; }



/*void * clock_thread(void * ignored_argument)
{
      time_t sec, prev;
      prev = time(0);
      while (true) {
            sec = time(0);
            if (prev != sec)
            {
                  prev = sec;
                  pthread_mutex_lock(&mtx);
                  changed++;
                  printf("\tUpdate clock\n");
                  pthread_mutex_unlock(&mtx);
            }
            else
                  usleep(500000);
      }
}*/

int main()
{
      touchman = new touch_manager();


      touchman->add_group(TOUCH_GROUP_ZOOM, 15);
      touchman->add_point(TOUCH_GROUP_ZOOM, "zoomout", { 29,297 , 23 });
      touchman->add_point(TOUCH_GROUP_ZOOM, "zoomin", { 291,297 , 23 });

      
      touchman->add_group(TOUCH_GROUP_SHIPSHAPE, 5);
      touchman->add_group(TOUCH_GROUP_SHIPLIST, 12);
      //touchman->add_rect(TOUCH_GROUP_SHIPLIST, "test", { 10,20,30,40 });

      touchman->add_group(TOUCH_GROUP_INFO, 10);
      touchman->add_rect(TOUCH_GROUP_INFO, "info", { 0,0,480,40 });
      
      touchman->add_group(TOUCH_GROUP_INFOWINDOW, 30,0);
      touchman->add_rect(TOUCH_GROUP_INFOWINDOW, "window", { 20,20,440,280 });
      touchman->dump();

      printf("---------------------------------\n");
      std::string name;
      int32 gi;
      if (touchman->check_point(130, 130, gi, name))
      {
            printf("point ok, group %d, name: %s\n",gi,name.c_str());
      }
      else
            printf("point not found\n");

      /*


      touchscr = new   touchscreen("/dev/input/event0", 480, 320);


      signal(SIGINT, handle_int);


      touches_s t;
      while (running)
      {
            while (touchscr->pop(t))
            {
                  printf("main pop %d %d\n", t.x, t.y);
            }
            //printf("main loop\n");
            usleep(50000);
      }*/
      printf("*** main exit ***\n");
}