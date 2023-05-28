#include <cstdio>
#include <unistd.h>
#include "../touch.h"
#include "../video.h"
video_driver * screen;
touchscreen * touchscr;


void draw_targ(video_driver * scr, int x, int y)
{
      const int lsz = 20;
      const int csz = 12;
      scr->draw_line(x - lsz, y, x + lsz, y, clBlack);
      scr->draw_line(x, y - lsz, x, y + lsz, clBlack);
      scr->circle({ x,y,csz }, clRed, clNone);
}

int main()
{


      const int cross_margin = 50;


      // output
      screen = new video_driver("/dev/fb1", 0); // debug purpose = 1 buffer, production value = 5
      if (screen->get_last_error())
      {
            printf("video_init error: %d.\n", screen->get_last_error());
            return 1;
      }
      int l = 0, r = screen->width(), b = 0, t = screen->height();
      int minx = 1000, miny = 1000, maxx = 1000, maxy = 1000;
      screen->load_font(FONT_NORMAL, data_path("/img/normal.png"));

      //  int corr[2][4] = { {1,1} }
      touchscr = new   touchscreen("/dev/input/event1", screen->width(), screen->height());
      int y, x = 200, lh = 10, cnt = 0;
      touches_coords tc;
      while (true) {

            while (touchscr->pop(tc))
            {
                  //printf("%d,%d\n", tc.raw.x, tc.raw.y);
                  minx = imin(tc.raw.x, minx);
                  miny = imin(tc.raw.y, miny);
                  maxx = imax(tc.raw.x, maxx);
                  maxy = imax(tc.raw.y, maxy);
            }

            screen->fill_rect(l, b, r, t, clLtGray);

            draw_targ(screen, l + cross_margin, b + cross_margin);
            draw_targ(screen, l + cross_margin, t - cross_margin);
            draw_targ(screen, r - cross_margin, t - cross_margin);
            draw_targ(screen, r - cross_margin, b + cross_margin);

            y = 200;
            screen->draw_text(FONT_NORMAL, x, y, string_format("minx: %d", minx), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;
            screen->draw_text(FONT_NORMAL, x, y, string_format("miny: %d", miny), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;
            screen->draw_text(FONT_NORMAL, x, y, string_format("maxx: %d", maxx), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;
            screen->draw_text(FONT_NORMAL, x, y, string_format("maxy: %d", maxy), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;

            screen->draw_text(FONT_NORMAL, x, y, string_format("last raw: %d, %d", tc.raw.x, tc.raw.y), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;
            screen->draw_text(FONT_NORMAL, x, y, string_format("last adj: %d, %d", tc.adjusted.x, tc.adjusted.y), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;


            screen->draw_text(FONT_NORMAL, x, y, string_format("CNT: %d", cnt++), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= lh;

            screen->flip();
            usleep(100000);
      }



      //touchscr = new   touchscreen("/dev/input/event0", 480, 320);

      printf("hello from %s!\n", "touchcal");
      return 0;
}
