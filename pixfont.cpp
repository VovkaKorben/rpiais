#include <vector>
#include "pixfont.h"

enum class searchdir { horizontal, vertical };


class mypng
{
private:
      const uint8 treshold = 127;
      bool _loaded;
      puint32 d;
      uint32 w, h;
public:
      uint32 width() { return w; }
      uint32 height() { return h; }
      puint32 data() { return d; }

      bool loaded() { return _loaded; }
      bool get_marker(int start, int pos, searchdir dir, int32 & s, int32 & e)
      {
            /* get marker position, checking by transparency > treshold
            if dir==0, horizontal marker at scanline POS from position START
            if dir==1, marker should be vertival and POS assume column      */
            bool start_found = false;
            //int chk = dir == searchdir::vertical ? pos + start * w : pos * w + start;
            puint32 td = d + (dir == searchdir::vertical ? pos + start * w : pos * w + start);
            int end_pos = dir == searchdir::vertical ? h : w;
            uint8 alpha;
            for (int search_pos = start; search_pos < end_pos; search_pos++)
            {
                  alpha =(uint8)( (*td) >> 24);
                  if (start_found) {
                        if (alpha < treshold) {
                              e = search_pos - 1;
                              return true;
                        }
                  }
                  else
                        if (alpha > treshold)
                        {
                              start_found = true;
                              s = search_pos;
                        }
                  td += dir == searchdir::vertical ? w : 1;
            }
            return false;
      };
      mypng(const std::string filename)
      {
            _loaded = false;
            // read PNG data
            std::ifstream inp{ filename, std::ios_base::binary };
            if (!inp) {
                  printf("Can't open png file: %s\n", filename.c_str());
                  return;
            }
            inp.seekg(0, inp.end);
            size_t inp_isize = inp.tellg();
            uint32 * inp_data = new uint32[inp_isize];
            inp.seekg(0, inp.beg);
            inp.read((pchar)inp_data, inp_isize);

            puint8 ttt;
            unsigned error = lodepng_decode32(&ttt, &w, &h, (puint8)inp_data, inp_isize);
            d = (puint32)ttt;

            delete[]inp_data;
            if (error) {
                  std::cout << "PNG encoding error " << error << ": " << lodepng_error_text(error) << std::endl;
                  return;
            }

            _loaded = true;
      };

};
font::font(std::string filename)

{
      int32  count = 0;
      interval = 1;
      mypng png_file(filename);
      if (!png_file.loaded())   
            {
                  printf("font load error, no file: %s\n", filename.c_str());
                  return;
            };

      // get vertical dimensions
      if (!png_file.get_marker(0, 0, searchdir::vertical, height_start, height_end)) { std::cout << "Load font failed. Can't find height marker #1" << std::endl;  return; }
      if (!png_file.get_marker(0, 1, searchdir::vertical, real_height_start, real_height_end)) { std::cout << "Load font failed. Can't find height marker #2" << std::endl;  return; }


      // calculate horizontal markers
      int search_start = 0, total_width = 0;
      {
            int32 cs, ce;
            //        std::vector<char_info_t> tmp_info;
            while (png_file.get_marker(search_start, 1, searchdir::horizontal, cs, ce)) {
                  search_start = ce + 1;
                  count++;
                  total_width += ce - cs + 1;
            }
      }


      pixdata = new uint32[total_width * real_height()];
      search_start = 0;

      // copy pixel data to work array
      int32 c,  x, y;
      puint32 src_ptr_init, src_ptr, dest_ptr = pixdata;
      char_info_s ch_info;

      for (c = 0; c < count; c++)
      {
            // get char horizontal bounds
            png_file.get_marker(search_start, 0, searchdir::horizontal, ch_info.width_start, ch_info.width_end);
            png_file.get_marker(search_start, 1, searchdir::horizontal, ch_info.real_width_start, ch_info.real_width_end);
            ch_info.data = dest_ptr;
            char_info.push_back(ch_info);

            src_ptr_init = png_file.data() + real_height_start * png_file.width() + ch_info.real_width_start;
            for (y = 0; y < real_height(); y++)
            {
                  src_ptr = src_ptr_init;
                  for (x = 0; x < ch_info.real_width(); x++)
                  {
                        // convert ABGR -> ARGB & inverse Alpha
                        *dest_ptr =
                              ((*src_ptr & 0xFF00FF00) ^ 0xFF000000) | // preserve Alpha and Green
                              ((*src_ptr & 0xFF) << 16) | // move Red 
                              ((*src_ptr & 0xFF0000) >> 16); // move Blue

                        src_ptr++;
                        dest_ptr++;
                  }
                  src_ptr_init += png_file.width();
            }
            search_start =  ch_info.real_width_end+1;
      }
};
font::~font() {
      if (pixdata)
            delete[] pixdata;
};