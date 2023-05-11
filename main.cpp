#define map_show 1
#define vessels_show 1

#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <unistd.h>
//#include <limits.h>
#include <cfloat>
#include "mydefs.h"
#include "pixfont.h"
#include "video.h"
#include "nmea.h"
#include "db.h"
#include "inputs.h"

#if map_show==1
const uint64_t SHAPES_UPDATE = 5 * 60 * 1000;  // 5 min for update shapes
const uint64_t SHAPES_UPDATE_FIRST = 3 * 1000;  // 3 sec for startup updates
uint64_t next_map_update;
#endif

int min_fit, circle_radius;
image * img_minus, * img_plus;
int last_msg_id = 1;// 111203 - 1;
std::map <int, poly>  shapes;

//using namespace std;
video_driver * screen;
mysql_driver * mysql;


const int max_zoom_index = 9;
const int ZOOM_RANGE[max_zoom_index] = { 50, 75,200,300, 1000, 2000, 3000, 5000,10000 };
int zoom_index = 2;
double  zoom;
int update_nmea(int msg_id) {
      int nmea_result;
      mysql->exec_prepared(PREPARED_NMEA, msg_id);
      mysql->store();
      int total = 0;
      while (mysql->fetch())
      {
            msg_id = mysql->get_myint("id");
            nmea_result = parse_nmea(mysql->get_mystr("data"));
            if (nmea_result)
                  printf("parse_nmea returns %d\n", nmea_result);
            total++;
      }
      if (total)
            printf("NMEA processed: %d\n", total);

      return msg_id;
}
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
intpt transform_point(floatpt pt)
{
      pt.offset_remove(own_vessel.get_meters());

      pt.to_polar();
      if (own_vessel.get_relative())
            pt.x -= own_vessel.get_heading();
      pt.y *= zoom;
      pt.to_decart();
      int x = int(round(pt.x)) + CENTER_X,
            y = int(round(pt.y)) + CENTER_Y;
      //y = invertYaxis ? CENTER_Y - y - 1 : y + CENTER_Y;
      return intpt{ x,y };
}
irect rotate_shape_box(poly * shape)
{
      irect rct;
      floatpt fpt;
      intpt ipt;
      int corner = 3, first = 1;
      while (corner >= 0)
      {
            fpt = shape->bounds.get_corner(corner);
            ipt = transform_point(fpt);
            if (first)
            {
                  rct.assign_pos(ipt.x, ipt.y);
                  first = 0;
            }
            else
            {
                  rct.x1 = imin(rct.x1, ipt.x);
                  rct.y1 = imin(rct.y1, ipt.y);
                  rct.x2 = imax(rct.x2, ipt.x);
                  rct.y2 = imax(rct.y2, ipt.y);
            }
            corner--;
      }

      return rct;
}
irect rotate_shape(poly * shape)
{
      irect rct;
      int first = 1;
      //rct.assign_pos(0.0, 0.0);

      for (int pt_index = 0; pt_index < shape->points_count; pt_index++)
      {
            shape->work[pt_index] = transform_point(shape->origin[pt_index]);
            if (first)
            {
                  rct.assign_pos(shape->work[pt_index].x, shape->work[pt_index].y);
                  first = 0;
            }
            else {
                  rct.x1 = imin(rct.x1, shape->work[pt_index].x);
                  rct.y1 = imin(rct.y1, shape->work[pt_index].y);
                  rct.x2 = imax(rct.x2, shape->work[pt_index].x);
                  rct.y2 = imax(rct.y2, shape->work[pt_index].y);
            }
      }
      return rct;

}
////////////////////////////////////////////////////////////
#if map_show==1

