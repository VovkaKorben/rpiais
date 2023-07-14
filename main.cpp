#include "mydefs.h"
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <cfloat>
#include "pixfont.h"
#include "video.h"
#include "nmea.h"
#include "db.h"
#include <limits.h>
#include <chrono>
#include "touch.h"
#include <cstdint>
//#include <bits/stdc++.h>
#include <stdlib.h>
#include "nmeastreams.h"
#include <SimpleIni.h>
#include <queue>

//const uint64 SHAPES_UPDATE = 5 * 60 * 1000;  // 5 min for update shapes
const uint64_t SHAPES_UPDATE = 10 * 1000;  // 5 min for update shapes
uint64_t next_map_update;


int32 min_fit, circle_radius;


image* img_minus, * img_plus;
const int32 ZOOM_BTN_MARGIN = 20;


std::map <int32, poly>  shapes;

int32 show_info_window = 0;
//using namespace std;
video_driver* screen;

touchscreen* touchscr;
touch_manager* touchman;
nmea_reciever* nmea_recv;

StringQueue  nmea_list;



const int max_zoom_index = 9;
const int ZOOM_RANGE[max_zoom_index] = { 50, 120,200,300, 1000, 2000, 3000, 5000,10000 };
int zoom_index = 4;
double  zoom;
int init_sock(CSimpleIniA* ini)
{
      nmea_recv = new nmea_reciever(ini, &nmea_list);
      return 0;
}
int32  update_nmea()
{
      std::string nmea_str;
      int32 c = 0;
      while (!nmea_list.empty())
      {
            nmea_str = nmea_list.front();
            nmea_list.pop();
            //printf("nmea in: %s\n", nmea_str.c_str());
            parse_nmea(nmea_str);
            c++;
      }
      return c;

}
void zoom_changed(int32 new_zoom_index)
{

      if (new_zoom_index < 0)
            new_zoom_index = 0;
      else if (new_zoom_index >= max_zoom_index)
            new_zoom_index = max_zoom_index - 1;
      zoom_index = new_zoom_index;
      zoom = (double)min_fit / ZOOM_RANGE[zoom_index] / 2;
}
////////////////////////////////////////////////////////////
IntPoint transform_point(FloatPoint pt)
{
      //pt.offset_remove(own_vessel.get_meters());

      pt.to_polar();
      if (own_vessel.get_relative())
            pt.x -= own_vessel.get_heading();
      pt.y *= zoom;
      pt.to_decart();
      int x = int(round(pt.x)) + CENTER_X,
            y = int(round(pt.y)) + CENTER_Y;
      //y = invertYaxis ? CENTER_Y - y - 1 : y + CENTER_Y;
      return IntPoint{ x,y };
}
IntRect rotate_shape_box(poly* shape)
{
      IntRect rct;
      FloatPoint fpt;
      IntPoint ipt;
      int corner = 3, first = 1;
      while (corner >= 0)
      {
            fpt = shape->bounds.get_corner(corner);
            ipt = transform_point(fpt);
            if (first)
            {
                  rct = IntRect{ ipt.x, ipt.y };
                  first = 0;
            }
            else
            {
                  rct.minmax_expand(ipt);
            }
            corner--;
      }

      return rct;
}
IntRect rotate_shape(poly* shape)
{
      IntRect rct;
      int first = 1;
      //rct.assign_pos(0.0, 0.0);

      for (int pt_index = 0; pt_index < shape->points_count; pt_index++)
      {
            shape->work[pt_index] = transform_point(shape->origin[pt_index]);
            if (first)
            {
                  rct = IntRect{ shape->work[pt_index].x, shape->work[pt_index].y };
                  first = 0;
            }
            else {
                  rct.minmax_expand(shape->work[pt_index]);
            }
      }
      return rct;

}
////////////////////////////////////////////////////////////
int32 load_shapes()
{

      //MYSQL_RES * res;      MYSQL_ROW row;


      //double overlap_coeff = 1.05 * ZOOM_RANGE[max_zoom_index - 1];
      double overlap_coeff = 5.0;// 0.05 * ZOOM_RANGE[max_zoom_index - 1];
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
      const std::string& tmp = sstr.str();
      const char* cstr = tmp.c_str();

      // IntRect tmp = VIEWBOX_RECT;
      FloatPoint gps = own_vessel.get_meters();
      mysql->exec_prepared(PREPARED_MAP1,
            gps.x + VIEWBOX_RECT.left() * overlap_coeff,
            gps.y + VIEWBOX_RECT.bottom() * overlap_coeff,
            gps.x + VIEWBOX_RECT.right() * overlap_coeff,
            gps.y + VIEWBOX_RECT.top() * overlap_coeff,
            cstr);
      //mysql->free_result();
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
            shape.origin = new FloatPoint[shape.points_count];
            shape.work = new IntPoint[shape.points_count];

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
      if (own_vessel.get_pos_index() != -1)
      {

            if (ms > next_check)
            {
                  load_shapes();
                  //garbage_shapes(shapes)
                  next_check = ms + SHAPES_UPDATE;
            }
            //      else                  next_check = ms + SHAPES_UPDATE_FIRST;
            return next_check;
      }
      else
            return ms;

      //return 0;
}

