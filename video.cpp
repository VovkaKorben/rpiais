#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wconversion"


#include <algorithm>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "video.h"
#include "pixfont.h"
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include  <linux/kd.h>
//#include  <ais_types.h>


#define FB_NO_ERROR 0;
#define FB_OPEN_FAILED 1;
#define FB_GET_VSCREENINFO_FAILED 2;
#define FB_GET_FSCREENINFO_FAILED 3;
#define FB_MMAP_FAILED 4;
#define FB_PUT_VSCREENINFO_FAILED 5;
#define FB_PAN_DISPLAY_FAILED 6;
#define FB_INVALID_BUFFER_ADDRESS 7;
#define FB_BUFFER_COUNT_ERROR 8;
#define FB_SET_KDSETMODE_FAILED 9;




//int32 CENTER_X, CENTER_Y;
IntPoint CENTER;
IntRect VIEWBOX_RECT, SCREEN_RECT, SHIPLIST_RECT, WINDOW_RECT;
double overlap_coeff;
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
void video_driver::flip()
{

      if (buffer_count)
      {
            vinfo.yoffset = vinfo.yres * current_fb;
            if (ioctl(fbdev, FBIOPAN_DISPLAY, &vinfo)) { last_error = FB_PAN_DISPLAY_FAILED; return; }
            current_fb = (current_fb + 1) % buffer_count;
            //pix_buf = fb_start + pixel_count * current_fb;
      }
      else
      {
            memcpy(fb_start, pix_buf, screen_size);
      }
      last_error = FB_NO_ERROR;
}

// _buffer_count = 0, used internal offscreen buffer
// _buffer_count = 1, uses current screen as is
// _buffer_count = >1, tries to change framebuffer



video_driver::video_driver(const char* devname, int _buffer_count) {
#ifndef bpp
      printf("No bit per pixel defined, aborting\n");
      abort();
#endif 

      struct fb_fix_screeninfo finfo;

      // create file descriptor
      fbdev = open(devname, O_RDWR);
      if (fbdev == -1)
      {
            perror("open");
            last_error = FB_OPEN_FAILED;
            return;
      }

      // get initial virtual info
      if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
            perror("ioctl");
            last_error = FB_GET_VSCREENINFO_FAILED;
            return;
      }

      int32 real_buffers = _buffer_count;
      if (!real_buffers)
            real_buffers = 1;

      // try setup virtual bounds to visible dimensions
      if (_buffer_count > 1) {
            uint32 desired_width = vinfo.xres;
            uint32 desired_height = vinfo.yres * real_buffers;

            vinfo.xres_virtual = desired_width;
            vinfo.yres_virtual = desired_height;
            vinfo.bits_per_pixel = bpp;
            if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo)) {
                  printf("video_driver init: Error setting variable information.\n");
            }
            // get updated virtual info
            if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
                  perror("ioctl");
                  last_error = FB_GET_VSCREENINFO_FAILED;
                  return;
            }
      }
#if bpp==16
      if (vinfo.bits_per_pixel != 16) {
            printf("Bit per pixel mismatch, expect 16, got %d\n", vinfo.bits_per_pixel);
            abort();
      }
#endif 
#if bpp==32
      if (vinfo.bits_per_pixel != 32) {
            printf("Bit per pixel mismatch, expect 32, got %d\n", vinfo.bits_per_pixel);
            abort();
      }