int load_shapes()
{

      //MYSQL_RES * res;      MYSQL_ROW row;


      //double overlap_coeff = 1.05 * ZOOM_RANGE[max_zoom_index - 1];
      double overlap_coeff = 3.0;// 0.05 * ZOOM_RANGE[max_zoom_index - 1];
      int shapes_total = 0, points_total = 0;

      // create string with existing id's
      std::stringstream sstr;

      if (shapes.size()) {
            int total = 0;
            sstr << " and recid not in(";
            for (std::map<int, poly>::iterator iter = shapes.begin(); iter != shapes.end(); ++iter)
            {
                  if (total++ != 0)
                        sstr << ",";
                  sstr << iter->first;

            }
            sstr << ")";

            //  sstr.seekg(0, ios::end);            int size = sstr.tellg();

      }
      const std::string & tmp = sstr.str();
      const char * cstr = tmp.c_str();
      mysql->exec_prepared(PREPARED_MAP1,
                           own_vessel.get_meters().x + VIEWBOX_RECT.x1 * overlap_coeff,
                           own_vessel.get_meters().y + VIEWBOX_RECT.y1 * overlap_coeff,
                           own_vessel.get_meters().x + VIEWBOX_RECT.x2 * overlap_coeff,
                           own_vessel.get_meters().y + VIEWBOX_RECT.y2 * overlap_coeff,
                           cstr);
      mysql->free_result();
      while (mysql->has_next());

      mysql->exec_prepared(PREPARED_MAP2);
      mysql->store();
      // fetching shapes
      poly shape;
      int recid;
      while (mysql->fetch())
      {


            shape.bounds.x1 = mysql->get_myfloat("minx");
            shape.bounds.y1 = mysql->get_myfloat("miny");
            shape.bounds.x2 = mysql->get_myfloat("maxx");
            shape.bounds.y2 = mysql->get_myfloat("maxy");


            shape.path_count = mysql->get_myint("parts");
            shape.pathindex = new int[shape.path_count + 1];
            shape.points_count = mysql->get_myint("points");
            shape.pathindex[shape.path_count] = shape.points_count; // set closing point
            shape.origin = new floatpt[shape.points_count];
            shape.work = new intpt[shape.points_count];

            recid = mysql->get_myint("recid");
            shapes[recid] = shape;

            shapes_total++;
      }

      mysql->free_result();
      if (!mysql->has_next())
            return 1;
      mysql->store();

      // read points
      int prev_recid = -1;
      int partid, prev_partid;
      int curr_path_index, curr_point_index;

      //poly * sh = nullptr;
      while (mysql->fetch())
      {

            recid = mysql->get_myint("recid");
            if (recid != prev_recid)
            {
                  prev_recid = recid;
                  //sh = &shapes[recid];
                  prev_partid = -1;
                  curr_point_index = 0;
                  curr_path_index = 0;
            }

            partid = mysql->get_myint("partid");
            if (partid != prev_partid)
            {
                  // add path start
                  prev_partid = partid;
                  shapes[recid].pathindex[curr_path_index++] = curr_point_index;
            }
            shapes[recid].origin[curr_point_index++] = {
                  mysql->get_myfloat("x"),
                  mysql->get_myfloat("y")
            };

            points_total++;
      }

      mysql->free_result();
      mysql->has_next();

      std::cout << "Shapes loaded: " << shapes_total << std::endl;
      std::cout << "Points added: " << points_total << std::endl;
      return 0;
}
uint64_t  update_map_data(uint64_t next_check)
{
      uint64_t ms = utc_ms();
      if (ms > next_check)
      {
            load_shapes();
            //garbage_shapes(shapes)
            next_check += SHAPES_UPDATE;
      }
      else
            next_check += SHAPES_UPDATE_FIRST;


      return next_check;


}
#endif
////////////////////////////////////////////////////////////
void draw_vessels(const ARGB fill_color, const ARGB outline_color)
{
      irect test_box;
      const vessel * v;
      floatpt vessel_center, fp;
      intpt int_center;
      int circle_size = std::max(int(6 * zoom), 3);
      for (const auto & vx : vessels)
      {
            //printf("lat lon: %.12f,%.12f\n", v.lat, v.lon);
           // printf("mmsi lat lon: %d %.12f,%.12f\n",v.first,  v.lat, v.lon);
            v = &vx.second;
            if (v->pos_ok)
            {

                  // calculate vessel center (with zoom and rotation)

                  vessel_center = v->gps;
                  vessel_center.latlon2meter();
                  vessel_center.offset_remove(own_vessel.get_meters());
                  vessel_center.to_polar();
                  vessel_center.y *= zoom;
                  if (own_vessel.get_relative())
                        vessel_center.x -= own_vessel.get_heading();
                  vessel_center.to_decart();
                  int_center = vessel_center.to_int();
                  int_center.offset_add(CENTER_X, CENTER_Y);
                  if (v->size_ok && v->angle_ok)
                  {
                        // rotate each point
                        for (int p = 0; p < v->points_count; p++)
                        {
                              fp = v->origin[p];
                              fp.to_polar();
                              fp.y *= zoom;

                              // only if size and angle known - rotate


                              {
                                    fp.x -= v->course;
                                    if (own_vessel.get_relative())
                                          fp.x -= own_vessel.get_heading();
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
                              screen->draw_shape(v, fill_color, outline_color);
                              //screen->draw_text(small_font, test_box.hcenter(), test_box.vcenter(), string_format("%d", v->mmsi), 0x000000, HALIGN_CENTER | VALIGN_CENTER);
                              //cout << i << " accepted" << "\n";
                        }
                  }
                  else
                  { //unknown size or angle, just draw small circle with position
                        screen->draw_circle(int_center.x, int_center.y, circle_size, 0x00000000, 0x0000FF00);
                  }

            }





      }
}
void draw_shapes(const ARGB fill_color, const ARGB outline_color)
{

      //rotate box and get new bounds
      irect new_box;
      for (auto & shape : shapes)
            //auto const & ent1 : mymap
            //for (poly shape : shapes)
      {
            if (fill_color != NO_COLOR || outline_color != NO_COLOR) {
                  //calculate new transormed corners
                  new_box = rotate_shape_box(&shape.second);
                  if (new_box.is_intersect(SCREEN_RECT))
                  {
                        // check if full rotate geometry still visible
                        new_box = rotate_shape(&shape.second);
                        if (new_box.is_intersect(SCREEN_RECT))
                              screen->draw_shape(&shape.second, fill_color, outline_color);
                  }
            }
      }

}
void draw_grid(double angle)
{

      floatpt fp;
      //const string sides = "ESWN";
      const std::string sides = " S N";
      for (int s = 0; s < 4; s++)
      {
            // axis
            fp.x = -angle;
            fp.y = min_fit;
            fp.to_decart();
            screen->draw_line(CENTER_X, CENTER_Y, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), 0x10000000);

            // axis letters
            fp.x = -angle;
            fp.y = min_fit + 9;
            fp.to_decart();
            screen->draw_text(SPECCY_FONT, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), sides.substr(s, 1), 0x00000000, HALIGN_CENTER | VALIGN_CENTER);

            angle += 90.0;
            if (angle >= 360.0) angle -= 360.0;
      }
      int v;
      std::string s;
      for (int i = 1; i < 6; i++)
      {
            // distance circle
            screen->draw_circle(CENTER_X, CENTER_Y, i * circle_radius, 0x10000000, NO_COLOR);

            // distance marks
            fp.x = -angle;
            fp.y = circle_radius * i;
            fp.to_decart();
            v = ZOOM_RANGE[zoom_index] / 5 * i;
            if (v >= 1000)  // make large numbers more compact
                  s = string_format("%d.%d", v / 1000, v % 1000);
            else
                  s = string_format("%d", v);
            screen->draw_text(SPECCY_FONT, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), s, 0x0000FF, HALIGN_CENTER | VALIGN_CENTER);
      }

}

