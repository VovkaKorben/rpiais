#pragma once
#ifndef __PIXFONT_H
#define __PIXFONT_H
#include "mydefs.h"
#include <bitset>

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
      std::vector<puint8> start_lut;
      std::vector<int32>       width_lut;
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

namespace pngchunk {
#pragma pack(1)
      struct FHDR {
            uint8 marker;
            uint8 name[3];
            uint16 crlf;
            uint8 dos_eof;
            uint8 nix_eof;
      };
      struct CHUNK_HDR {
            uint32 length;
            char type[4];

      };
      struct IHDR {


            uint32 width;
            uint32 height;
            uint8 bit_depth;
            uint8 color_type;
            uint8 compression_method;
            uint8 filter_method;
            uint8 interlace_method;
      };
#pragma pack() 
}


class image {
private:
      uint32 data_size, alpha_size, w, h;
      uint8 * data, * alpha;
      bool loaded;
public:
      image() {
            data_size = alpha_size = h = w = 0;
            loaded = false;
            data = alpha = NULL;
      }
      int load_png(const char * filename) {
            data_size = alpha_size = h = w = 0;
            loaded = false;
            data = alpha = NULL;
            //  png_image png(filename);            if (!png.is_loaded())                  throw "png load error";

            return 0;
      }

      ~image() {


      }
};

#endif