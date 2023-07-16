#pragma once
#ifndef __VIDEO_H
#define __VIDEO_H
#include "mydefs.h"
#include "pixfont.h"
#include "ais_types.h"
#include <linux/fb.h>
#include <map>
#include <vector>

const ARGB clSea = 0x0097c2;
const ARGB clLand = 0xb7b074;
const ARGB clBlack = 0x000000;
const ARGB clWhite = 0xFFFFFF;
const ARGB clNone = 0xFFFFFFFF;
const ARGB clLtGray = 0xC0C0C0;
const ARGB clDkGray = 0x404040;
const ARGB clGray = 0x808080;
const ARGB clRed = 0xFF0000;
const ARGB clDkRed = 0x880000;

const ARGB clGreen = 0x00FF00;
const ARGB clYellow = 0xFFFF00;
const ARGB clBlue = 0x0000FF;

const ARGB clTransparency12 = 0x20000000;
const ARGB clTransparency25 = 0x40000000;
const ARGB clTransparency50 = 0x80000000;
const ARGB clTransparency75 = 0xC0000000;

extern IntRect VIEWBOX_RECT, SCREEN_RECT, SHIPLIST_RECT, WINDOW_RECT;
extern IntPoint VIEWBOX_CENTER;
extern int32 CENTER_X, CENTER_Y;


#define FONT_OUTLINE 0
#define FONT_NORMAL 1
//#define FONT_LARGE 2
#define FONT_MONOMEDIUM 3



#ifdef BPP16
#define bpp 16
#define PXPTR puint16
#endif 
#ifdef BPP32
#define bpp 32
#define PXPTR puint32
#endif 



//WARN_CONVERSION_OFF
#if bpp==16
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
#endif 
//WARN_RESTORE




class video_driver
{
private:
      //std::map <uint32, std::vector<uint32>> circle_cache;

      int32 last_error,
            fbdev, // fb handle
            current_fb, buffer_count,
            // pixel_count, // = width * height
            screen_size, // = pixel_count * byte_per_pix, 
            _width, _height;

      std::map <int, font> fonts;
      fb_var_screeninfo vinfo;
      PXPTR   pix_buf, fb_start;
      //      voidPIX_PTR pix_buf, fb_start;

      //void draw_outline(const poly * sh, const ARGB color);

      void draw_line_fast(int y, int xs, int xe, const ARGB color);
public:
      void draw_line_v2(const IntPoint pt1, const IntPoint pt2, const ARGB color);
      void draw_outline_v2(const Poly* sh, const ARGB color);
      void flip();
      video_driver(const char* devname, int _buffer_count = 1);
      ~video_driver();
      int get_last_error() { return last_error; };
      int width() { return _width; };
      int height() { return _height; };
      bool load_font(const int index, const std::string filename);
      int32 get_font_height(const int32 font_index);
      int32 set_font_interval(const int32 font_index, const int32 font_interval);
      void fill_rect(int32 x0, int32 y0, int32 x1, int32 y1, const ARGB color);
      void fill_rect(IntRect rct, const ARGB color);
      void rectangle(IntRect rct, const ARGB color);
      //void draw_line(int x1, int y1, int x2, int y2, const ARGB color);
      void draw_pix(const int32 x, const int32 y, const ARGB color);
      void draw_shape(const Poly* sh, const ARGB fill_color, const ARGB outline_color);
      //void draw_text(int font_index, int x, int y, std::string s, int flags);
      void draw_text(const int32 font_index, int32 x, int32 y, const std::string s, const uint32 flags, const ARGB black_swap, const ARGB white_swap);
      void draw_text(const int32 font_index, IntPoint pt, const std::string s, const uint32 flags, const ARGB black_swap, const ARGB white_swap);
      void draw_image(image* img, int32 x, int32 y, int32 flags, int32 transparency = 255); // 255 mean full visible, 0 mean none visible
      //void _make_circle_cache(const uint32 radius);
      void draw_circle(const IntCircle circle, const ARGB outline, const ARGB fill);
      //void draw_circle(const IntPoint center, const  int32 radius, const ARGB outline, const ARGB fill);





      //int video_init(frame_func ff);
      //void video_frame_start(const ARGB background);
      void video_frame_end();
      void video_loop_start();
};

video_driver* screen;
#endif