struct less_than_key
{
      inline bool operator() (const int & mmsi0, const int & mmsi1)
      {
            double d0 = vessels[mmsi0].distance, d1 = vessels[mmsi1].distance;
            if (std::isnan(d0)) return false;
            else if (std::isnan(d1)) return true;
            else return d0 < d1;

      }
};
void draw_vessels_info() {
      int left = CENTER_X * 2;
      //w = screen->get_width() / 3;
      screen->draw_fill_rect(left, 0, screen->get_width(), screen->get_height(), 0xA0FFFFFF);

      // calculate distance + get all MMSI to vector
      std::vector<int> mmsi;
      for (std::map<int, vessel>::iterator it = vessels.begin(); it != vessels.end(); ++it) {

            if (it->second.pos_ok)
                  it->second.distance = it->second.gps.haversine(own_vessel.get_gps());

            else
                  it->second.distance = NAN;

            mmsi.push_back(it->first);
      }

      std::sort(mmsi.begin(), mmsi.end(), less_than_key());
      int lines_count = std::min((int)mmsi.size(), 10);
      int y_coord;
      for (int i = 0; i < lines_count; i++)
      {
            y_coord = screen->get_height() - 5 - i * 10;
            
            screen->draw_text(SPECCY_FONT, left + 5, y_coord, string_format("%d", mmsi[i]), 0x00000000, VALIGN_TOP | HALIGN_LEFT);
            screen->draw_text(SPECCY_FONT, screen->get_width()-5, y_coord, string_format("%d", vessels[mmsi[i]].distance), 0x00000000, VALIGN_TOP | HALIGN_RIGHT);
      }

      //for (int y = 5; y < screen->get_height(); y += 10)

}
void draw_frame()
{



      if (!own_vessel.pos_ok())
      {
            // print `no GPS` and exit
            screen->draw_bg(0x001111);
            screen->draw_text(SPECCY_FONT, screen->get_width() / 2, screen->get_height() / 2, "no GPS position", 0xFFFF00, VALIGN_CENTER | HALIGN_CENTER);
            return;
      }

      screen->draw_bg(0x0097c2);
      //screen->draw_text(SPECCY_FONT, screen->get_width() / 2, 25, "HAS GPS !!!", 0xFFFF00, VALIGN_CENTER | HALIGN_CENTER);
      //floatpt mypos = own_vessel.get_pos();
      //mypos.latlon2meter();
      // offset MYPOS from gps center to geometrical boat center


#if map_show==1
      next_map_update = update_map_data(next_map_update);
      draw_shapes(0xb7b074, 0x16150e);
#endif
#if vessels_show==1
      draw_vessels(0xcd8183, 0x000000);
#endif
      //
            /*
            uint64_t currentTime = utc_ms();
            nbFrames++;
            uint64_t xxx = currentTime - last_time;
            if (currentTime - last_time >= FPS_INTERVAL) { // If last prinf() was more than 1 sec ago
                  // printf and reset timer
                  fps = string_format("Frame: %.3fms", FPS_INTERVAL / double(nbFrames));
                  nbFrames = 0;
                  last_time += FPS_INTERVAL;
            }
            */


      draw_grid(own_vessel.get_relative() ? own_vessel.get_heading() : 0.0);
      draw_vessels_info();

      /*
      draw_text(small_font, 5, VIEW_HEIGHT - 10, string_format("Angle: %.2f", my_angle), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 20, string_format("Pos: %.2f x %.2f", mypos.x, mypos.y), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 30, string_format("[ZOOM] ind: %d value: %d coeff: %.5f%", zoom_index, ZOOM_RANGE[zoom_index], zoom), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 40, string_format("Mouse: %d x %d", mouse_pos.x, mouse_pos.y), 0x000000);
      draw_text(small_font, 5, VIEW_HEIGHT - 50, fps, 0x000000);
      */
      screen->draw_image(img_minus, 0, screen->get_height() - 1, HALIGN_LEFT | VALIGN_TOP);
      screen->draw_image(img_plus, screen->get_width() / 3 * 2, screen->get_height() - 1, HALIGN_RIGHT | VALIGN_TOP);
     /* screen->draw_image(img_test,20, screen->get_height() - 10, HALIGN_LEFT | VALIGN_TOP);
      screen->draw_image(img_test, 100, screen->get_height() - 100, HALIGN_LEFT | VALIGN_TOP,140);
      screen->draw_image(img_test, 130, screen->get_height() - 30, HALIGN_LEFT | VALIGN_TOP,70);*/
      
}
////////////////////////////////////////////////////////////

