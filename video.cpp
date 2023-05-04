#include <unistd.h>
#include <algorithm>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "video.h"
#include "pixfont.h"

#define FB_NO_ERROR 0;
#define FB_OPEN_FAILED 1;
#define FB_GET_VSCREENINFO_FAILED 2;
#define FB_GET_FSCREENINFO_FAILED 3;
#define FB_MMAP_FAILED 4;
#define FB_PUT_VSCREENINFO_FAILED 5;
#define FB_PAN_DISPLAY_FAILED 6;


int CENTER_X, CENTER_Y;

irect VIEWBOX_RECT, SCREEN_RECT;

void _insertionSort(bucketset * b) {
      double _fx, _s;
      int j, _y, _ix;
      for (int i = 1; i < b->cnt; i++)
      {
            _y = b->barr[i].y;
            _fx = b->barr[i].fx;
            _ix = b->barr[i].ix;
            _s = b->barr[i].slope;
            j = i - 1;
            while ((j >= 0) && (_ix < b->barr[j].ix))
            {
                  b->barr[j + 1].y = b->barr[j].y;
                  b->barr[j + 1].fx = b->barr[j].fx;
                  b->barr[j + 1].ix = b->barr[j].ix;
                  b->barr[j + 1].slope = b->barr[j].slope;
                  j--;
            }
            b->barr[j + 1].y = _y;
            b->barr[j + 1].fx = _fx;
            b->barr[j + 1].ix = _ix;
            b->barr[j + 1].slope = _s;
      }
}
////////////////////////////////////////////////////////////
void video_driver::flip()
{

      vinfo.yoffset = height * current_fb;
      //vinfo.activate = FB_ACTIVATE_VBL;
      if (ioctl(fbdev, FBIOPAN_DISPLAY, &vinfo)) { last_error = FB_PAN_DISPLAY_FAILED; return; }
      current_fb = (current_fb + 1) % buffer_count;
      fb_current = fb_start + pixel_count * current_fb;
      last_error = FB_NO_ERROR;
}
video_driver::video_driver(int _width, int _height, int _bpp, int _buffer_count) {


      fb_fix_screeninfo finfo;
      width = _width, height = _height, buffer_count = _buffer_count;
      bpp = _bpp, bytes_per_pix = _bpp >> 3;

      fbdev = open("/dev/fb0", O_RDWR);
      if (!fbdev) { last_error = FB_OPEN_FAILED; return; }

      if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) { last_error = FB_GET_VSCREENINFO_FAILED; return; }
      // fill new values
      vinfo.xres = width; // try a smaller resolution
      vinfo.yres = height;
      vinfo.xres_virtual = width;
      vinfo.yres_virtual = height * buffer_count; // double the physical
      vinfo.bits_per_pixel = bpp;
      if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo)) {
            printf("Error setting variable information.\n");
      }

      if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) { last_error = FB_GET_FSCREENINFO_FAILED; return; }

      screen_size = vinfo.xres * vinfo.yres * bytes_per_pix;
      fb_size = screen_size * buffer_count;
      pixel_count = width * height;

      fb_start = (puint32)mmap(NULL, fb_size, PROT_WRITE | PROT_READ, MAP_SHARED, fbdev, 0);// map framebuffer to user memory 
      if ((intptr_t)fb_start == -1) { last_error = FB_MMAP_FAILED; return; }

      // set buffer 0 as first
      current_fb = 0;
      fb_current = fb_start;


      CENTER_X = width / 4;
      CENTER_Y = height / 2;
      VIEWBOX_RECT.assign_pos(-CENTER_X, -CENTER_Y, CENTER_X - 1, CENTER_Y - 1);
      SCREEN_RECT.assign_pos(0, 0, width - 1, height - 1);

      et = new bucketset[height];

      last_error = FB_NO_ERROR;
}
video_driver::~video_driver() {
      delete[]et;
      munmap(fb_start, fb_size);
      close(fbdev);
}

