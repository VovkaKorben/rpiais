#define map_show 1
#define vessels_show 1

#include <cstdio>
#include <iostream>
#include "mydefs.h"
#include "pixfont.h"
#include "video.h"
#include "nmea.h"
#include "db.h"

#if map_show==1
const uint64_t SHAPES_UPDATE = 5 * 60 * 1000;  // 5 min for update shapes
const uint64_t SHAPES_UPDATE_FIRST = 3 * 1000;  // 3 sec for startup updates
uint64_t next_map_update;
#endif



int min_fit, circle_radius;

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
////////////////////////////////////////////////////////////
void draw_vessels(const BGRA fill_color, const BGRA outline_color)
{
      irect test_box;
      vessel * v;
      floatpt vessel_center, fp;
      intpt int_center;
      for (size_t i = 0; i < vessels.items.size(); i++)
      {
            v = &vessels.items[i];
            if ((v->flags & VESSEL_XY) == VESSEL_XY)
            {
                  // calculate vessel center (with zoom and rotation)
                  vessel_center = { v->lon,v->lat };
                  vessel_center.latlon2meter();
                  vessel_center.offset_remove(mypos);
                  vessel_center.to_polar();
                  vessel_center.y *= zoom;
                  if (relative)
                        vessel_center.x -= my_angle;
                  vessel_center.to_decart();
                  int_center = vessel_center.to_int();
                  int_center.offset_add(CENTER_X, CENTER_Y);

                  // rotate each point
                  for (int p = 0; p < v->points_count; p++)
                  {
                        fp = v->origin[p];
                        fp.to_polar();
                        fp.y *= zoom;

                        // only if size and angle known - rotate
                        if ((v->flags & VESSEL_LRTBA) == VESSEL_LRTBA)
                        {
                              fp.x -= v->course;
                              if (relative)
                                    fp.x -= my_angle;
                        }
                        fp.to_decart();
                        v->work[p] = fp.to_int();
                        v->work[p].offset_add(int_center);


                        if (p == 0)
                              test_box.assign_pos(v->work[p].x, v->work[p].y);
                        else
                        {
                              test_box.x1 = imin(test_box.x1, v->work[p].x);
                              test_box.y1 = imin(test_box.y1, v->work[p].y);
                              test_box.x2 = imax(test_box.x2, v->work[p].x);
                              test_box.y2 = imax(test_box.y2, v->work[p].y);
                        }

                  }
                  if (test_box.is_intersect(SCREEN_RECT))
                  {
                        draw_shape(v, fill_color, outline_color);
                        draw_text(small_font, test_box.hcenter(), test_box.vcenter(), string_format("%d", v->mmsi), 0x000000, HALIGN_CENTER | VALIGN_CENTER);
                        //cout << i << " accepted" << "\n";
                  }

            }





      }
}
void draw_shapes(const BGRA fill_color, const BGRA outline_color)
{

      //rotate box and get new bounds
      irect new_box;
      for (myshape shape : shapes)
      {
            if (fill_color != NO_COLOR || outline_color != NO_COLOR) {
                  //calculate new transormed corners
                  new_box = rotate_shape_box(&shape);
                  if (new_box.is_intersect(SCREEN_RECT))
                  {
                        // check if full rotate geometry still visible
                        new_box = rotate_shape(&shape);
                        if (new_box.is_intersect(SCREEN_RECT))
                              draw_shape(&shape, fill_color, outline_color);
                  }
            }
      }
}
void draw_grid(double angle)
{

      floatpt fp;
      const string sides = "WSEN";
      for (int s = 0; s < 4; s++)
      {
            fp.x = -angle;
            fp.y = min_fit;
            fp.to_decart();
            draw_line(CENTER_X, CENTER_Y, CENTER_X + fp.x, CENTER_Y + fp.y, GRID_COLOR);

            fp.x = -angle;
            fp.y = min_fit + 9;
            fp.to_decart();
            draw_text(small_font, CENTER_X + fp.x, CENTER_Y + fp.y, sides.substr(s, 1), 0x88000000, HALIGN_CENTER | VALIGN_CENTER);

            angle += 90.0;
            if (angle >= 360.0) angle -= 360.0;
      }
      int v;
      string s;
      for (int i = 1; i < 6; i++)
      {
            draw_circle(CENTER_X, CENTER_Y, i * circle_radius, 0x22000000);
            fp.x = -angle;
            fp.y = circle_radius * i;
            fp.to_decart();
            v = ZOOM_RANGE[zoom_index] / 5 * i;
            if (v >= 1000)  // make large numbers more compact
                  s = string_format("%d.%d", v / 1000, v % 1000);
            else
                  s = string_format("%d", v);
            draw_text(small_font, CENTER_X + fp.x, CENTER_Y + fp.y, s, 0x220033, HALIGN_CENTER | VALIGN_CENTER);
      }

}
void draw_screen()
{
      video_frame_start(0x0097c2);

      vessel * v = vessels.get_own();


      last_msg_id = update_nmea(last_msg_id);
      if ((v->flags & VESSEL_XY) == VESSEL_XY)
      {

            mypos.x = v->lon;
            mypos.y = v->lat;
            mypos.latlon2meter();
            // offset MYPOS from gps center to geometrical boat center


#if map_show==1
            next_map_update = update_map_data(mysql_con, next_map_update);
            draw_shapes(0xb7b074, 0x16150e);
#endif
#if vessels_show==1
            draw_vessels(0xcd8183, 0x000000);
#endif
      }

      uint64_t currentTime = utc_ms();
      nbFrames++;
      uint64_t xxx = currentTime - last_time;
      if (currentTime - last_time >= FPS_INTERVAL) { // If last prinf() was more than 1 sec ago
            // printf and reset timer
            fps = string_format("Frame: %.3fms", FPS_INTERVAL / double(nbFrames));
            nbFrames = 0;
            last_time += FPS_INTERVAL;
      }


      draw_grid(relative ? my_angle : 0.0);

      draw_text(small_font, 5, VIEW_HEIGHT - 10, string_format("Angle: %.2f", my_angle), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 20, string_format("Pos: %.2f x %.2f", mypos.x, mypos.y), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 30, string_format("[ZOOM] ind: %d value: %d coeff: %.5f%", zoom_index, ZOOM_RANGE[zoom_index], zoom), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 40, string_format("Mouse: %d x %d", mouse_pos.x, mouse_pos.y), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 50, fps, 0x000000);


      video_frame_end();
      pix_screen.update(pixels);

}
////////////////////////////////////////////////////////////
void video_loop_start() {
      while (pix_screen.open())
      {
            draw_screen();
      }
}
////////////////////////////////////////////////////////////
int main()
{
      try {
            if (init_db("127.0.0.1", "map_reader", "map_reader", "ais"))
            {
                  printf("mysql init error.\n");
                  return 1;

            }


            if (video_init(120, 120))
            {
                  printf("video_init error.\n");
                  return 1;
            }


            min_fit = imin(
                  imin(CENTER_X, VIEW_WIDTH - CENTER_X),
                  imin(CENTER_Y, VIEW_HEIGHT - CENTER_Y)
            ) - 20;
            circle_radius = min_fit / 5;

            zoom_changed(zoom_index);
#if map_show==1
            next_map_update = utc_ms() - 1;
#else
            cout << "Map draw disabled (#define map_show 0)" << endl;
#endif
            
            
            
            return 0;
            //struct timespec t;            clock_start(t);
           //while (true) {
            for (int y = 0; y < VIEW_HEIGHT; y++)
                  for (int x = 0; x < VIEW_WIDTH; x++)
                        draw_pix(x, y, 0xee8d4500);
            draw_text(small_font, 10, 10, "TEST STRING", 0x00000000, VALIGN_BOTTOM | HALIGN_LEFT);


            //clock_end(t);

      }
      catch (char * e) {
            cerr << "[EXCEPTION] " << e << endl;
            return false;
      }

      return 0;
}