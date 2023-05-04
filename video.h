#pragma once
#ifndef __VIDEO_H
#define __VIDEO_H
#include "mydefs.h"
#include "pixfont.h"
#include <linux/fb.h>
#include <map>
#define NORMAL_FONT 0
#define SPECCY_FONT 1

#define max_vertices 500
#define NO_COLOR  0xFFFFFFFF
struct bucket {
      int y, ix;
      double fx, slope;
};
struct bucketset {
      int cnt;
      bucket barr[max_vertices];
};
class video_driver
{
private:




      bucketset aet, *et;

      void edge_tables_reset();

      void edge_store_tuple_float(bucketset * b, int y_end, double  x_start, double  slope);
      void edge_store_tuple_int(bucketset * b, int y_end, double  x_start, double  slope);
      void edge_store_table(intpt pt1, intpt pt2);
      void edge_update_slope(bucketset * b);
      void edge_remove_byY(bucketset * b, int scanline_no);

      void calc_fill(const poly * sh);
      void draw_fill(const BGRA color);
      void draw_outline(const poly * sh, const BGRA color);

      puint32 fb_start, // fb start pointer
            fb_current; // fb active page pointer

      int last_error,
            fbdev, // fb handle
            current_fb, pixel_count, buffer_count,
            screen_size, // screen size = w*h*byte_per_pix, 
            fb_size,// fb_size = screen * buff_count
            width, height,
            bpp, bytes_per_pix;
      std::map <int, bitmap_font> fonts;
      fb_var_screeninfo vinfo;
      void draw_line_fast(int y, int xs, int xe, const BGRA color);
      void draw_polyline(int x, int y, const BGRA color, int close_polyline = 0);
public:


      void flip();

      video_driver(int _width, int _height, int _bpp = 32, int _buffer_count = 2);
      ~video_driver();
      int get_last_error() { return last_error; };
      int get_width() { return width; };
      int get_height() { return height; };

      bool load_font(const int index, const char * filename);
      



            void draw_line(int x1, int y1, int x2, int y2, const BGRA color);
      void draw_pix(const int x, const int y, const BGRA color);
      void draw_shape(const poly * sh, const BGRA fill_color, const BGRA outline_color);
      void draw_text(int font_index, int x, int y, string s, const BGRA color, int flags = VALIGN_BOTTOM | HALIGN_LEFT);
      void draw_circle(int cx, int cy, int radius, const BGRA color);
      void draw_bg(const BGRA background);
};

extern irect VIEWBOX_RECT, SCREEN_RECT;
extern int CENTER_X, CENTER_Y;




//int video_init(frame_func ff);
//void video_frame_start(const BGRA background);
void video_frame_end();
void video_loop_start();

#endif