////////////////////////////////////////////////////////////
void draw_vessels(const ARGB fill_color, const ARGB outline_color)
{
      IntRect test_box;
      const vessel* v;
      FloatPoint vessel_center, fp, own_pos;
      IntPoint int_center;
      uint32 circle_size = imax((int)(6 * zoom), 3);
      own_pos = own_vessel.get_meters();
      for (const auto& vx : vessels)
      {
            v = &vx.second;
            if (v->pos_ok)
            {
                  // calculate vessel center (with zoom and rotation)
                  vessel_center = v->gps;
                  vessel_center.latlon2meter();

                  vessel_center.offset_remove(own_pos);
                  vessel_center.to_polar();
                  vessel_center.y *= zoom;
                  if (own_vessel.get_relative())
                        vessel_center.x -= own_vessel.get_heading();
                  vessel_center.to_decart();

                  int_center = vessel_center.to_int();
                  int_center.offset_add(CENTER_X, CENTER_Y);

                  //printf("%d %d \n", int_center.x, int_center.y);

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
                                    test_box = IntRect{ v->work[p].x, v->work[p].y };
                              else
                              {
                                    test_box.minmax_expand(v->work[p]);
                                    /*test_box.x1 = imin(test_box.x1, v->work[p].x);
                                    test_box.y1 = imin(test_box.y1, v->work[p].y);
                                    test_box.x2 = imax(test_box.x2, v->work[p].x);
                                    test_box.y2 = imax(test_box.y2, v->work[p].y);
                                    */
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
                        screen->circle({ int_center.x, int_center.y, circle_size }, 0x00000000, 0x0000FF00);
                  }

            }





      }
}
void draw_shapes(const ARGB fill_color, const ARGB outline_color)
{

      //rotate box and get new bounds
      IntRect new_box;
      for (auto& shape : shapes)
            //auto const & ent1 : mymap
            //for (poly shape : shapes)
      {
            if (fill_color != clNone || outline_color != clNone) {
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

      FloatPoint fp = { 0.0,0.0 };
      //const string sides = "ESWN";
      const std::string sides = " S N";

      for (int s = 0; s < 4; s++)
      {
            // axis
            fp.x = -angle;
            fp.y = min_fit + 3;
            fp.to_decart();
            screen->draw_line_v2(CENTER, CENTER.offset(fp.ix(), fp.iy()), 0xF0000000);

            // axis letters
            fp.x = -angle;
            fp.y = min_fit + 9;
            fp.to_decart();
            screen->draw_text(FONT_OUTLINE, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), sides.substr(s, 1), HALIGN_CENTER | VALIGN_CENTER, clBlack, clLand);

            angle += 90.0;
            if (angle >= 360.0) angle -= 360.0;
      }
      int32 v;
      std::string s;
      for (uint32 i = 1; i < 6; i++)
      {
            // distance circle
            screen->circle({ CENTER_X, CENTER_Y, i * circle_radius }, 0xF0000000, clNone);

            // distance marks
            fp.x = -angle;
            fp.y = circle_radius * i;
            fp.to_decart();
            v = ZOOM_RANGE[zoom_index] / 5 * i;
            if (v >= 1000)  // make large numbers more compact
                  //     s = string_format("%d.%d", v / 1000, v % 1000);
                  s = string_format("%.1f", v / 1000.0);
            else
                  s = string_format("%d", v);
            screen->draw_text(FONT_OUTLINE, CENTER_X + fp.ix(), CENTER_Y + fp.iy(), s, HALIGN_CENTER | VALIGN_CENTER, clBlack, clLand);
      }

}
void draw_infoline()
{
      const int PIVOTX = CENTER_X, PIVOTY = 20,
            SPACEX = 3, SPACEY = 1;
      char buf[16] = { 0 };
      using sysclock_t = std::chrono::system_clock;

      std::time_t now = sysclock_t::to_time_t(sysclock_t::now());

      // time and date
      std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
      screen->draw_text(FONT_MONOMEDIUM, PIVOTX - SPACEX, PIVOTY + SPACEY, std::string(buf), VALIGN_BOTTOM | HALIGN_RIGHT, clBlack, clLand, true);
      std::strftime(buf, sizeof(buf), "%a,%e %b", std::localtime(&now));
      screen->draw_text(FONT_MONOMEDIUM, PIVOTX - SPACEX, PIVOTY - SPACEY, std::string(buf), VALIGN_TOP | HALIGN_RIGHT, clBlack, clLand, true);


      FloatPoint gps = own_vessel.get_pos();
      //double lon = dm_s2deg_v2(gps.x),            lat = dm_s2deg_v2(gps.y);
      screen->draw_text(FONT_MONOMEDIUM,
            PIVOTX + SPACEX, PIVOTY + SPACEY,
            string_format("\x81 %.8f %.8f", gps.x, gps.y),// lon, lat),
            VALIGN_BOTTOM | HALIGN_LEFT,
            clBlack, clLand, true);
      screen->draw_text(FONT_MONOMEDIUM, PIVOTX + SPACEX, PIVOTY - SPACEY, string_format("\x80 %d/%d", sat.get_used_count(), sat.get_active_count()), VALIGN_TOP | HALIGN_LEFT, clBlack, clLand, true);

}
void draw_vessels_info() {
      struct less_than_key
      {
            inline bool operator() (const int& mmsi0, const int& mmsi1)
            {
                  double d0 = vessels[mmsi0].distance, d1 = vessels[mmsi1].distance;
                  if (std::isnan(d0)) return false;
                  else if (std::isnan(d1)) return true;
                  else return d0 < d1;

            }
      };
      const int LINE_HEIGHT = 12;
      //int left = ;
      //IntRect info_rect{ CENTER_X * 2 ,0,screen->get_width() -1,screen->get_height() }
      int headers[4] = { 2, 20, 85, -2 };
      //w = screen->get_width() / 3;
      screen->fill_rect(SHIPLIST_RECT, 0x30FFFFFF);

      // get all MMSI to vector + alculate distance
      std::vector<int> mmsi;
      for (std::map<int, vessel>::iterator it = vessels.begin(); it != vessels.end(); ++it) {

            if (it->second.pos_ok)
                  it->second.distance = it->second.gps.haversine(own_vessel.get_pos());
            else
                  it->second.distance = INT_MAX;
            mmsi.push_back(it->first);
      }

      // sort mmsi by distance
      std::sort(mmsi.begin(), mmsi.end(), less_than_key());

      // draw header
      int   y_coord = SHIPLIST_RECT.top() - 2;
      screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.left() + headers[0], y_coord - 1, "CC", VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.left() + headers[1], y_coord - 1, "MMSI", VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.left() + headers[2], y_coord - 1, "SHIPNAME", VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.right() + headers[3], y_coord - 1, "DIST", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      y_coord -= LINE_HEIGHT;

      // draw ship list
      touchman->clear_group(TOUCH_GROUP_SHIPLIST);
      int lines_count = imin((int)mmsi.size(), y_coord / LINE_HEIGHT), mid;
      for (int i = 0; i < lines_count; i++)
      {
            mid = vessels[mmsi[i]].mid;
            if (mid_list.count(mid) != 0)
            {
                  std::string ccode = mid_list[mid].code;
                  if (mid_country.count(ccode) != 0)
                  {
                        //image * img = &mid_country[ccode];
                        screen->draw_image(&mid_country[ccode], SHIPLIST_RECT.left() + 2, y_coord, VALIGN_TOP | HALIGN_LEFT);
                  }
            }
            else
            { // no mid info found, just out mid code
                  screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.left() + headers[0], y_coord - 1, string_format("%d", mid), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
            }
            /*
                      struct mid_struct_s
                      {
                            std::string code, country;
                      };

                      extern std::map<int, mid_struct_s> mid_list;
                      extern std::map<std::string, image> mid_country;
                      */

            screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.left() + headers[1], y_coord - 1, string_format("%d", mmsi[i]), VALIGN_TOP | HALIGN_LEFT, clRed, clNone);
            screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.left() + headers[2], y_coord - 1, vessels[mmsi[i]].shipname, VALIGN_TOP | HALIGN_LEFT, clBlue, clNone);
            screen->draw_text(FONT_NORMAL, SHIPLIST_RECT.right() + headers[3], y_coord - 1, string_format("%d", vessels[mmsi[i]].distance), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            touchman->add_rect(
                  TOUCH_GROUP_SHIPLIST,
                  mmsi[i],// "ship",//std::string(),
                  { SHIPLIST_RECT.left(),y_coord - LINE_HEIGHT,SHIPLIST_RECT.right(),y_coord });
            y_coord -= LINE_HEIGHT;
      }


      // draw lines
      // 


}