////////////////////////////////////////////////////////////
inline void video_driver::draw_pix(const int x, const int y, const BGRA color) {
      if (x < 0) return;
      if (y < 0) return;
      if (x >= width) return;
      if (y >= height) return;
      //puint8 addr = (puint8)(pix_ptr - y * SCANLINE_LEN + x * BYTES_PER_PIXEL);
      puint32 addr = fb_current + x + (height - y - 1) * width;
      int alpha = color & 0xFF;
      if (alpha == 0) // no transparency, simple put color
      {
            *addr = color;
            // int32 x = *((puint32)addr);

          //  *((puint32)addr) = color;
            //int32 y = *((puint32)addr);
      }
      else if (alpha != 255) {
            //new_color = (alpha) * (foreground_color)+(1 - alpha) * (background_color)
            // r0 = int(r1 * a + r0 * ia);
            puint8 addr8 = (puint8)addr;
            float a = (float)alpha / 255.0f, ia = 1.0f - a;
            *addr8 = (uint8)(((color >> 24) & 0xFF) * a + ia * *addr8);
            addr8++;
            *addr8 = ((color >> 16) & 0xFF) * a + ia * *addr8;
            addr8++;
            *addr8 = ((color >> 8) & 0xFF) * a + ia * *addr8;
            addr8++;
            *addr8 = 0;

      }
}
void video_driver::draw_line(int x0, int y0, int x1, int y1, const BGRA color) {
      int w = x1 - x0;
      int h = y1 - y0;
      int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
      if (w < 0) dx1 = -1; else if (w > 0) dx1 = 1;
      if (h < 0) dy1 = -1; else if (h > 0) dy1 = 1;
      if (w < 0) dx2 = -1; else if (w > 0) dx2 = 1;
      int longest = abs(w);
      int shortest = abs(h);
      if (!(longest > shortest)) {
            longest = abs(h);
            shortest = abs(w);
            if (h < 0) dy2 = -1; else if (h > 0) dy2 = 1;
            dx2 = 0;
      }
      int numerator = longest >> 1;
      for (int i = 0; i <= longest; i++) {
            draw_pix(x0, y0, color);
            //putpixel(x, y, color);
            numerator += shortest;
            if (!(numerator < longest)) {
                  numerator -= longest;
                  x0 += dx1;
                  y0 += dy1;
            }
            else {
                  x0 += dx2;
                  y0 += dy2;
            }
      }
}
void video_driver::draw_line_fast(int y, int xs, int xe, const BGRA color)
/*
* ONLY HORIZONTAL LINES
* WITHOUT ALPHA
*/
{
      // check line (or part) lies in window
      if (xe < 0 || xs >= width) return;

      // clip start & end to window bounds
      if (xs < 0) xs = 0;
      if (xe >= width) xe = width - 1;

      puint32 addr = fb_current + (height - y - 1) * width + xs;
      xe -= xs;

      while (xe--)
            *(addr++) = color;
}
void video_driver::draw_polyline(int x, int y, const BGRA color, int close_polyline)
{
      static int count = 0;
      static int first_x, first_y;
      static int prev_x, prev_y;

      if (close_polyline != 0 && count > 1)
      {
            draw_line(prev_x, prev_y, first_x, first_y, color);
            count = 0;
      }
      else

            if (count == 0)
            {
                  first_x = x;
                  first_y = y;
                  prev_x = x;
                  prev_y = y;
                  count++;
            }
            else
                  if (prev_x != x || prev_y != y)
                  {
                        //printf("%d,%d -> %d,%d\n", prev_x, prev_y, x, y );
                        draw_line(prev_x, prev_y, x, y, color);
                        prev_x = x;
                        prev_y = y;
                        count++;
                  }
}
void video_driver::draw_text(int font_index, int x, int y, string s, const BGRA color, int flags)
{
      if (fonts.count(font_index) == 0)
            return;
      puint8 data;
      int32 code, char_width, char_height, ty;
      // std::transform(s.begin(), s.end(), s.begin(), ::toupper);

       // detect text dimensions
      int overall_width = 0, overall_count = 0;
      for (char & c : s) {
            code = fonts[font_index].get_char(c);
            if (code == -1)
                  continue;
            overall_width += fonts[font_index].width_lut[code];
            overall_count++;
      }
      overall_width += overall_count - 1;

      if ((flags & HALIGN_CENTER) == HALIGN_CENTER)            x -= overall_width / 2;
      else if ((flags & HALIGN_RIGHT) == HALIGN_RIGHT)            x -= overall_width;

      if ((flags & VALIGN_CENTER) == VALIGN_CENTER)            y += fonts[font_index].height / 2;
      else   if ((flags & VALIGN_BOTTOM) == VALIGN_BOTTOM)            y += fonts[font_index].height;


      //y += fnt->height;
      for (char & c : s) {
            if (x >= width)
                  break;
            code = fonts[font_index].get_char(c);
            if (code == -1)
                  continue;
            data = fonts[font_index].start_lut[code];
            char_width = fonts[font_index].width_lut[code];
            char_height = fonts[font_index].height;
            ty = y;
            while (char_height--)
            {
                  for (int tx = 0; tx < char_width; tx++)
                  {
                        if (*data)
                              draw_pix(x + tx, ty, color);
                        data++;
                  }
                  ty--;
            }
            x += char_width + 1;
      }

}
void video_driver::draw_circle(int cx, int cy, int radius, const BGRA color)
{
      int f = 1 - radius;
      int ddF_x = 0;
      int ddF_y = -2 * radius;
      int x = 0;
      int y = radius;

      draw_pix(cx, cy + radius, color);
      draw_pix(cx, cy - radius, color);
      draw_pix(cx + radius, cy, color);
      draw_pix(cx - radius, cy, color);

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
            draw_pix(cx + x, cy + y, color);
            draw_pix(cx - x, cy + y, color);
            draw_pix(cx + x, cy - y, color);
            draw_pix(cx - x, cy - y, color);
            draw_pix(cx + y, cy + x, color);
            draw_pix(cx - y, cy + x, color);
            draw_pix(cx + y, cy - x, color);
            draw_pix(cx - y, cy - x, color);
      }
}
void video_driver::draw_bg(const BGRA background)
{
      puint32 addr = fb_current; // fb start pointer
      for (int pix = 0; pix < pixel_count; pix++)
            *(addr++) = background;

}
////////////////////////////////////////////////////////////
void video_driver::edge_tables_reset()
{
      aet.cnt = 0;
      for (int i = 0; i < height; i++)
            et[i].cnt = 0;
}

