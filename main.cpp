#include <cstdio>
#include <iostream>
//#include <mysql.h>
#include <mysql.h>
#include "mydefs.h"
#include "pixfont.h"
#include "video.h"
//sql::Connection * mysql_con = NULL;

MYSQL * conn;

bitmap_font * small_font;

int min_fit,circle_radius;

using namespace std;


const int max_zoom_index = 9;
const int ZOOM_RANGE[max_zoom_index] = { 50, 100, 200, 500, 1000, 2000, 3000, 5000,10000 };
int zoom_index = 4;
double  zoom;
void zoom_changed(int new_zoom_index)
{

      if (new_zoom_index < 0)
            new_zoom_index = 0;
      else if (new_zoom_index >= max_zoom_index)
            new_zoom_index = max_zoom_index - 1;
      zoom_index = new_zoom_index;
      zoom = (double)min_fit / ZOOM_RANGE[zoom_index] / 2;
}
int video_init() {
      cout << "using PixelToaster as render system\n";
      
      /*
      if (!pix_screen.open("AIS", VIEW_WIDTH, VIEW_HEIGHT, PixelToaster::Output::Default, PixelToaster::Mode::TrueColor))
            return 1;
      return 0;
      */

      small_font = new bitmap_font("C:\\ais\\cpp\\include\\font.bmp");
      VIEWBOX_RECT.assign_pos(-CENTER_X, -CENTER_Y, CENTER_X - 1, CENTER_Y - 1);
      SCREEN_RECT.assign_pos(0, 0, VIEW_WIDTH - 1, VIEW_HEIGHT - 1);
      min_fit = imin(
            imin(CENTER_X, VIEW_WIDTH - CENTER_X),
            imin(CENTER_Y, VIEW_HEIGHT - CENTER_Y)
      ) - 20;
      circle_radius = min_fit / 5;


      zoom_changed(zoom_index);
}
int main()
{
      try {
            conn = connect_db("127.0.0.1", "map_reader", "map_reader", "ais");
            video_init();
      }
      catch (char * e) {
            cerr << "[EXCEPTION] " << e << endl;
            return false;
      }

      return 0;
}