void draw_track_info(int32 x, int32 y)
{// draw last tracking
      int32 col_pos[6] = { 0,75,150,250,350 };
      int32 sessid, time,dist;
      //uint64_t timestart, timeend;      double dist;
      // tracks header
      screen->draw_text(FONT_NORMAL, x + col_pos[1], y, "ID", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[2], y, "sec", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[3], y, "dist", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      y -= 4 + screen->get_font_height(FONT_NORMAL);

      if (mysql->exec_prepared(PREPARED_GPS_TOTAL))
            return;

      mysql->store();
      while (mysql->fetch()) {
            sessid = mysql->get_myint("sessid");
            time = mysql->get_myint("time");
            dist = mysql->get_myint("dist");

            screen->draw_text(FONT_NORMAL, x + col_pos[1], y, string_format("#%d", sessid), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[2], y, time_diff(time), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[3], y, string_format("%d", dist), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            y -= 4 + screen->get_font_height(FONT_NORMAL);

      }
      //      gps_session_id = driver->get_myint("sessid") + 1;
      mysql->has_next();
}
void draw_satellites_info(int32 x, int32 y)
{
      // satellite table header      
      int32 col_pos[6] = { 0,25,50,75,100,400 };
      screen->draw_text(FONT_NORMAL, x + col_pos[1], y, "PRN", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[2], y, "El", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[3], y, "Az", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[4], y, "SNR", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      y -= 4 + screen->get_font_height(FONT_NORMAL);

      // satellite table
      ARGB color;
      int32 lw;
      for (auto& s : sat.list)
      {
            if (!s.second.used && !s.second.is_active())
                  continue;

            // draw satellite SNR bar

            if (s.second.snr < 10)
            {
                  color = clRed;
            }
            else
                  if (s.second.snr > 20)
                  {
                        color = clGreen;
                  }
                  else
                  {
                        color = clYellow;
                  }

            if (!s.second.used)
            {
                  color |= clTransparency75;
            }

            lw = ((col_pos[5] - col_pos[4]) * s.second.snr) / 100;
            screen->fill_rect(x + col_pos[4] + 2, y - 8, x + col_pos[4] + lw + 1, y, color);

            // draw satellite text data
            if (s.second.used)
            {
                  color = clBlack;
            }
            else if (s.second.is_active())
            {
                  color = clGray;
            }
            else
            {
                  continue;
            }
            screen->draw_text(FONT_NORMAL, x + col_pos[1], y, string_format("%d", s.first), VALIGN_TOP | HALIGN_RIGHT, color, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[2], y, string_format("%d", s.second.elevation), VALIGN_TOP | HALIGN_RIGHT, color, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[3], y, string_format("%d", s.second.azimuth), VALIGN_TOP | HALIGN_RIGHT, color, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[4], y, string_format("%d", s.second.snr), VALIGN_TOP | HALIGN_RIGHT, color, clNone);
            y -= 4 + screen->get_font_height(FONT_NORMAL);
      }

}

void draw_infowindow_global()
{
      int32 x = WINDOW_RECT.left() + 5, y = WINDOW_RECT.top() - 5;
      // window header
      screen->draw_text(FONT_MONOMEDIUM, x, y, "Global Info", VALIGN_TOP | HALIGN_LEFT, clNone, clNone);
      y -= 10 + screen->get_font_height(FONT_MONOMEDIUM);
      draw_track_info(x, y);
      draw_satellites_info(x + 350, y);
}
void draw_timewindow_global()
{
      int32 x = WINDOW_RECT.left() + 5, y = WINDOW_RECT.top() - 5;
      screen->draw_text(FONT_MONOMEDIUM, x, y, "Time Info", VALIGN_TOP | HALIGN_LEFT, clNone, clNone);//y -= 10 + screen->get_font_height(FONT_MONOMEDIUM);

}

void draw_infowindow_vessel()
{
      vessel* v = &vessels[show_info_window];
      if (!v) return;
      int32 x = WINDOW_RECT.left() + 5, y = WINDOW_RECT.top() - 5;
      screen->draw_text(FONT_MONOMEDIUM, x, y, "VESSEL Info", VALIGN_TOP | HALIGN_LEFT, clNone, clNone); y -= 10 + screen->get_font_height(FONT_MONOMEDIUM);
      screen->draw_text(FONT_NORMAL, x, y, string_format("MMSI: %d", show_info_window), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
      screen->draw_text(FONT_NORMAL, x, y, string_format("GPS: %.6f,%.6f", v->gps.x, v->gps.y), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
      screen->draw_text(FONT_NORMAL, x, y, string_format("COG: %f", v->course), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
      screen->draw_text(FONT_NORMAL, x, y, string_format("Heading: %f", v->heading), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
}

void draw_infowindow()
{
      if (!show_info_window)
            return;
      // draw window
      //const int32 lh = 12;
      screen->fill_rect(WINDOW_RECT, clWhite | clTransparency12);
      screen->rectangle(WINDOW_RECT, clBlack);

      if (show_info_window == -2)
      {// global info (sattelites etc)
            draw_infowindow_global();

      }
      else if (show_info_window == -1)
      {// time, path, etc
            draw_timewindow_global();

      }
      //else if (show_info_window == -2)
      //{// global info (sattelites etc)
      //      draw_infowindow_global();

      //}
      else if (show_info_window > 0)
      { // ship info
            draw_infowindow_vessel();
      }

}
void draw_frame()
{
      try
      {
            /*
            if (!own_vessel.pos_ok())
            {
                  // print `no GPS` and exit
                  screen->rectangle(SCREEN_RECT, 0x001111);
                  //screen->draw_text(FONT_NORMAL, screen->get_width() / 2, screen->get_height() / 2, "no GPS position", VALIGN_CENTER | HALIGN_CENTER, 0xFFFF00, clWhite);
                  return;
            }
            */
            screen->fill_rect(SCREEN_RECT, clSea);

            //screen->draw_text(SPECCY_FONT, screen->get_width() / 2, 25, "HAS GPS !!!", 0xFFFF00, VALIGN_CENTER | HALIGN_CENTER);
            //FloatPoint mypos = own_vessel.get_pos();
            //mypos.latlon2meter();

            if (own_vessel.get_pos_index() == -1)
            {
                  screen->draw_text(FONT_MONOMEDIUM,
                        screen->width() / 2, screen->height() / 2,
                        "NO GPS", VALIGN_CENTER | HALIGN_CENTER, clDkRed, clNone);
                  return;
            }
            touchman->set_enabled(1);

            draw_shapes(clLand, clBlack);
            draw_grid(own_vessel.get_relative() ? own_vessel.get_heading() : 0.0);
            draw_vessels(0xcd8183, clBlack);
            draw_vessels_info();

            // plus and minus images
            screen->draw_image(img_minus, ZOOM_BTN_MARGIN, screen->height() - ZOOM_BTN_MARGIN, HALIGN_LEFT | VALIGN_TOP, 150);
            screen->draw_image(img_plus, SHIPLIST_RECT.left() - ZOOM_BTN_MARGIN, screen->height() - ZOOM_BTN_MARGIN, HALIGN_RIGHT | VALIGN_TOP, 150);

            draw_infoline();

            int x = SHIPLIST_RECT.left() + 5, y = SHIPLIST_RECT.bottom() + 5;
            screen->draw_text(FONT_NORMAL, x, y, string_format("ZOOM #%d %f", zoom_index, zoom), VALIGN_BOTTOM | HALIGN_LEFT, clBlack, clNone);

            draw_infowindow();
      }
      catch (...)
      {
            std::cerr << "[E] draw_frame" << std::endl;
      }
}
////////////////////////////////////////////////////////////
void process_touches() {

      std::string msg;
      int32 group_id, area_id;
      touches_coords_s t;
      //touchman->check_point(30, 300, gi, name);
      while (touchscr->pop(t))
      {
            PRINT_STRING(string_format("Touch: %d,%d ... \n", t.adjusted[0], t.adjusted[1]));
            if (touchman->check_point(t.adjusted[0], t.adjusted[1], group_id, area_id))
            {
                  //PRINT_STRING(                  string_format(printf("OK\n");
                  switch (group_id)
                  {
                        case TOUCH_GROUP_ZOOM: {
                              if (area_id == TOUCH_AREA_ZOOMIN)
                              {

                                    zoom_changed(zoom_index - 1);
                              }
                              else if (area_id == TOUCH_AREA_ZOOMOUT)
                              {
                                    zoom_changed(zoom_index + 1);
                              }

                              break;
                        }
                        case TOUCH_GROUP_INFOLINE:
                        { // show info window
                              show_info_window = -1 - area_id; // -1 for time, -2 for gps

                              // deactivate all touch groups,except window group
                              touchman->set_groups_active(0);
                              if (touchman->set_group_active(TOUCH_GROUP_INFOWINDOW, 1))
                                    printf("[TOUCH] error set active %d to group %d", 1, TOUCH_GROUP_INFOWINDOW);
                              break;
                        }
                        case TOUCH_GROUP_INFOWINDOW:
                        { // hide info window
                              show_info_window = 0;
                              // enable all touch groups, disable window group
                              touchman->set_groups_active(1);
                              if (touchman->set_group_active(TOUCH_GROUP_INFOWINDOW, 0))
                                    printf("[TOUCH] error set active %d to group %d", 0, TOUCH_GROUP_INFOWINDOW);
                              break;
                        }
                        case TOUCH_GROUP_SHIPLIST:
                        {// show vessel info window
                              show_info_window = area_id;
                              touchman->set_groups_active(0);                               // deactivate all touch groups,except window group
                              if (touchman->set_group_active(TOUCH_GROUP_INFOWINDOW, 1))
                                    printf("[TOUCH] error set active %d to group %d", 1, TOUCH_GROUP_INFOWINDOW);
                              break;
                        }

                        default:
                        {
                              printf("Unhandled touch group: %d\n", group_id);
                              break;
                        }
                  }

                  //printf("(group %d, name: %s)\n", gi, name.c_str());
                  //touchman->dump();
            }
            else
                  printf("not found\n");
      }





}
////////////////////////////////////////////////////////////
void video_loop_start() {
      /*
      const int FPS = 60;
      unsigned long long ns_delay = 1000000000L / FPS, diff, frames_total = 0;
      struct timespec frame_start, frame_end, measure_start;

      struct timespec sleep_timer = { 0,0 }, sleep_dummy;
        */



      int frames = 0;

      std::string fps_str = "";


      while (true)
      {

            //last_msg_id = update_nmea(last_msg_id);
            update_nmea();
            next_map_update = update_map_data(next_map_update);
            draw_frame();

            process_touches();
            touchman->debug(screen);


            frames++;
            //fps_str = "none";            screen->draw_text(FONT_NORMAL, 5, 5, fps_str, 0xFFFF00, VALIGN_BOTTOM | HALIGN_LEFT);


            screen->flip();
            usleep(100000);
      }

      /*  clock_gettime(CLOCK_REALTIME, &measure_start);
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

        }*/
}
////////////////////////////////////////////////////////////
void init_video(const char* buff) {
      screen = new video_driver(buff, 0); // debug purpose = 1 buffer, production value = 5
      if (screen->get_last_error())
      {
            printf("video_init error: %d.\n", screen->get_last_error());
            return;
      }

      //screen->fill_rect(SCREEN_RECT, clLtGray);
      //screen->draw_text(FONT_LARGE, 100, 100, "XYZ", VALIGN_TOP | HALIGN_LEFT, clRed | clTransparency50, clNone);

      screen->load_font(FONT_OUTLINE, data_path("/img/outline.png"));
      screen->load_font(FONT_NORMAL, data_path("/img/normal_v2.png"));
      screen->load_font(FONT_MONOMEDIUM, data_path("/img/medium.png"));
      screen->set_font_interval(FONT_MONOMEDIUM, 2);


      img_minus = new image(data_path("/img/minus_v2.png"));
      img_plus = new image(data_path("/img/plus.png"));
      min_fit = imin(
            imin(CENTER_X, screen->width() - CENTER_X),
            imin(CENTER_Y, screen->height() - CENTER_Y)
      ) - 20;
      circle_radius = min_fit / 5;
}
void init_touch(CSimpleIniA* ini) {

      touchscr = new   touchscreen(ini, screen->width(), screen->height());
      touchman = new touch_manager();

      touchman->add_group(TOUCH_GROUP_ZOOM, 15);
      IntRect rct;
      rct.set_left(ZOOM_BTN_MARGIN);
      rct.set_top(screen->height() - ZOOM_BTN_MARGIN);
      rct.set_right(rct.left() + img_minus->width());
      rct.set_bottom(rct.top() - img_minus->height());
      touchman->add_circle(TOUCH_GROUP_ZOOM, TOUCH_AREA_ZOOMOUT, rct.center(), 40);


      rct.set_right(SHIPLIST_RECT.left() - ZOOM_BTN_MARGIN);
      rct.set_left(rct.right() - img_plus->width());
      rct.set_bottom(rct.top() - img_plus->height());
      touchman->add_circle(TOUCH_GROUP_ZOOM, TOUCH_AREA_ZOOMIN, rct.center(), 40);


      touchman->add_group(TOUCH_GROUP_SHIPSHAPE, 5);
      touchman->add_group(TOUCH_GROUP_SHIPLIST, 12);
      //touchman->add_rect(TOUCH_GROUP_SHIPLIST, "test", { 10,20,30,40 });

      touchman->add_group(TOUCH_GROUP_INFOLINE, 10);
      touchman->add_rect(TOUCH_GROUP_INFOLINE, 0, { SCREEN_RECT.left(),SCREEN_RECT.bottom(),CENTER_X ,40 });
      touchman->add_rect(TOUCH_GROUP_INFOLINE, 1, { CENTER_X,SCREEN_RECT.bottom(),SCREEN_RECT.right(),40 });

      touchman->add_group(TOUCH_GROUP_INFOWINDOW, 30, 0);
      touchman->add_rect(TOUCH_GROUP_INFOWINDOW, 0, WINDOW_RECT);

      touchman->set_enabled(1); // tmp!!!
}
void init_database() {
      mysql = new mysql_driver("127.0.0.1", "map_reader", "map_reader", "ais");
      if (mysql->get_last_error_str()) {
            printf("mysql init error.\n%s\n", mysql->get_last_error_str());
            return;

      }
      init_db(mysql);
}
void do_poly_test() {
      poly shape;

      shape.path_count = 1;
      shape.points_count = 3;
      shape.pathindex = new int[shape.path_count + 1];
      shape.pathindex[shape.path_count] = shape.points_count; // set closing point
      shape.origin = new FloatPoint[shape.points_count];
      shape.work = new IntPoint[shape.points_count];

      shape.origin[0] = { -70.866, 377.953 };
      shape.origin[1] = { 283.465, 354.331 };
      shape.origin[2] = { 59.055, 47.244 };
      for (int n = 0; n < shape.points_count; n++)
            shape.work[n] = shape.origin[n].to_int();





      screen->fill_rect(SCREEN_RECT, clLtGray);
      screen->draw_outline_v2(&shape, clBlack);

}
int main()
{

      //printf("Started...\n\n");      std::cout << "Started 2..." << std::endl;
      try {
            CSimpleIniA ini;
            std::string ini_filename = data_path("/options.ini");
            SI_Error rc = ini.LoadFile(ini_filename.c_str());
            if (rc < 0)
            {
                  printf("Error while loading ini-file: %s", ini_filename.c_str());
                  return EXIT_FAILURE;
            }



            const char* buff;
            buff = ini.GetValue("main", "video", "/dev/fb0");
            init_video(buff);
            init_touch(&ini);
            init_database();
            init_sock(&ini);
            //sat = new sattelites();

            zoom_changed(zoom_index);

            next_map_update = 0;

            // simulate info bar click
            touchscr->simulate_click(360, 20);


            video_loop_start();
            return 0;
      }
      catch (const char* exception) // обработчик исключений типа const char*
      {
            std::cerr << "Error: " << exception << '\n';
      }
      catch (char* e) {
            std::cerr << "[EXCEPTION] " << e << std::endl;
            return false;
      }
      catch (...) {
            std::cerr << "[E] " << std::endl;
      }
      return 0;
}
