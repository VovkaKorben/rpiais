#pragma once
#ifndef __PIXFONT_H
#define __PIXFONT_H
#include "mydefs.h"
#include "lodepng.h"

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
class font
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

            c -= 32;
            if (c >= count)
                  return -1;
            return c;

      }
      font(std::string filename)
      {
            count = 0;
            std::ifstream inp{ filename, std::ios_base::binary };
            if (!inp) {
                  printf("Can't open font file: %s", filename.c_str());
                  return;
            }
            inp.seekg(0, inp.end);
            size_t inp_isize = inp.tellg();
            uint8 * inp_data = new uint8[inp_isize];
            inp.seekg(0, inp.beg);
            inp.read((char *)inp_data, inp_isize);


      };
      ~font() {
            delete[] pixdata;
            //delete[] start_lut;            delete[] width_lut;
      };
};


class image {
private:
      uint32 data_size, w, h;
      puint8 data;
      bool loaded;
public:
      bool is_loaded() { return loaded; }
      uint32 width() { return w; }
      uint32 height() { return h; }
      puint8 data_ptr() { return data; };
      image(std::string filename) {
            data_size = h = w = 0;
            loaded = false;
            data = NULL;

            std::ifstream inp{ filename, std::ios_base::binary };

            if (!inp) return;//                 printf("Error open file: %s", argv[1]);
            //size_t isize, expected_size = w * h * bpp / 8;
            inp.seekg(0, inp.end);
            size_t inp_isize = inp.tellg();
            uint8 * inp_data = new uint8[inp_isize];
            inp.seekg(0, inp.beg);
            inp.read((char *)inp_data, inp_isize);


            unsigned error = lodepng_decode32(&data, &w, &h, inp_data, inp_isize);
            delete[]inp_data;
            if (error) {
                  std::cout << "PNG encoding error " << error << ": " << lodepng_error_text(error) << std::endl;
                  return;
            }

            loaded = true;

      }


      ~image() {


      }
};

#endif