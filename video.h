#pragma once
#ifndef __VIDEO_H
#define __VIDEO_H
#include "mydefs.h"
#include "pixfont.h"
#include "ais_types.h"
#include <map>

const ARGB clSea = 0x0097c2;
const ARGB clLand = 0xb7b074;
const ARGB clBlack = 0x000000;
const ARGB clWhite = 0xFFFFFF;
const ARGB clNone = 0xFFFFFFFF;
extern irect VIEWBOX_RECT, SCREEN_RECT, INFO_RECT, WINDOW_RECT;
extern int CENTER_X, CENTER_Y;


#ifdef LINUX
#include <linux/fb.h>
#endif
#ifdef WIN
#include "PixelToaster.h"
#endif

#define FONT_OUTLINE 0
#define FONT_NORMAL 1



#define max_vertices 500



#define bpp 16
#define bytes_per_pix bpp/8

WARN_CONVERSION_OFF
inline uint16 rgb888to565(uint32 color)
{
      return  ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
}
WARN_RESTORE
//#define to16(x) ((x >> 8) & 0xF800) | ((x >> 5) & 0x07E0) | ((x >> 3) & 0x001F)




#if bpp==16
#define PIX_PTR puint16
#else
#define PIX_PTR puint32
#endif

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

#ifdef WIN
      PixelToaster::Display * pixtoast;
      std::vector<PixelToaster::TrueColorPixel> pixels;

#endif


      bucketset aet, * et;

      void edge_tables_reset();

      void edge_store_tuple_float(bucketset * b, int y_end, double  x_start, double  slope);
      void edge_store_tuple_int(bucketset * b, int y_end, double  x_start, double  slope);
      void edge_store_table(intpt pt1, intpt pt2);
      void edge_update_slope(bucketset * b);
      void edge_remove_byY(bucketset * b, int scanline_no);

      void calc_fill(const poly * sh);
      void draw_fill(const ARGB color);
      void draw_outline(const poly * sh, const ARGB color);



      PIX_PTR fb_start, // fb start pointer
            fb_current; // fb active page pointer

      int last_error,
            fbdev, // fb handle
            current_fb, buffer_count, w, h,
            pixel_count, // = width * height
            screen_size, // = pixel_count * byte_per_pix, 
            fb_size; // = screen_size * buff_count


      std::map <int, font> fonts;
#ifdef LINUX
      fb_var_screeninfo vinfo;
#endif // LINUX


      void draw_line_fast(int y, int xs, int xe, const ARGB color);
      void draw_polyline(int x, int y, const ARGB color, int close_polyline = 0);
public:
      void upd_pixtoast();

      void flip();

      video_driver(int _width, int _height, int _buffer_count = 1);
      ~video_driver();
      int get_last_error() { return last_error; };
      int get_width() { return w; };
      int get_height() { return h; };

      bool load_font(const int index, const std::string filename);



      void fill_rect(int x0, int y0, int x1, int y1, const ARGB color);
      void fill_rect(irect rct, const ARGB color);
      void rectangle(irect rct, const ARGB color);
      void draw_line(int x1, int y1, int x2, int y2, const ARGB color);
      void draw_pix(const int x, const int y, const ARGB color);
      void draw_shape(const poly * sh, const ARGB fill_color, const ARGB outline_color);
      //void draw_text(int font_index, int x, int y, std::string s, int flags);
      void draw_text(int font_index, int x, int y, std::string s, uint32 flags, const ARGB black_swap, const ARGB white_swap) ;
      void draw_image(image * img, int x, int y, int flags, int transparency = 255); // 255 mean full visible, 0 mean none visible
      void circle(const int cx, const int cy, const int radius, const ARGB outline, const ARGB fill);
      //void draw_bg(const ARGB color);
};





//int video_init(frame_func ff);
//void video_frame_start(const ARGB background);
void video_frame_end();
void video_loop_start();

#endif
