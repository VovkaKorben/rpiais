#pragma once
#ifndef __PIXFONT_H
#define __PIXFONT_H
#include "mydefs.h"
const int VALIGN_TOP = 0x01;
const int VALIGN_BOTTOM = 0x02;
const int VALIGN_CENTER = 0x04;
const int HALIGN_LEFT = 0x10;
const int HALIGN_RIGHT = 0x20;
const int HALIGN_CENTER = 0x40;
class bitmap_font
{

private:
      int32 count;
public:
      puint8 pixdata;
      vector<puint8> start_lut;
      vector<int32>       width_lut;
      int32 height;
      int32 get_char(char c)
      {
            /*
now for get symbol data
code -= 32
start_lut[code] = pointer to data start
width_lut[code] = width symbol in pixels
data len occupied by symbol = width_lut[code]*height
pix data stored sequentally left to right, from top to bottom

*/
            c -= 32;
            if (c >= count)
                  return -1;
            return c;

      }
      bitmap_font() {};
      bitmap_font(const char * filename);
      ~bitmap_font() {
            delete[] pixdata;
            //delete[] start_lut;            delete[] width_lut;
      };
};

#endif