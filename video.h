#pragma once
#ifndef __VIDEO_H
#define __VIDEO_H
#include "mydefs.h"
#include "pixfont.h"
#include "ais_types.h"
#include <linux/fb.h>
#include <map>

const ARGB clSea = 0x0097c2;
const ARGB clLand = 0xb7b074;
const ARGB clBlack = 0x000000;
const ARGB clWhite = 0xFFFFFF;
const ARGB clNone = 0xFFFFFFFF;
const ARGB clLtGray = 0xC0C0C0;
const ARGB clGray = 0x808080;
const ARGB clRed = 0xFF0000;
const ARGB clTransparency25 = 0x40000000;
const ARGB clTransparency50 = 0x80000000;
const ARGB clTransparency75 = 0xC0000000;

extern IntRect VIEWBOX_RECT, SCREEN_RECT, SHIPLIST_RECT, WINDOW_RECT;
extern int32 CENTER_X, CENTER_Y;





#define FONT_OUTLINE 0
#define FONT_NORMAL 1
#define FONT_LARGE 2
#define max_vertices 500



//#define bpp 16#define bytes_per_pix bpp/8

//WARN_CONVERSION_OFF
inline uint16 rgb888to565(uint32 color)
{
      return  ((uint16)(color >> 8) & 0xF800) |
            ((uint16)(color >> 5) & 0x07E0) |
            ((uint16)(color >> 3) & 0x001F);
}
inline uint32 rgb565to888(uint16 c)
{
      return ((c & 0xF800) << 8) |
            ((c & 0x07E0) << 5) |
            ((c & 0x001F) << 3);

}
//WARN_RESTORE
//#define to16(x) ((x >> 8) & 0xF800) | ((x >> 5) & 0x07E0) | ((x >> 3) & 0x001F)





#define PIX_PTR puint16

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
      int32 last_error,
            fbdev, // fb handle
            current_fb, buffer_count,
            pixel_count, // = width * height
            screen_size, // = pixel_count * byte_per_pix, 
            _width, _height;
      std::map <int, font> fonts;
      fb_var_screeninfo vinfo;
      PIX_PTR pix_buf, fb_start;
      bucketset aet, * et;
      void edge_tables_reset();
      void edge_store_tuple_float(bucketset * b, int y_end, double  x_start, double  slope);
      void edge_store_tuple_int(bucketset * b, int y_end, double  x_start, double  slope);
      void edge_store_table(IntPoint pt1, IntPoint pt2);
      void edge_update_slope(bucketset * b);
      void edge_remove_byY(bucketset * b, int scanline_no);
      void calc_fill(const poly * sh);
      void draw_fill(const ARGB color);
      void draw_outline(const poly * sh, const ARGB color);
      void draw_line_fast(int y, int xs, int xe, const ARGB color);
      void draw_polyline(int x, int y, const ARGB color, int close_polyline = 0);
public:
      void flip();
      video_driver(const char * devname, int _buffer_count = 1);
      ~video_driver();
      int get_last_error() { return last_error; };
      int width() { return _width; };
      int height() { return _height; };
      bool load_font(const int index, const std::string filename);
      int32 get_font_height(const int32 font_index);
      int32 set_font_interval(const int32 font_index, const int32 font_interval );
      void fill_rect(int x0, int y0, int x1, int y1, const ARGB color);
      void fill_rect(IntRect rct, const ARGB color);
      void rectangle(IntRect rct, const ARGB color);
      void draw_line(int x1, int y1, int x2, int y2, const ARGB color);
      void draw_pix(const int x, const int y, const ARGB color);
      void draw_shape(const poly * sh, const ARGB fill_color, const ARGB outline_color);
      //void draw_text(int font_index, int x, int y, std::string s, int flags);
      void draw_text(int font_index, int x, int y, std::string s, uint32 flags, const ARGB black_swap, const ARGB white_swap, bool dbg = false);
      void draw_image(image * img, int x, int y, int flags, int transparency = 255); // 255 mean full visible, 0 mean none visible
      void circle(const IntCircle circle, const ARGB outline, const ARGB fill);
};





//int video_init(frame_func ff);
//void video_frame_start(const ARGB background);
void video_frame_end();
void video_loop_start();

#endif
