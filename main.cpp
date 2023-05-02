#define map_show 1
#define vessels_show 1

#include <cstdio>
#include <cstring>
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
int last_msg_id = 111203 - 1;
map <int, myshape>  shapes;

//using namespace std;
video_driver * screen;
mysql_driver * mysql;


const int max_zoom_index = 9;
const int ZOOM_RANGE[max_zoom_index] = { 50, 100, 200, 500, 1000, 2000, 3000, 5000,10000 };
int zoom_index = 4;
double  zoom;
int update_nmea(int last_msg_id) {
      int nmea_result;
      mysql->exec_prepared(PREPARED_NMEA, last_msg_id);
      mysql->store();
      while (mysql->fetch())
      {
            last_msg_id = mysql->get_myint("id");
            nmea_result = parse_nmea(mysql->get_mystr("data"));
            if (nmea_result)
                  printf("parse_nmea returns %d\n", nmea_result);
      }
      return last_msg_id;
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
      pt.offset_remove(own_vessel.pos);

      pt.to_polar();
      if (own_vessel.relative)
            pt.x -= own_vessel.heading;
      pt.y *= zoom;
      pt.to_decart();
      int x = int(round(pt.x)) + CENTER_X,
            y = int(round(pt.y)) + CENTER_Y;
      //y = invertYaxis ? CENTER_Y - y - 1 : y + CENTER_Y;
      return intpt{ x,y };
}
irect rotate_shape_box(myshape * shape)
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
irect rotate_shape(myshape * shape)
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

      MYSQL_RES * res;
      MYSQL_ROW row;


      //double overlap_coeff = 1.05 * ZOOM_RANGE[max_zoom_index - 1];
      double overlap_coeff = 3.0;// 0.05 * ZOOM_RANGE[max_zoom_index - 1];
      int shapes_total = 0, points_total = 0;

      // create string with existing id's
      stringstream sid;
      if (shapes.size()) {
            int total = 0;
            sid << " and recid not in(";
            for (std::map<int, myshape>::iterator iter = shapes.begin(); iter != shapes.end(); ++iter)
            {
                  if (total != 0)
                        sid << ",";
                  sid << iter->first;
                  total++;
            }
            sid << ")";
      }

      mysql->exec_prepared(PREPARED_MAP,
                                     own_vessel.pos.x + VIEWBOX_RECT.x1 * overlap_coeff,
                                     own_vessel.pos.y + VIEWBOX_RECT.y1 * overlap_coeff,
                                     own_vessel.pos.x + VIEWBOX_RECT.x2 * overlap_coeff,
                                     own_vessel.pos.y + VIEWBOX_RECT.y2 * overlap_coeff,
                                     sid.str());
      mysql->store();
  

      // fetching shapes
      myshape shape;
      while (mysql->fetch()) {

            {

                  shape.id = mysql->get_myint("");
                  /*  shape.bounds.x1 = res->getDouble("minx");
                    shape.bounds.y1 = res->getDouble("miny");
                    shape.bounds.x2 = res->getDouble("maxx");
                    shape.bounds.y2 = res->getDouble("maxy");


                    shape.path_count = res->getInt("parts");
                    shape.pathindex = new int[shape.path_count + 1];
                    shape.points_count = res->getInt("points");
                    shape.pathindex[shape.path_count] = shape.points_count; // set closing point
                    shape.origin = new floatpt[shape.points_count];
                    shape.work = new intpt[shape.points_count];
                    shapes.push_back(shape);
                    */
                  shapes_total++;
            }

            // read points
            int new_recid, prev_recid = -1;
            int new_partid, prev_partid = -1;
            int curr_path_index = -1, curr_point_index = -1;
            /*
                        myshape * sh = nullptr;
                        sq = read_file("C:\\ais\\cpp\\AIS\\sql\\mapread_prepareid3.sql");
                        res = stmt->executeQuery(sq);
                        while (res->next())
                        {

                              new_recid = res->getInt("recid");
                              if (new_recid != prev_recid)
                              {
                                    prev_recid = new_recid;
                                    sh = get_shape_by_id(new_recid);
                                    if (sh == NULL)
                                          throw exception("Invalid shape pointer");
                                    prev_partid = -1;
                                    curr_point_index = 0;
                                    curr_path_index = 0;
                              }

                              new_partid = res->getInt("partid");
                              if (new_partid != prev_partid)
                              {
                                    prev_partid = new_partid;
                                    sh->pathindex[curr_path_index] = curr_point_index;
                                    curr_path_index++;
                              }

                              sh->origin[curr_point_index] = {
                                    (double)(res->getDouble("x")),
                                    (double)(res->getDouble("y"))
                              };
                              curr_point_index++;
                              points_total++;
                        }



                        delete stmt;
                        delete res;
                        */
            cout << "Shapes loaded: " << shapes_total << endl;
            cout << "Points added: " << points_total << endl;
      }
}
uint64_t  update_map_data( uint64_t next_check)
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
                  vessel_center.offset_remove(own_vessel.pos);
                  vessel_center.to_polar();
                  vessel_center.y *= zoom;
                  if (own_vessel.relative)
                        vessel_center.x -= own_vessel.heading;
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
                              if (own_vessel.relative)
                                    fp.x -= own_vessel.heading;
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





      }
}
void draw_shapes(const BGRA fill_color, const BGRA outline_color)
{

      //rotate box and get new bounds
      irect new_box;
      /* for (myshape shape : shapes)
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
       */
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
            screen->draw_line(CENTER_X, CENTER_Y, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), 0x00000000);

            fp.x = -angle;
            fp.y = min_fit + 9;
            fp.to_decart();
            screen->draw_text(SPECCY_FONT, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), sides.substr(s, 1), 0x88000000, HALIGN_CENTER | VALIGN_CENTER);

            angle += 90.0;
            if (angle >= 360.0) angle -= 360.0;
      }
      int v;
      string s;
      for (int i = 1; i < 6; i++)
      {
            screen->draw_circle(CENTER_X, CENTER_Y, i * circle_radius, 0x22000000);
            fp.x = -angle;
            fp.y = circle_radius * i;
            fp.to_decart();
            v = ZOOM_RANGE[zoom_index] / 5 * i;
            if (v >= 1000)  // make large numbers more compact
                  s = string_format("%d.%d", v / 1000, v % 1000);
            else
                  s = string_format("%d", v);
            screen->draw_text(SPECCY_FONT, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), s, 0x220033, HALIGN_CENTER | VALIGN_CENTER);
      }

}
void draw_frame()
{



      if (!own_vessel.pos_ok())
      {
            // print `no GPS` and exit
            screen->draw_bg(0x001111);
            screen->draw_text(SPECCY_FONT, VIEW_WIDTH / 2, VIEW_HEIGHT / 2, "no GPS position", 0xFFFF00, VALIGN_CENTER | HALIGN_CENTER);
            return;
      }

      screen->draw_bg(0x000909);
      screen->draw_text(SPECCY_FONT, VIEW_WIDTH / 2, VIEW_HEIGHT / 2, "HAS GPS !!!", 0xFFFF00, VALIGN_CENTER | HALIGN_CENTER);
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


      // draw_grid(relative ? my_angle : 0.0);
       /*
       draw_text(small_font, 5, VIEW_HEIGHT - 10, string_format("Angle: %.2f", my_angle), 0x000000);
       draw_text(small_font, 5, VIEW_HEIGHT - 20, string_format("Pos: %.2f x %.2f", mypos.x, mypos.y), 0x000000);
       draw_text(small_font, 5, VIEW_HEIGHT - 30, string_format("[ZOOM] ind: %d value: %d coeff: %.5f%", zoom_index, ZOOM_RANGE[zoom_index], zoom), 0x000000);
       draw_text(small_font, 5, VIEW_HEIGHT - 40, string_format("Mouse: %d x %d", mouse_pos.x, mouse_pos.y), 0x000000);
       draw_text(small_font, 5, VIEW_HEIGHT - 50, fps, 0x000000);
       */


}
////////////////////////////////////////////////////////////

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
                  fps_str = string_format("Frame: %.3f ms", frames_total / 1000000.0 / frames);
                  frames = 0;
                  measure_start = frame_end;
                  frames_total = 0;
            }
            screen->draw_text(SPECCY_FONT, 5, VIEW_HEIGHT - 5, fps_str, 0xFFFF00, VALIGN_TOP | HALIGN_LEFT);
            screen->draw_text(SPECCY_FONT, 5, 5, fps_str, 0xFFFF00, VALIGN_BOTTOM | HALIGN_LEFT);
            // switch page

            screen->flip();

      }
}
////////////////////////////////////////////////////////////
int main()
{
      try {
            mysql = new mysql_driver("127.0.0.1", "map_reader", "map_reader", "ais");
            if (mysql->get_last_error_str()) {
                  printf("mysql init error.\n%s\n");
                  return 1;

            }
            load_dicts(mysql);

            screen = new video_driver(480, 320, 32,5);
            if (screen->get_last_error())
            
            {
                  printf("video_init error.\n");
                  return 1;
            }
            screen->load_font(NORMAL_FONT,"/home/pi/projects/rpiais/font.bmp");
            screen->load_font(SPECCY_FONT,"/home/pi/projects/rpiais/speccy.bmp");
            
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

            video_loop_start();

            return 0;
            //struct timespec t;            clock_start(t);


            //clock_end(t);

      }
      catch (char * e) {
            cerr << "[EXCEPTION] " << e << endl;
            return false;
      }

      return 0;
}


