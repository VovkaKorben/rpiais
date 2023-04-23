#include <vector>
#include "pixfont.h"


#pragma pack(2)
struct BMPFileHeader {
      uint16 file_type{ 0x4D42 };          // File type always BM which is 0x4D42
      uint32 file_size{ 0 };               // Size of the file (in bytes)
      uint16 reserved1{ 0 };               // Reserved, always 0
      uint16 reserved2{ 0 };               // Reserved, always 0
      uint32 offset_data{ 0 };             // Start position of pixel data (bytes from the beginning of the file)
};
struct BMPInfoHeader {
      uint32 size{ 0 };                      // Size of this header (in bytes)
      int32 width{ 0 };                      // width of bitmap in pixels
      int32 height{ 0 };                     // width of bitmap in pixels
      //       (if positive, bottom-up, with origin in lower left corner)
            //       (if negative, top-down, with origin in upper left corner)
      uint16 planes{ 1 };                    // No. of planes for the target device, this is always 1
      uint16 bit_count{ 0 };                 // No. of bits per pixel
      uint32 compression{ 0 };               // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
      uint32 size_image{ 0 };                // 0 - for uncompressed images
};

#pragma pack() 

class bmp_image
{
private:
      bool loaded;
      BMPInfoHeader BMPINFO;
      BMPFileHeader BMPFILE;
      uint8 * data;
      int32 w, h, scanlinelen;
public:
      int32 getw() { return BMPINFO.width; };
      int32 geth() { return BMPINFO.height; };
      int32 getsl() { return scanlinelen; };
      bool is_loaded() { return loaded; };
      uint8 pix(int x, int y)
      {
            return data[y * scanlinelen + x];
      }
      bmp_image(const char * filename)
      {
            loaded = false;
            std::ifstream inp{ filename, std::ios_base::binary };
            if (!inp)
                  return;
            inp.unsetf(std::ios::skipws);
            inp.read((char *)&BMPFILE, sizeof(BMPFILE));
            if (BMPFILE.file_type != 0x4D42) {
                  throw std::runtime_error("Error! Unrecognized file format.");
            }

            inp.read((char *)&BMPINFO, sizeof(BMPINFO));
            if (BMPINFO.bit_count != 8 || BMPINFO.planes != 1)
                  return;

            scanlinelen = BMPINFO.width;
            int pad = scanlinelen % 4;
            if (pad)
                  scanlinelen += 4 - pad;


            //std::cerr << "Error! The file \"" << filename << "\" does not seem to contain bit mask information\n";
            //throw std::runtime_error("Error! Unrecognized file format.");
            // Jump to the pixel data location
            inp.seekg(BMPFILE.offset_data, inp.beg);

            data = new uint8[scanlinelen * BMPINFO.height];
            inp.read((char *)data, scanlinelen * BMPINFO.height);
            loaded = true;
      }
};



bitmap_font::bitmap_font(const char * filename)
{
      //int32 w, h;
      //uint8 * data;      int32 * start_lut, * width_lut;
      bmp_image bmp(filename);
      if (!bmp.is_loaded())
            return;

      int32 x = 0;
      uint8 p, prev = 0;
      count = 0;
      vector<int32> pix_start;
      while (x < bmp.getw())
      {
            p = bmp.pix(x, bmp.geth() - 1);
            if (p != prev)// signal changed
            {
                  if (p) {  //data started
                        pix_start.push_back(x);
                  }
                  else
                  {   // data ended
                        width_lut.push_back(x - pix_start[count]);
                        count++;
                  }
            }
            prev = p;
            x++;
      }

      height = bmp.geth() - 2;
      int total_width = pix_start[count - 1] + width_lut[count - 1];
      pixdata = new uint8[total_width * height];
      puint8 pix_ptr = pixdata;
      start_lut.reserve(count);
      for (int i = 0; i < count; i++)
      {
            start_lut.push_back( pix_ptr);
            for (int y = bmp.geth() - 3; y >= 0; y--)
                  for (int x = 0; x < width_lut[i]; x++)
                  {
                        *pix_ptr = bmp.pix(pix_start[i] + x, y);

                        pix_ptr++;
                  }
            // start_lut[i] = store_pos;
      }


};