#endif 


      // setup screen numbers
      buffer_count = _buffer_count;
      _width = vinfo.xres, _height = vinfo.yres;
      screen_size = vinfo.xres_virtual * vinfo.yres_virtual * vinfo.bits_per_pixel / 8;

      /*  if (_buffer_count > 0)
        {
              vinfo.xres_virtual = vinfo.xres;
              vinfo.yres_virtual = vinfo.yres * _buffer_count;
              if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo)) {
                    printf("video_driver init: Error setting variable information.\n");
              }
              if (vinfo.yres != vinfo.yres_virtual)
                    _buffer_count = 1;
              buffer_count = _buffer_count;
  }
        else {
              _buffer_count = 1;
              buffer_count = 0;
        }
        */

        // check FINFO is ok
      if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
            last_error = FB_GET_FSCREENINFO_FAILED;
            return;
      }
      if ((intptr_t)finfo.smem_start == 0)
      {
            printf("FBIOGET_FSCREENINFO returns invalid buffer address.\n");
            //last_error = FB_INVALID_BUFFER_ADDRESS;
      }


      // map fb memory
      puint8 tmp_mmap, tmp_offscreen;
      tmp_mmap = (puint8)mmap(NULL, screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fbdev, 0);
      if ((intptr_t)tmp_mmap == -1) {
            last_error = FB_MMAP_FAILED;
            return;
      }

      if (buffer_count) {
            tmp_offscreen = tmp_mmap;
            current_fb = 0;
      }
      else {
            tmp_offscreen = (puint8)malloc(screen_size);
      }

      fb_start = (PXPTR)tmp_mmap;
      pix_buf = (PXPTR)tmp_offscreen;

      // creating rects


      if (ioctl(fbdev, KDSETMODE, KD_GRAPHICS)) {
            perror("ioctl");
            last_error = FB_SET_KDSETMODE_FAILED;
      }
      polydriver = new poly_driver_class(_height, 10000);
      //et = new bucketset[_height];
      last_error = FB_NO_ERROR;
}
video_driver::~video_driver() {
      //delete[]et;
      if (!buffer_count)
            free(pix_buf);
      munmap(fb_start, screen_size * buffer_count);
      if (ioctl(fbdev, KDSETMODE, KD_TEXT)) {
            last_error = FB_SET_KDSETMODE_FAILED;
            return;
      }
      close(fbdev);

}

////////////////////////////////////////////////////////////
void video_driver::draw_pix(const int32 x, const int32 y, const ARGB color) {
      if (x < 0) return;
      if (y < 0) return;
      if (x >= _width) return;
      if (y >= _height) return;
      //puint8 addr = (puint8)(pix_ptr - y * SCANLINE_LEN + x * BYTES_PER_PIXEL);
      int a = _height - y - 1;
      a = a * vinfo.xres_virtual;
      a = a + x;

      PXPTR addr = pix_buf + (_height - y - 1) * vinfo.xres_virtual + x;

      int alpha = color >> 24;


      if (alpha == 0)
      {
            // no transparency at all
#if bpp==16  
            * addr = rgb888to565(color);
#endif
#if bpp==32
            * addr = color;
#endif
      }
      else if (alpha != 255) // has semi-transparency
      {
            float a = (float)alpha / 255.0f, ia = 1.0f - a;

#if bpp==16  
            uint16 c = *addr;

            uint32 r = ((color & 0x00FF0000) >> 8) * ia + (c & 0xF800) * a;
            uint32 g = ((color & 0x0000FF00) >> 5) * ia + (c & 0x07E0) * a;
            uint32 b = ((color & 0x000000FF) >> 3) * ia + (c & 0x001F) * a;

            *addr = (r & 0xF800) | (g & 0x07E0) | (b & 0x001F);
#endif
#if bpp==32
            uint32 bgcolor = *addr;

            uint8 r = ((color & 0x00FF0000) >> 16) * ia + ((bgcolor & 0x00FF0000) >> 16) * a;
            uint8 g = ((color & 0x0000FF00) >> 8) * ia + ((bgcolor & 0x0000FF00) >> 8) * a;
            uint8 b = (color & 0x000000FF) * ia + (bgcolor & 0x000000FF) * a;

            *addr = (r << 16) | (g << 8) | b;

#endif



      }

}

#if bpp==16  
#endif
#if bpp==32
#endif

/*void video_driver::_make_circle_cache(const uint32 radius)
{
      std::vector<uint32> tmp(radius);

}*/

////////////////////////////////////////////////////////////


typedef int OutCode;

