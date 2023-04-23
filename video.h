#pragma once
#ifndef __VIDEO_H
#define __VIDEO_H
#include "mydefs.h"
#include "pixfont.h"

//#include "PixelToaster.h"
//using namespace PixelToaster;
//extern vector<TrueColorPixel> pixels;
extern irect VIEWBOX_RECT, SCREEN_RECT;


const rgba NO_COLOR = 0xFFFFFFFF;
//void video_init();
//int video_init(frame_func ff);
void video_frame_start(const rgba background);
void video_frame_end();
void video_loop_start();
void draw_line(int x1, int y1, int x2, int y2, const rgba color);

inline void draw_pix(const int x, const int y, const rgba color);
void draw_shape(const poly * sh, const rgba fill_color, const rgba outline_color);
void draw_text(bitmap_font * fnt, int x, int y, string s, const rgba color, int flags = VALIGN_BOTTOM | HALIGN_LEFT);
void draw_circle(int cx, int cy, int radius, const rgba color);
#endif
