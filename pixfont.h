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

/*
#define CHAR_SUNRISE \x80
#define CHAR_SUNSET \x81
#define CHAR_SATUSED \x82
#define CHAR_SATTOTAL \x83
#define CHAR_DISTPART \x84
#define CHAR_DISTALL \x85
#define CHAR_SHORTSPACE \x85
*/
struct char_info_s
{
      int32 width_start, width_end, real_width_start, real_width_end;
      puint32 data;
      int32 width() { return width_end - width_start + 1; }
      int32 real_width() { return real_width_end - real_width_start + 1; }

};
class font {
private:
      int32 height_start, real_height_start, height_end, real_height_end, interval;
      puint32 pixdata = nullptr;
      std::vector <char_info_s> char_info;
public:
      int32 height() { return height_end - height_start; };
      int32 real_height() { return real_height_end - real_height_start + 1; };
      int32 height_start_delta() { return height_start - real_height_start; }
      int32 get_interval() { return interval; }
      void set_interval(int32 font_interval)
      {
            interval = font_interval;
      }
      // uint32 offset() { return ofs; };
      char_info_s* get_char_info(uint8 charcode)
      {

            size_t v = charcode - 32;
            if (v >= char_info.size())
                  return nullptr;
            return &char_info[v];

      }
      font() {};
      font(std::string filename);
      ~font();
};



class image {
private:
      uint32 data_size;
      int32 w, h;
      puint8 data;
      bool loaded;
public:
      bool is_loaded() { return loaded; }
      int32 width() { return w; }
      int32 height() { return h; }
      puint8 data_ptr() { return data; };
      image() {}
      image(std::string filename) {
            data_size = h = w = 0;
            loaded = false;
            data = NULL;

            std::ifstream inp{ filename, std::ios_base::binary };

            if (!inp) return;//                 printf("Error open file: %s", argv[1]);
            //size_t isize, expected_size = w * h * bpp / 8;
            inp.seekg(0, inp.end);
            size_t inp_isize = inp.tellg();
            uint8* inp_data = new uint8[inp_isize];
            inp.seekg(0, inp.beg);
            inp.read((char*)inp_data, inp_isize);

            uint32 tw, th;
            uint32 error = lodepng_decode32(&data, &tw, &th, inp_data, inp_isize);
            w = (int32)tw;
            h = (int32)th;
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