const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000
OutCode ComputeOutCode(int32 x, int32 y)
{
      OutCode code = INSIDE;  // initialised as being inside of clip window

      if (x < SCREEN_RECT.left())           // to the left of clip window
            code |= LEFT;
      else if (x > SCREEN_RECT.right())      // to the right of clip window
            code |= RIGHT;
      if (y < SCREEN_RECT.bottom())           // below the clip window
            code |= BOTTOM;
      else if (y > SCREEN_RECT.top())      // above the clip window
            code |= TOP;

      return code;
}
bool CohenSutherlandLineClip(int32 x0, int32 y0, int32 x1, int32 y1)
{
      OutCode outcode0 = ComputeOutCode(x0, y0);
      OutCode outcode1 = ComputeOutCode(x1, y1);
      bool accept = false;

      while (true) {
            if (!(outcode0 | outcode1)) {
                  accept = true;
                  break;
            }
            else if (outcode0 & outcode1) {
                  break;
            }
            else {
                  int32 x, y;

                  // At least one endpoint is outside the clip rectangle; pick it.
                  OutCode outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;
                  if (outcodeOut & TOP) {           // point is above the clip window
                        x = x0 + (x1 - x0) * (SCREEN_RECT.top() - y0) / (y1 - y0);
                        y = SCREEN_RECT.top();
                  }
                  else if (outcodeOut & BOTTOM) { // point is below the clip window
                        x = x0 + (x1 - x0) * (SCREEN_RECT.bottom() - y0) / (y1 - y0);
                        y = SCREEN_RECT.bottom();
                  }
                  else if (outcodeOut & RIGHT) {  // point is to the right of clip window
                        y = y0 + (y1 - y0) * (SCREEN_RECT.right() - x0) / (x1 - x0);
                        x = SCREEN_RECT.right();
                  }
                  else if (outcodeOut & LEFT) {   // point is to the left of clip window
                        y = y0 + (y1 - y0) * (SCREEN_RECT.left() - x0) / (x1 - x0);
                        x = SCREEN_RECT.left();
                  }
                  if (outcodeOut == outcode0) {
                        x0 = x;
                        y0 = y;
                        outcode0 = ComputeOutCode(x0, y0);
                  }
                  else {
                        x1 = x;
                        y1 = y;
                        outcode1 = ComputeOutCode(x1, y1);
                  }
            }
      }
      return accept;
}
////////////////////////////////////////////////////////////
void video_driver::draw_line(IntPoint* pt1, IntPoint* pt2, const ARGB color) {

      int32 x1 = pt1->x, y1 = pt1->y;
      int32 x2 = pt2->x, y2 = pt2->y;
      if (!CohenSutherlandLineClip(x1, y1, x2, y2))
            return;
      int32 w = x2 - x1;
      int32 h = y2 - y1;
      int32 dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
      if (w < 0) dx1 = -1; else if (w > 0) dx1 = 1;
      if (h < 0) dy1 = -1; else if (h > 0) dy1 = 1;
      if (w < 0) dx2 = -1; else if (w > 0) dx2 = 1;
      int32 longest = abs(w);
      int32 shortest = abs(h);
      if (!(longest > shortest)) {
            longest = abs(h);
            shortest = abs(w);
            if (h < 0) dy2 = -1; else if (h > 0) dy2 = 1;
            dx2 = 0;
      }
      int32 numerator = longest >> 1;
      for (int32 i = 0; i <= longest; i++) {
            draw_pix(x1, y1, color);
            numerator += shortest;
            if (!(numerator < longest)) { numerator -= longest;                  x1 += dx1;                  y1 += dy1; }
            else { x1 += dx2;                  y1 += dy2; }
      }
}
/*
void video_driver::draw_outline_v2(const Poly* sh, const ARGB color)
{
      IntPoint* prev_point, * point;
      int path_end, point_id = 0;
      for (int path_id = 0; path_id < sh->path_count; path_id++)
      {
            path_end = sh->pathindex[path_id + 1];
            prev_point = &sh->work[path_end - 1];

            while (point_id < path_end)
            {

                  point = &sh->work[point_id];
                  draw_line_v2(*prev_point, *point, color);
                  prev_point = point;
                  point_id++;
            }
      }
}*/
////////////////////////////////////////////////////////////
bool video_driver::load_font(const int index, const std::string filename)
{
      if (fonts.count(index) != 0)
            return false;
      fonts.emplace(std::piecewise_construct,
            std::forward_as_tuple(index),
            std::forward_as_tuple(filename));

      return true;
}
int32 video_driver::get_font_height(const int font_index)
{
      if (fonts.count(font_index) == 0)
            return -1;
      return fonts[font_index].height();
}
int32 video_driver::set_font_interval(const int32 font_index, const int32 font_interval) {
      if (fonts.count(font_index) == 0)
            return 1;
      fonts[font_index].set_interval(font_interval);
      return 0;
}
void video_driver::draw_text(const int32 font_index, IntPoint pt, const std::string s, const uint32 flags, const ARGB black_swap, const ARGB white_swap) {
      draw_text(font_index, pt.x, pt.y, s, flags, black_swap, white_swap);
}
void video_driver::draw_text(const int32 font_index, int32 x, int32 y, const std::string s, const uint32 flags, const ARGB black_swap, const ARGB white_swap)
//void video_driver::draw_text(int font_index, int x, int y, std::string s, const ARGB color, int flags)
{
      if (fonts.count(font_index) == 0)
            return;

      // detect text dimensions
      int overall_width = 0, overall_count = 0;
      char_info_s* ch_info;
      /*
      std::string s = "\x81";
      int32 xxx = s.at(0);

      for (int32 c = 1; c < s.size(); c++)
      {
            int32 xxx = s.at(c);
            printf("[%d] %d\n", c, xxx);
      }
      */


      for (const uint8& c : s) {
            ch_info = fonts[font_index].get_char_info(c);
            if (ch_info)
            {
                  overall_width += ch_info->width();
                  overall_count++;
            }
      }
      overall_width += (overall_count - 1) * fonts[font_index].get_interval();

      // recalc initial point
      if ((flags & HALIGN_CENTER) == HALIGN_CENTER) x -= overall_width / 2;
      else if ((flags & HALIGN_RIGHT) == HALIGN_RIGHT) x -= overall_width;

      if ((flags & VALIGN_CENTER) == VALIGN_CENTER)
            y += fonts[font_index].height() / 2 + fonts[font_index].height_start_delta();
      else   if ((flags & VALIGN_BOTTOM) == VALIGN_BOTTOM)
            y += fonts[font_index].height() + fonts[font_index].height_start_delta();// +fonts[font_index].offset();

      // precalc alpha for replaces
      uint32 black_swap_alpha = (black_swap >> 24) ^ 0xFF,
            white_swap_alpha = (white_swap >> 24) ^ 0xFF;


      int32 scanline_current, y_cnt, x_cnt, x_pos;
      puint32 data;
      uint32 font_pix, clr, tmp_alpha;
      //ARGB temp_color, paste_color = color & 0xFFFFFF;
      for (const uint8& c : s) {

            if (x >= _width) return; // go away if next char outside screen

            ch_info = fonts[font_index].get_char_info(c);
            if (ch_info)
            {
                  x -= ch_info->width_start - ch_info->real_width_start;

                  data = ch_info->data;
                  scanline_current = y;
                  y_cnt = fonts[font_index].real_height();
                  while (y_cnt--)
                  {
                        x_cnt = ch_info->real_width();
                        x_pos = x;
                        while (x_cnt--)
                        {
                              // check template color
                              font_pix = *data;
                              clr = font_pix & 0xFFFFFF;

                              if ((clr == 0x000000) && (black_swap != clNone))
                              {
                                    /*
                                    uint32 aswap = (black_swap >> 24) ^ 0xFF;
                                    uint32 afont = (*data >> 24) ^ 0xFF;
                                    uint32 ca = aswap * afont;
                                    ca = ca ^ 0x0000FF00;
                                    ca = ca << 16;
                                    ca = ca & 0xFF000000;
                                    font_pix = ca | (black_swap & 0xFFFFFF);
                                    */
                                    tmp_alpha = ((font_pix >> 24) ^ 0xFF) * black_swap_alpha;
                                    font_pix = (((tmp_alpha ^ 0x0000FF00) << 16) & 0xFF000000) | (black_swap & 0xFFFFFF);

                              }
                              else if ((clr == 0xFFFFFF) && (white_swap != clNone))
                              {

                                    /*  float aswap = ((white_swap >> 24) ^ 0xFF) / 255.0;
                                      float afont = (*data >> 24) / 255.0;
                                      float aresult = aswap * afont;
                                      uint32 ares = aresult * 255;
                                      ares = ares << 24;
                                      font_pix = ares | (white_swap & 0xFFFFFF);*/
                                    tmp_alpha = ((font_pix >> 24) ^ 0xFF) * white_swap_alpha;
                                    font_pix = (((tmp_alpha ^ 0x0000FF00) << 16) & 0xFF000000) | (white_swap & 0xFFFFFF);
                              }



                              draw_pix(x_pos, scanline_current, font_pix);
                              data++;
                              x_pos++;
                        }
                        scanline_current--;
                  }
                  x += ch_info->width_start - ch_info->real_width_start + ch_info->width() + fonts[font_index].get_interval();
            }
      }

}