void video_driver::edge_store_tuple_float(bucketset * b, int y_end, double  x_start, double  slope)
{
      b->barr[b->cnt].y = y_end;
      b->barr[b->cnt].fx = x_start;
      b->barr[b->cnt].slope = slope;
      b->cnt++;
      //	_insertionSort(b);

}
void video_driver::edge_store_tuple_int(bucketset * b, int y_end, double  x_start, double  slope)
{
      //if (dbg_flag)		cout << string_format("+ Added\ty: %d\tx: %.3f\ts: %.3f\n", y_end, x_start, slope);
      b->barr[b->cnt].y = y_end;
      b->barr[b->cnt].fx = x_start;
      b->barr[b->cnt].ix = int(round(x_start));
      b->barr[b->cnt].slope = slope;
      b->cnt++;
      //	_insertionSort(b);

}
void video_driver::edge_store_table(intpt pt1, intpt pt2) {
      double dx, dy, slope;

      // if both points lies below or above viewable rect - edge skipped
      if ((pt1.y < 0 and pt2.y < 0) || (pt1.y >= height and pt2.y >= height))
            return;
      dy = pt1.y - pt2.y;

      // horizontal lines are not stored in edge table
      if (dy == 0)
            return;
      dx = pt1.x - pt2.x;

      if (is_zero(dx))
            slope = 0.0;
      else
            slope = dx / dy;

      //check if one point lies below view rect - recalculate intersection with zero scanline
      if (pt1.y < 0)
      {
            pt1.x = int(round(pt1.x - pt1.y * slope));
            pt1.y = 0;
      }
      else if (pt2.y < 0)
      {
            pt2.x = int(round(pt2.x - pt2.y * slope));
            pt2.y = 0;

      }

      int _y, sc;
      double  _x;

      if (dy > 0)
      {
            sc = pt2.y;
            _y = pt1.y;
            _x = pt2.x;
      }
      else
      {
            sc = pt1.y;
            _y = pt2.y;
            _x = pt1.x;
      }


      //if (sc < 0 || sc >= VIEW_HEIGHT)            throw exception("incorrect scanline");
      //cout << string_format("Store tuple:\ty: %d->%d\tx: %.3f\ts: %.3f", sc, _y, _x, slope) << endl;
      edge_store_tuple_float(&et[sc], _y, _x, slope);


}
void video_driver::edge_update_slope(bucketset * b) {
      for (int i = 0; i < b->cnt; i++)
      {
            b->barr[i].fx += b->barr[i].slope;
            b->barr[i].ix = int(round(b->barr[i].fx));
      }
}
void video_driver::edge_remove_byY(bucketset * b, int scanline_no)
{
      int i = 0, j;
      while (i < b->cnt)
      {
            if (b->barr[i].y == scanline_no)
            {
                  //if (dbg_flag)				cout << string_format("- Removed\ty: %d\tx: %.3f\n", scanline_no, b->barr[i].x);

                  for (j = i; j < b->cnt - 1; j++)
                  {
                        b->barr[j].y = b->barr[j + 1].y;
                        b->barr[j].fx = b->barr[j + 1].fx;
                        b->barr[j].ix = b->barr[j + 1].ix;
                        b->barr[j].slope = b->barr[j + 1].slope;

                  }
                  b->cnt--;
            }
            else
                  i++;
      }
}
////////////////////////////////////////////////////////////
void video_driver::calc_fill(const poly * sh) {

      intpt prev_point, point;
      int path_end, point_id = 0;
      for (int path_id = 0; path_id < sh->path_count; path_id++)
      {
            path_end = sh->pathindex[path_id + 1];
            prev_point = sh->work[path_end - 1];
            while (point_id < path_end)
            {
                  point = sh->work[point_id];
                  //cout << "Process line: " << point << " -> " << prev_point << endl;

                  edge_store_table(point, prev_point);
                  prev_point = point;
                  point_id++;
            }

      }
}
void video_driver::draw_fill(const BGRA color)
{
      /*if (dbg_flag)
      {
            dump_shapes();
            print_table();
      }*/


      for (int scanline_no = 0; scanline_no < height; scanline_no++)
      {
            /*if (dbg_flag && scanline_no >= 156 && scanline_no <= 158)
            {
                  cout << "------------------------------" << endl;
                  cout << "Scanline before: " << scanline_no << endl;
                  printTuple(&aet);
                  cout << "-  -  -  -  -  -  -  -  -  -  -  -\n\n" << endl;
            }*/


            for (int b = 0; b < et[scanline_no].cnt; b++)
            {
                  edge_store_tuple_int(
                        &aet,
                        et[scanline_no].barr[b].y,
                        et[scanline_no].barr[b].fx,
                        et[scanline_no].barr[b].slope);

            }

            edge_remove_byY(&aet, scanline_no);
            _insertionSort(&aet);


            int bucket_no = 0, coordCount = 0, x1, x2;
            while (bucket_no < aet.cnt)
            {
                  if ((coordCount % 2) == 0)
                        x1 = aet.barr[bucket_no].ix;
                  else
                  {
                        x2 = aet.barr[bucket_no].ix;
                        draw_line_fast(scanline_no, x1, x2, color);

                  }
                  coordCount++;
                  bucket_no++;
            }

            edge_update_slope(&aet);
      }

}
void video_driver::draw_outline(const poly * sh, const BGRA color)
{


      intpt //* prev_point,
            * point;
      int path_end, point_id = 0;
      for (int path_id = 0; path_id < sh->path_count; path_id++)
      {
            path_end = sh->pathindex[path_id + 1];
            //prev_point = &sh->work[path_end - 1];

            while (point_id < path_end)
            {

                  point = &sh->work[point_id];
                  draw_polyline(point->x, point->y, color, 0);
                  // prev_point = point;
                  point_id++;
            }

            draw_polyline(0, 0, color, 1); // close poly

      }
}
////////////////////////////////////////////////////////////
void video_driver::draw_shape(const poly * sh, const BGRA fill_color, const BGRA outline_color)
{
      if (fill_color != NO_COLOR)
      {

            edge_tables_reset();
            calc_fill(sh);
            draw_fill(fill_color);
      }
      if (outline_color != NO_COLOR)
      {
            draw_outline(sh, outline_color);
      }
}


bool video_driver::load_font(const int index, const char * filename)
{
      if (fonts.count(index) != 0)
            return false;
      fonts.emplace(std::piecewise_construct,
                    std::forward_as_tuple(index),
                    std::forward_as_tuple(filename));

      return true;
}