void parse_input(void)
{
      //---------------------------------------------
      //----- CHECK FIFO FOR ANY RECEIVED BYTES -----
      //---------------------------------------------
      // Read up to 255 characters from the port if they are there
      if (our_input_fifo_filestream != -1)
      {
            unsigned int  InputBuffer[256];
            ssize_t BufferLength = read(our_input_fifo_filestream, (void *)InputBuffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
            if (BufferLength < 0)
            {
                  //An error occured (this can happen)
            }
            else if (BufferLength == 0)
            {
                  //No data waiting
            }
            else
            {
                  // for (ssize_t i = 0; i < BufferLength / (ssize_t)EV_SIZE; i++)                        screen->draw_text(SPECCY_FONT, 5, 10 * i, string_format(" %.08X", InputBuffer[i]), 0x00AA0000, VALIGN_BOTTOM | HALIGN_LEFT);
                   //printf("event: %.08X\n", InputBuffer[i]);
            }
      }
}
inline unsigned long long tm_diff(timespec c, timespec p)
{
      unsigned long long s = c.tv_sec - p.tv_sec, n = c.tv_nsec - p.tv_nsec;
      return s * 1000000 + n / 1000;

}
void video_loop_start() {

      const int FPS = 60;
      unsigned long long ns_delay = 1000000000L / FPS, diff, frames_total = 0;
      struct timespec frame_start, frame_end, measure_start;

      struct timespec sleep_timer = { 0,0 }, sleep_dummy;




      int frames = 0;
      std::string fps_str = "";

      clock_gettime(CLOCK_REALTIME, &measure_start);
      while (true)
      {


            clock_gettime(CLOCK_REALTIME, &frame_start);
            last_msg_id = update_nmea(last_msg_id);
            draw_frame();
            parse_input();


            // df = timediff(pt, ct);
            frames++;
            clock_gettime(CLOCK_REALTIME, &frame_end);
            diff = (frame_end.tv_sec - frame_start.tv_sec) * 1000000000L + frame_end.tv_nsec - frame_start.tv_nsec;
            frames_total += diff;

            if (ns_delay > diff)
            {
                  sleep_timer.tv_nsec = ns_delay - diff;
                  nanosleep(&sleep_timer, &sleep_dummy);
            }

            //diff = tm_diff(curr, prev);
            if (frame_end.tv_sec > measure_start.tv_sec)
            {
                  diff = (frame_end.tv_sec - measure_start.tv_sec) * 1000000000L + frame_end.tv_nsec - measure_start.tv_nsec;

                  // printf("diff:%-13ld\tframes:%-5d\tframes_total:%-12ld\t%.10f\n", diff, frames, frames_total, frames_total/ (double)frames);
                 // fps_str = string_format("Frame: %.3f ms", frames_total / 1000000.0 / frames);
                  fps_str = "none";
                  frames = 0;
                  measure_start = frame_end;
                  frames_total = 0;
            }
            //screen->draw_text(SPECCY_FONT, 5, VIEW_HEIGHT - 5, fps_str, 0xFFFF00, VALIGN_TOP | HALIGN_LEFT);
            screen->draw_text(SPECCY_FONT, 5, 5, fps_str, 0xFFFF00, VALIGN_BOTTOM | HALIGN_LEFT);
            // switch page

            screen->flip();

      }
}
////////////////////////////////////////////////////////////
int main()
{
      /* floatpt fp1 = { 24.9532280834854000,60.1674667786688000 },
             fp2 = { 24.955862012736600,60.167291983683600 };
       fp1.latlon2meter();
       fp2.latlon2meter();
       double d = fp1.distance(fp2);
       */

      try {
            mysql = new mysql_driver("127.0.0.1", "map_reader", "map_reader", "ais");
            if (mysql->get_last_error_str()) {
                  printf("mysql init error.\n%s\n", mysql->get_last_error_str());
                  return 1;

            }
            init_db(mysql);
            //load_dicts(mysql);

            screen = new video_driver(480, 320,5); // debug purpose = 1 buffer, production value = 5
            if (screen->get_last_error())
            {
                  printf("video_init error.\n");
                  return 1;
            }
            screen->load_font(NORMAL_FONT, "/home/pi/projects/rpiais/font.bmp");
            screen->load_font(SPECCY_FONT, "/home/pi/projects/rpiais/speccy.bmp");
            img_minus = new image("/home/pi/projects/rpiais/img/minus.png");
            img_plus = new image("/home/pi/projects/rpiais/img/plus.png");
           // img_test = new image("/home/pi/projects/rpiais/img/test3.png");

            min_fit = imin(
                  imin(CENTER_X, screen->get_width() - CENTER_X),
                  imin(CENTER_Y, screen->get_height() - CENTER_Y)
            ) - 20;
            circle_radius = min_fit / 5;
            if (KeyboardMonitorInitialise())
            {
                  //  printf("input init error.\n");
                   // return 1;
            }
            zoom_changed(zoom_index);
#if map_show==1
            next_map_update = utc_ms() - 1;
#else
            std::cout << "Map draw disabled (#define map_show 0)" << std::endl;
#endif
            video_loop_start();
            return 0;
      }
      catch (char * e) {
            std::cerr << "[EXCEPTION] " << e << std::endl;
            return false;
      }
      return 0;
}