//void video_driver::draw_circle(IntPoint center, int32 radius, const ARGB outline, const ARGB fill) {
//      draw_circle({ center.x,center.y,radius }, outline, fill);
//}
void video_driver::draw_circle(const IntCircle circle, const ARGB outline, const ARGB fill) {

      if (fill != clNone) {
            int f = 1 - circle.r();
            int ddF_x = 0;
            int ddF_y = -2 * circle.r();
            int x = 0;
            int y = circle.r();

            draw_line_fast(circle.y(), circle.x() - circle.r(), circle.x() + circle.r(), fill);
            draw_pix(circle.x(), circle.y() + circle.r(), fill);
            draw_pix(circle.x(), circle.y() - circle.r(), fill);



            while (x < y)
            {
                  if (f >= 0)
                  {
                        y--;
                        ddF_y += 2;
                        f += ddF_y;
                  }
                  x++;
                  ddF_x += 2;
                  f += ddF_x + 1;

                  draw_line_fast(circle.y() + y, circle.x() - x, circle.x() + x, fill);
                  draw_line_fast(circle.y() - y, circle.x() - x, circle.x() + x, fill);

                  draw_line_fast(circle.y() - x, circle.x() - y, circle.x() + y, fill);
                  draw_line_fast(circle.y() + x, circle.x() - y, circle.x() + y, fill);


            }
      }
      if (outline != clNone) {

            int f = 1 - circle.r();
            int ddF_x = 0;
            int ddF_y = -2 * circle.r();
            int x = 0;
            int y = circle.r();



            draw_pix(circle.x() - x, circle.y() + circle.r(), outline);
            draw_pix(circle.x() - x, circle.y() - circle.r(), outline);
            draw_pix(circle.x() - x + circle.r(), circle.y(), outline);
            draw_pix(circle.x() - x - circle.r(), circle.y(), outline);

            while (x < y)
            {
                  if (f >= 0)
                  {
                        y--;
                        ddF_y += 2;
                        f += ddF_y;
                  }
                  x++;
                  ddF_x += 2;
                  f += ddF_x + 1;


                  draw_pix(circle.x() + x, circle.y() + y, outline);
                  draw_pix(circle.x() - x, circle.y() + y, outline);
                  draw_pix(circle.x() + x, circle.y() - y, outline);
                  draw_pix(circle.x() - x, circle.y() - y, outline);
                  draw_pix(circle.x() + y, circle.y() + x, outline);
                  draw_pix(circle.x() - y, circle.y() + x, outline);
                  draw_pix(circle.x() + y, circle.y() - x, outline);
                  draw_pix(circle.x() - y, circle.y() - x, outline);
            }
      }

}
void video_driver::rectangle(IntRect rct, const ARGB color) {
      int i;
      for (i = rct.left(); i <= rct.right(); i++)
      {
            draw_pix(i, rct.bottom(), color);
            draw_pix(i, rct.top(), color);
      }
      for (i = rct.bottom() + 1; i < rct.top(); i++)
      {
            draw_pix(rct.left(), i, color);
            draw_pix(rct.right(), i, color);
      }
}
void video_driver::fill_rect(int32 x0, int32 y0, int32 x1, int32 y1, const ARGB color) {
      for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
                  draw_pix(x, y, color);
}
void video_driver::fill_rect(IntRect rct, const ARGB color) {
      fill_rect(rct.left(), rct.bottom(), rct.right(), rct.top(), color);
}
void video_driver::draw_line_fast(int y, int xs, int xe, const ARGB color)
{ //  ONLY HORIZONTAL LINES, WITHOUT ALPHA
      // check line (or part) lies in window
      //printf("draw_line_fast s: %d, e: %d, y: %d\n", xs, xe, y);
      if (xe < 0 || xs >= _width) return;
      if (y < 0 || y >= _height) return;

      // clip start & end to window bounds
      if (xs < 0) xs = 0;
      if (xe >= _width) xe = _width - 1;

      xe -= xs;
      PXPTR addr = pix_buf + (_height - y - 1) * vinfo.xres_virtual + xs;
#if bpp==16
      uint16 c = rgb888to565(color);
      while (xe--)
            *(addr++) = c;
#endif
#if bpp==32
      while (xe--)
            *(addr++) = color;
#endif
}
void video_driver::draw_image(image* img, int32 x, int32 y, int32 flags, int32 transparency)
{
      if (!img->is_loaded()) return;
      if (transparency == 0) return; // nothing draw with zero transparency
      if ((flags & HALIGN_CENTER) == HALIGN_CENTER)            x -= img->width() / 2;
      else if ((flags & HALIGN_RIGHT) == HALIGN_RIGHT)            x -= img->width();

      if ((flags & VALIGN_CENTER) == VALIGN_CENTER)            y += img->height() / 2;
      else   if ((flags & VALIGN_BOTTOM) == VALIGN_BOTTOM)            y += img->height();
      //      WARN_FLOATCONVERSION_OFF
                  //WARN_CONVERSION_OFF
                  // transparency = 255- transparency ;
      puint8 src = img->data_ptr();
      PXPTR dst_start = pix_buf + (_height - y - 1) * vinfo.xres_virtual + x,
            dst_current;
      uint8 r, g, b;
      float a, ia;
      int tx, ty = img->height();

      while (ty--)
      {
            dst_current = dst_start;
            tx = img->width();
            while (tx--)
            {
                  a = ((*(src + 3)) * transparency) / 65025.0f, ia = 1.0f - a;
#if bpp==16
                  r = (*src) * a + ((*dst_current >> 8) & 0xF8) * ia; src++;
                  g = (*src) * a + ((*dst_current >> 3) & 0xFC) * ia; src++;
                  b = (*src) * a + ((*dst_current << 3) & 0xF8) * ia; src += 2;
                  *dst_current = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
#endif
#if bpp==32
                  r = (*src) * a + ((*dst_current >> 16) & 0xFF) * ia; src++;
                  g = (*src) * a + ((*dst_current >> 8) & 0xFF) * ia; src++;
                  b = (*src) * a + (*dst_current & 0xFF) * ia; src += 2;
                  *dst_current = (r << 16) | (g << 8) | b;
#endif



                  dst_current++;
            }
            dst_start += vinfo.xres_virtual; // proceed to next line
      }
      //   WARN_RESTORE            WARN_RESTORE

}
void video_driver::draw_shape(Poly* sh, const ARGB fill_color, const ARGB outline_color) {

      int32  point_index;


      if (fill_color != clNone)
      {
            // init edge table
            polydriver->reset();
            point_index = 0;
            // fill edge table

            // iterate paths
            for (int32 path_end : sh->path_ptr)
            {
                  path_end--;
                  //printf("---\ndraw_shape, points: %d\n", sh->work.size());                  //printf("---\nadd_segment: %d, %d\n", point_index, path_end);

                  // add "closing" segment
                  polydriver->add_segment(&sh->work[point_index], &sh->work[path_end]);

                  // add others segments
                  while (point_index < path_end)
                  {
                        //printf("add_segment: %d, %d\n", point_index, point_index + 1);
                        polydriver->add_segment(&sh->work[point_index], &sh->work[point_index + 1]);
                        point_index++;
                  }
            }
            // each scanline
            for (int32 y = 0; y < _height; y++)
            {
                  // put new vertices to active edge table
                  polydriver->update_aet(y);
                  polydriver->cleanup_aet(y);
                  //printf("Line %d, aet cnt: %d\n", y, polydriver->aet.cnt);
                  int bucket_no = 0, coordCount = 0, x1, x2;
                  while (bucket_no < polydriver->aet.cnt)
                  {
                        if ((coordCount % 2) == 0)
                              x1 = (int32)polydriver->aet.b[bucket_no].x;
                        else
                        {
                              x2 = (int32)polydriver->aet.b[bucket_no].x;
                              draw_line_fast(y, x1, x2, fill_color);

                        }
                        coordCount++;
                        bucket_no++;
                  }

                  polydriver->update_slope();


            }

      }
      if (outline_color != clNone)
      {
            point_index = 0;
            // iterate paths
            for (int32 path_end : sh->path_ptr)
            {
                  path_end--;
                  //printf("---\ndraw_shape, points: %d\n", sh->work.size());                  //printf("---\nadd_segment: %d, %d\n", point_index, path_end);

                  // draw "closing" segment
                  draw_line(&sh->work[point_index], &sh->work[path_end], outline_color);
                  //polydriver->add_segment();

                  // add others segments
                  while (point_index < path_end)
                  {
                        draw_line(&sh->work[point_index], &sh->work[point_index + 1], outline_color);
                        point_index++;
                  }
            }
      }
}
/*
void video_driver::draw_shape(const Poly* sh, const ARGB fill_color, const ARGB outline_color)
{
      if (fill_color != clNone)
      {

            edge_tables_reset();
            calc_fill(sh);
            draw_fill(fill_color);
      }
      if (outline_color != clNone)
      {
            draw_outline_v2(sh, outline_color);
      }
}
*/
video_driver* screen;