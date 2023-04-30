#pragma once
#ifndef __VIDEO_H
#define __VIDEO_H
#include "mydefs.h"
#include "pixfont.h"

//#include "PixelToaster.h"
//using namespace PixelToaster;
//extern vector<TrueColorPixel> pixels;

/*
inline uint32 rgb565to888(const uint16 v)
{
      return ((v & 0xF800) << 8) | // r
            ((v & 0x7E0) << 5) | // g
            ((v & 0x1F) << 3); // b 
}
inline uint16  rgb888to565(const uint32  v)
{
      return ((v & 0xF80000) >> 8) | // red
            ((v & 0x00FC00) >> 5) | // green
            ((v & 0xF8) >> 3); // blue
}
*/

extern irect VIEWBOX_RECT, SCREEN_RECT;
extern int CENTER_X , CENTER_Y ;
extern bitmap_font * small_font;

const BGRA NO_COLOR = 0xFFFFFFFF;
int video_init(int window_x, int window_y);
void video_close();
//int video_init(frame_func ff);
//void video_frame_start(const BGRA background);
void video_frame_end();
void video_loop_start();
void draw_line(int x1, int y1, int x2, int y2, const BGRA color);

 void draw_pix(const int x, const int y, const BGRA color);
void draw_shape(const poly * sh, const BGRA fill_color, const BGRA outline_color);
void draw_text(bitmap_font * fnt, int x, int y, string s, const BGRA color, int flags = VALIGN_BOTTOM | HALIGN_LEFT);
void draw_circle(int cx, int cy, int radius, const BGRA color);
void draw_bg(const BGRA background);
#endif
