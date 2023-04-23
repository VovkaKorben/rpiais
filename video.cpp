#include <algorithm>
#include "video.h"

//#include "PixelToaster.h"
//using namespace PixelToaster;
//Display * pix_screen;
//vector<TrueColorPixel> pixels(VIEW_WIDTH * VIEW_HEIGHT);
irect VIEWBOX_RECT, SCREEN_RECT;

//frame_func frame_handler;
//#define invertYaxis false
#define max_vertices 500

struct bucket {
      int y, ix;
      double fx, slope;
};
struct bucketset {
      int cnt;
      bucket barr[max_vertices];
};
bucketset aet, et[VIEW_HEIGHT];
/*

void video_loop_start() {
      while (pix_screen.open())
      {
            // cout << "video_loop_start\n";
    //        frame_handler();
      }
}
int video_init(frame_func ff) {
  //    frame_handler = ff;
      cout << "using PixelToaster as render system\n";


      //bitmap_font fnt("C:\\ais\\cpp\\include\\font.bmp"); // load bitmap font
      //load_font("C:\\ais\\cpp\\include\\font.bmp"); // load bitmap font
      //pix_screen.listener();

   //   pix_screen = new PixelToaster::Display("TrueColor Example", VIEW_WIDTH, VIEW_HEIGHT, PixelToaster::Output::Default, PixelToaster::Mode::TrueColor);
      if (!pix_screen.open("AIS", VIEW_WIDTH, VIEW_HEIGHT, PixelToaster::Output::Default, PixelToaster::Mode::TrueColor))
            return 1;


      return 0;

}*/
void video_frame_start(const rgba background) {
      for (int i = 0; i < VIEW_WIDTH * VIEW_HEIGHT - 1; i++)
      {
       //     pixels[i].integer = background;
      }
}
void video_frame_end() {
      //pix_screen->update(pixels);
}
////////////////////////////////////////////////////////////
inline void draw_pix(const int x, const int y, const rgba color) {
      if (x < 0) return;
      if (y < 0) return;
      if (x >= VIEW_WIDTH) return;
      if (y >= VIEW_HEIGHT) return;
      int addr = (VIEW_HEIGHT - y - 1) * VIEW_WIDTH + x;
      int alpha = (color >> 24) & 0xFF;
      if (alpha)
      {
            float a = alpha / 255.0f, ia = 1 - a;
            int bg = 0;// pixels[addr].integer;
            int r0 = (bg >> 16) & 0xFF, g0 = (bg >> 8) & 0xFF, b0 = bg & 0xFF;
            int r1 = (color >> 16) & 0xFF, g1 = (color >> 8) & 0xFF, b1 = color & 0xFF;

            //new_color = (alpha) * (foreground_color)+(1 - alpha) * (background_color)
            r0 = int(r1 * a + r0 * ia);
            g0 = int(g1 * a + g0 * ia);
            b0 = int(b1 * a + b0 * ia);
         //   pixels[addr].integer = (r0 << 16) | (g0 << 8) | b0;
      }
      else {
       //     pixels[addr].integer = color;
      }
}
void draw_line(int x0, int y0, int x1, int y1, const rgba color) {
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

void draw_line_fast(int y, int xs, int xe, const rgba color)
/*
* ONLY HORIZONTAL LINES
* WITHOUT ALPHA
*/
{
      if (xe < 0) return;
      if (xs >= VIEW_WIDTH) return;

      if (xs < 0) xs = 0;
      if (xe >= VIEW_WIDTH) xe = VIEW_WIDTH - 1;
      y = (VIEW_HEIGHT - y - 1) * VIEW_WIDTH;
      xs += y;
      xe += y;
      for (int x = xs; x <= xe; x++)
      {
       //     pixels[x].integer = color;
      }
}

void draw_polyline(int x, int y, const rgba color, int close_polyline = 0)
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
                        draw_line(prev_x, prev_y, x, y, color);
                        prev_x = x;
                        prev_y = y;
                        count++;
                  }
}
void draw_text(bitmap_font * fnt, int x, int y, string s, const rgba color, int flags)
{
      puint8 data;
      int32 code, char_width, char_height, ty;
      std::transform(s.begin(), s.end(), s.begin(), ::toupper);

      // detect text dimensions
      int overall_width = 0, overall_count = 0;
      for (char & c : s) {
            code = fnt->get_char(c);
            if (code == -1)
                  continue;
            overall_width += fnt->width_lut[code];
            overall_count++;
      }
      overall_width += overall_count - 1;

      if ((flags & HALIGN_LEFT) == HALIGN_LEFT)
      {
      }
      else if ((flags & HALIGN_CENTER) == HALIGN_CENTER)
      {
            x -= overall_width / 2;
      }
      else if ((flags & HALIGN_RIGHT) == HALIGN_RIGHT)
      {
            x -= overall_width;
      }

      if ((flags & VALIGN_TOP) == VALIGN_TOP)
      {

      }
      else   if ((flags & VALIGN_CENTER) == VALIGN_CENTER)
      {
            y += fnt->height / 2;
      }
      else   if ((flags & VALIGN_BOTTOM) == VALIGN_BOTTOM)
      {
            y += fnt->height;
      }


      //y += fnt->height;
      for (char & c : s) {
            if (x >= VIEW_WIDTH)
                  break;
            code = fnt->get_char(c);
            if (code == -1)
                  continue;
            data = fnt->start_lut[code];
            char_width = fnt->width_lut[code];
            char_height = fnt->height;
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



void draw_circle(int cx, int cy, int radius, const rgba color)
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

////////////////////////////////////////////////////////////
void edge_tables_reset()
{
      aet.cnt = 0;
      for (int i = 0; i < VIEW_HEIGHT; i++)
            et[i].cnt = 0;
}
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
void edge_store_tuple_float(bucketset * b, int y_end, double  x_start, double  slope)
{
      b->barr[b->cnt].y = y_end;
      b->barr[b->cnt].fx = x_start;
      b->barr[b->cnt].slope = slope;
      b->cnt++;
      //	_insertionSort(b);

}
void edge_store_tuple_int(bucketset * b, int y_end, double  x_start, double  slope)
{
      //if (dbg_flag)		cout << string_format("+ Added\ty: %d\tx: %.3f\ts: %.3f\n", y_end, x_start, slope);
      b->barr[b->cnt].y = y_end;
      b->barr[b->cnt].fx = x_start;
      b->barr[b->cnt].ix = int(round(x_start));
      b->barr[b->cnt].slope = slope;
      b->cnt++;
      //	_insertionSort(b);

}
void edge_store_table(intpt pt1, intpt pt2) {
      double dx, dy, slope;

      // if both points lies below or above viewable rect - edge skipped
      if ((pt1.y < 0 and pt2.y < 0) || (pt1.y >= VIEW_HEIGHT and pt2.y >= VIEW_HEIGHT))
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
void edge_update_slope(bucketset * b) {
      for (int i = 0; i < b->cnt; i++)
      {
            b->barr[i].fx += b->barr[i].slope;
            b->barr[i].ix = int(round(b->barr[i].fx));
      }
}
void edge_remove_byY(bucketset * b, int scanline_no)
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
void calc_fill(const poly * sh) {

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
void draw_fill(const rgba color)
{
      /*if (dbg_flag)
      {
            dump_shapes();
            print_table();
      }*/


      for (int scanline_no = 0; scanline_no < VIEW_HEIGHT; scanline_no++)
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
void draw_outline(const poly * sh, const rgba color)
{
      intpt * prev_point, * point;
      int path_end, point_id = 0;
      for (int path_id = 0; path_id < sh->path_count; path_id++)
      {
            path_end = sh->pathindex[path_id + 1];
            prev_point = &sh->work[path_end - 1];

            while (point_id < path_end)
            {

                  point = &sh->work[point_id];
                  draw_polyline(point->x, point->y, color, 0);
                  prev_point = point;
                  point_id++;
            }
            draw_polyline(0, 0, color, 1);

      }




}
////////////////////////////////////////////////////////////
void draw_shape(const poly * sh, const rgba fill_color, const rgba outline_color)
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