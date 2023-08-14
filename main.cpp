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
#include <stdlib.h>
#include "nmeastreams.h"
#include <SimpleIni.h>
#include <queue>
#include "ais_types.h"



const uint64_t SHAPES_UPDATE = 60 * 1000;  // 5 min for update shapes
uint64_t next_map_update;

int32 min_fit, circle_radius;
//IntRect zoomed_view;


image* img_minus, * img_plus;
const int32 ZOOM_BTN_MARGIN = 20;

/*
display_win_mode - defines what exactly shows, 0 = none, 1 = time/sats, 2 = ships
display_win_param - defines additional info, for info - win id, for ship - mmsi
*/
const int32 DISPLAY_WIN_NONE = 0;
const int32 DISPLAY_WIN_INFO = 1;
const int32 DISPLAY_WIN_VESSEL = 2;
int32 display_win_mode, display_win_param;

std::time_t global_time = 0;

const int32 INFOLINE_BOX_COUNT = 5;
IntRect infoline_rct[INFOLINE_BOX_COUNT];


touchscreen* touchscr;
touch_manager* touchman;
nmea_reciever* nmea_recv;
StringQueue  nmea_list;



const int max_zoom_index = 9;
const int ZOOM_RANGE[max_zoom_index] = { 50, 120,200,300, 1000, 2000, 3000, 5000,10000 };
int zoom_index = 4;
double  zoom, map_update_coeff;

int32 update_time() {
      std::time_t time_check = std::time(nullptr);
      if (time_check != global_time)
      {
            global_time = time_check;
            return 1;
      }
      return 0;


      /// now you can format the string as you like with `strftime`
}


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
      zoom = (double)min_fit / ZOOM_RANGE[zoom_index];
      //zoomed_view = VIEWBOX_RECT;      zoomed_view.zoom(1 / zoom);
}
////////////////////////////////////////////////////////////
int32 load_shapes()
{
      // create rect from screen and move to own pos
      IntPoint own_center = own_vessel.get_meters().to_int();
      IntRect query_coords(VIEWBOX_RECT);
      PolarPoint transform_value(-own_vessel.heading(), map_update_coeff);
      query_coords.transform_bounds(transform_value);
      query_coords.add(own_center);

      mysql->exec_prepared(PREPARED_MAP_QUERY,
            query_coords.left(),
            query_coords.bottom(),
            query_coords.right(),
            query_coords.top());
      int32 rec_id, points_total = 0, points_cnt;

      // fetch shapes
      std::vector <int32> id_list;

      mysql->exec_prepared(PREPARED_MAP_READ_BOUNDS);
      while (mysql->fetch())
      {
            rec_id = mysql->get_int("recid");
            id_list.push_back(rec_id);
            map_shapes.emplace(std::piecewise_construct, std::forward_as_tuple(rec_id), std::forward_as_tuple());
            map_shapes[rec_id].last_access = utc_ms();
            points_cnt = mysql->get_int("points");
            map_shapes[rec_id].shape.origin.resize(points_cnt);
            map_shapes[rec_id].shape.work.resize(points_cnt);
            points_total += points_cnt;
            map_shapes[rec_id].shape.path_ptr.resize(mysql->get_int("parts"));
            map_shapes[rec_id].shape.bounds = IntRect(
                  mysql->get_int("minx"),
                  mysql->get_int("miny"),
                  mysql->get_int("maxx"),
                  mysql->get_int("maxy")
            );
      }
      mysql->close_res();

      // fetching points
      int32 prev_rec_id = -1, prev_part_id = -1, part_id, part_index, point_index;//         rec_id, part_id, prev_part_id;
      mysql->exec_prepared(PREPARED_MAP_READ_POINTS);
      while (mysql->fetch())
      {
            rec_id = mysql->get_int("recid");
            if (prev_rec_id != rec_id)
            {             // start new shape
                  prev_rec_id = rec_id;
                  prev_part_id = -1;
                  point_index = 0;
                  part_index = -1;
            }
            part_id = mysql->get_int("partid");
            if (prev_part_id != part_id)
            { // next part start
                  prev_part_id = part_id;
                  part_index++;
            }

            map_shapes[rec_id].shape.origin[point_index] = {
            mysql->get_double("x") ,
            mysql->get_double("y")
            };
            point_index++;
            map_shapes[rec_id].shape.path_ptr[part_index] = point_index;
      }
      mysql->close_res();

      printf("[i] Shapes: %d, points: %d\n", id_list.size(), points_total);
      return 0;
}
int32  update_map_data(uint64_t& next_check)
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
            return 1;
      }
      else
            return 0;

      //return 0;
}

////////////////////////////////////////////////////////////
void draw_vessels(const ARGB fill_color, const ARGB outline_color)
{
      const int32 circle_radius = 5;
      vessel* v;
      FloatPoint vessel_center_f, ftmp,
            own_center = own_vessel.get_meters();
      IntPoint vessel_center_i, itmp;
      //    uint32 circle_size = imax((int)(6 * zoom), 3);

      PolarPoint polar_pt, transform_value;
      IntRect vessel_bounds;
      size_t n;
      touchman->clear_group(TOUCH_GROUP_SHIPSHAPE);
      // IntRect view_box(VIEWBOX_RECT);      view_box.zoom(1 / zoom);// = view_box.transform_bounds(transform_value);

      for (auto& vx : vessels)
      {
            v = &vx.second;
            if (v->pos_ok)
            {
                  // calculate vessel center (with zoom and rotation)
                  vessel_center_f = v->gps;
                  vessel_center_f.latlon2meter();
                  vessel_center_f.sub(&own_center);
                  polar_pt = vessel_center_f.to_polar();
                  polar_pt.zoom(zoom);
                  if (own_vessel.relative())
                        polar_pt.rotate(-own_vessel.heading());
                  vessel_center_i = polar_pt.to_int();
                  vessel_center_i.add(&CENTER);
                  //printf("VIEWBOX_RECT: %s\n", VIEWBOX_RECT.dbg().c_str());


                  if (v->size_ok && v->angle_ok)
                  {
                        transform_value = { DEG2RAD(v->heading),zoom };
                        if (own_vessel.relative())
                              transform_value.rotate(-own_vessel.heading());

                        for (n = 0; n < v->shape.origin.size(); n++)
                        {
                              ftmp = v->shape.origin[n];
                              ftmp.transform(&transform_value);
                              v->shape.work[n] = ftmp.to_int();
                              v->shape.work[n].add(&vessel_center_i);


                              if (n)
                                    vessel_bounds.modify(&v->shape.work[n]);
                              else
                                    vessel_bounds.init(&v->shape.work[n]);
                        }
                        //printf("vessel_bounds: %s\n", vessel_bounds.dbg().c_str());
                        if (vessel_bounds.is_intersect(SCREEN_RECT))
                        {
                              //printf("+ %s\n", v->shipname.c_str());
                              screen->draw_shape(&v->shape, clLime, clBlack);
                              //screen->draw_text(small_font, test_box.hcenter(), test_box.vcenter(), string_format("%d", v->mmsi), 0x000000, HALIGN_CENTER | VALIGN_CENTER);
                        }  //else                              printf("- %s\n", v->shipname.c_str());
                  }
                  else//unknown size or angle, just draw small circle with position
                  {
                        screen->draw_circle({ vessel_center_i.x, vessel_center_i.y, circle_radius }, clBlack, clRed);
                        vessel_bounds = IntRect(vessel_center_i.x- circle_radius, vessel_center_i.y- circle_radius, vessel_center_i.x+ circle_radius, vessel_center_i.y+ circle_radius);
                  }
                  touchman->add_rect(TOUCH_GROUP_SHIPSHAPE, vx.first, vessel_bounds);
            }
      }
}

void draw_shapes(const ARGB fill_color, const ARGB outline_color)
{
      // считаем наш тестовый прямоугольник (view_box), это видимая часть
     //все вычисления приводим к центру (0,0)

      //rotate box and get new bounds
      PolarPoint transform_value(own_vessel.heading(), zoom);
      //FloatPoint  = own_vessel.get_meters();
      //IntPoint own_center = own_vessel.get_meters().to_int();
      FloatPoint own_center = own_vessel.get_meters();
      FloatPoint fp;
      //IntRect view_box(VIEWBOX_RECT);      view_box.zoom(1 / zoom);// = view_box.transform_bounds(transform_value);


      IntRect test_box;
      size_t n;
      //Poly* p;
      for (auto& sh : map_shapes)

      {
            //p = &sh.second.shape;
            //calculate new transormed corners
            test_box = sh.second.shape.bounds;
            test_box.sub(own_center);
            test_box.transform_bounds(transform_value);

            //, own_center, nullptr);
            if (test_box.is_intersect(VIEWBOX_RECT))
            {
                  sh.second.last_access = utc_ms(); // update using time

                  // transform points
                  for (n = 0; n < sh.second.shape.origin.size(); n++)
                  {
                        fp = sh.second.shape.origin[n];
                        fp.sub(&own_center);
                        fp.transform(&transform_value);
                        sh.second.shape.work[n] = fp.to_int();
                        sh.second.shape.work[n].add(&CENTER);
                        //write_log( string_format("%d.0\t%d.0", sh.second.shape.work[n].x, sh.second.shape.work[n].y)  );
                  }


                  //sh.second.shape.transform_points(&transform_value, &own_center, nullptr);
                  //sh.second.shape.copy_origin_to_work();
                  //sh.second.shape.sub(center);
                  //sh.second.shape.transform_points(transform_value);
                  screen->draw_shape(&sh.second.shape, clLand, clBlack);
            }

      }

}
void draw_grid(double a)
{
      PolarPoint pp;
      //FloatPoint fp;
      IntPoint ip;
      //const string sides = "ESWN";
      const std::string sides = " S N";

      for (int s = 0; s < 4; s++)
      {
            // axis
            pp.angle = -a;
            pp.dist = min_fit + 3;
            ip = pp.to_int();
            ip.add(&CENTER);
            screen->draw_line(&CENTER, &ip, 0xF0000000);

            // axis letters
            pp.dist += 6;
            ip = pp.to_int();
            ip.add(&CENTER);
            screen->draw_text(FONT_OUTLINE, ip, sides.substr(s, 1), HALIGN_CENTER | VALIGN_CENTER, clBlack, clLand);

            a += RAD90;
            //if (angle >= 360.0) angle -= 360.0;
      }
      int32 v, radius;
      std::string s;
      pp.angle = 0;
      for (uint32 i = 1; i < 6; i++)
      {
            radius = i * circle_radius;
            // distance circle
            screen->draw_circle({ CENTER.x,CENTER.y, radius }, 0xF0000000, clNone);

            // distance marks
            pp.dist = radius;
            ip = pp.to_int();
            ip.add(&CENTER);
            v = ZOOM_RANGE[zoom_index] / 5 * i;
            if (v >= 1000)  // make large numbers more compact
                  s = string_format("%.1f", v / 1000.0);
            else
                  s = string_format("%d", v);
            screen->draw_text(FONT_OUTLINE, ip, s, HALIGN_CENTER | VALIGN_CENTER, clBlack, clLand);
      }

}
////////////////////////////////////////////////////////////
void prepare_infoline(int32 infoline_width) {
      const int32 BOX_SIZES[INFOLINE_BOX_COUNT] = { 54,54,86,34,68 };
      const int32 MARGIN = 20;

      // calc overall length
      int32 sum = 0;
      for (int32 c = 0; c < INFOLINE_BOX_COUNT; c++) {
            sum += BOX_SIZES[c];
      }
      int32 interval = (infoline_width - sum - MARGIN * 2) / (INFOLINE_BOX_COUNT - 1),
            x = MARGIN;
      for (int32 c = 0; c < INFOLINE_BOX_COUNT; c++) {
            infoline_rct[c] = { x ,MARGIN ,x + BOX_SIZES[c],MARGIN + 26 };
            x = infoline_rct[c].right() + interval;

            touchman->add_rect(TOUCH_GROUP_INFOLINE, c, infoline_rct[c]);
      }

      /*touchman->add_group(TOUCH_GROUP_INFOLINE, 10);
      //  touchman->add_rect(TOUCH_GROUP_INFOLINE, 0, { SCREEN_RECT.left(),SCREEN_RECT.bottom(),CENTER_X ,40 });
  //      touchman->add_rect(TOUCH_GROUP_INFOLINE, 1, { CENTER_X,SCREEN_RECT.bottom(),SCREEN_RECT.right(),40 });

      touchman->add_group(TOUCH_GROUP_INFOWINDOW, 30, 0);
      touchman->add_rect(TOUCH_GROUP_INFOWINDOW, 0, WINDOW_RECT);
      */
}
void draw_infoline() {
      int32 x, y,
            font_height = screen->get_font_height(FONT_NUMS) + 4;
      for (int32 c = 0; c < INFOLINE_BOX_COUNT; c++) {

            x = infoline_rct[c].left() + 3;
            y = infoline_rct[c].top() - 3;
            screen->fill_rect(infoline_rct[c], clWhite);

            //screen->draw_text(FONT_NUMS, x, y + 5, string_format("%d", c), VALIGN_BOTTOM | HALIGN_LEFT, clBlack, clNone);
            switch (c) {
                  case 0: { // time-date
                        char timeString[10];
                        //size_t error =
                        std::strftime(timeString, 10, "%T", std::gmtime(&global_time));
                        screen->draw_text(FONT_NUMS, x, y, timeString, VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        y -= font_height;
                        //error = 
                        std::strftime(timeString, 10, "%d.%m.%y", std::gmtime(&global_time));
                        screen->draw_text(FONT_NUMS, x, y, timeString, VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        break;
                  }

                  case 1: { // sunrise sunset
                        screen->draw_text(FONT_NUMS, x, y, string_format("\x80\x87%02d:%02d", 11, 2), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        y -= font_height;
                        screen->draw_text(FONT_NUMS, x, y, string_format("\x81\x87%02d:%02d", 5, 4), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        break;
                  }
                  case 2: { // latlon
                        FloatPoint gps = own_vessel.get_pos();
                        screen->draw_text(FONT_NUMS, x, y, deg2dms(gps.x), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        y -= font_height;
                        screen->draw_text(FONT_NUMS, x, y, deg2dms(gps.y), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        break;
                  }
                  case 3: { // sats
                        screen->draw_text(FONT_NUMS, x, y, string_format("\x82%2d", sat.get_used_count()), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        y -= font_height;
                        screen->draw_text(FONT_NUMS, x, y, string_format("\x83%2d", sat.get_active_count()), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        break;
                  }
                  case 4: { // dist


                        std::string v;
                        if (std::isnan(own_vessel.get_dist1()))
                              v = "\x84  \x87---.--";
                        else
                              v = "\x84" + format_thousand(own_vessel.get_dist1());
                        screen->draw_text(FONT_NUMS, x, y, v, VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        y -= font_height;
                        if (std::isnan(own_vessel.get_dist2()))
                              v = "\x84  \x87---.--";
                        else
                              v = "\x85" + format_thousand(own_vessel.get_dist2());
                        screen->draw_text(FONT_NUMS, x, y, v, VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
                        break;


                  }
            }





      }


}
////////////////////////////////////////////////////////////
void draw_vessels_info() {
      struct cmp_haversine
      {
            inline bool operator() (const int& mmsi0, const int& mmsi1)
            {
                  double d0 = vessels[mmsi0].distance, d1 = vessels[mmsi1].distance;
                  if (std::isnan(d0)) return false;
                  else if (std::isnan(d1)) return true;
                  else return d0 < d1;

            }
      };
      const int32 LINE_HEIGHT = 25;
      const int32 TEXT_MARGIN = 3;
      int32 headers[4] = { 2, 20, 85, -2 };
      std::vector<int32> mmsi;

      FloatPoint own_pos = own_vessel.get_pos();
      touchman->clear_group(TOUCH_GROUP_SHIPLIST);

      IntRect out_rect(SHIPLIST_RECT);
      screen->fill_rect(out_rect, 0x30FFFFFF);




      for (std::map<int32, vessel>::iterator it = vessels.begin(); it != vessels.end(); ++it) {     // get all MMSI to vector + alculate distance
            if (it->second.pos_ok)
            {
                  //printf("it->second.gps: %s\n", it->second.gps.dbg().c_str());
                  //printf("own_vessel.get_pos(): %s\n", own_vessel.get_pos().dbg().c_str());
                  it->second.distance = it->second.gps.haversine(&own_pos);

            }
            else
                  it->second.distance = INT_MAX;
            mmsi.push_back(it->first);
      }

      std::sort(mmsi.begin(), mmsi.end(), cmp_haversine());// sort mmsi by distance

      // draw header


      out_rect.bottom(out_rect.top() - LINE_HEIGHT);

      screen->draw_text(FONT_NORMAL, out_rect.left() + headers[0], out_rect.top() - TEXT_MARGIN, "CC", VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, out_rect.left() + headers[1], out_rect.top() - TEXT_MARGIN, "MMSI", VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, out_rect.left() + headers[2], out_rect.top() - TEXT_MARGIN, "SHIPNAME", VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, out_rect.right() + headers[3], out_rect.top() - TEXT_MARGIN, "DIST", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);

      out_rect.sub(0, LINE_HEIGHT);

      // draw ship list


//      int32 lines_count = imin((int32)mmsi.size(), y_coord / LINE_HEIGHT);
      int32 mid;
      for (size_t i = 0; i < mmsi.size(); i++)
      {
            mid = vessels[mmsi[i]].mid;
            if (mid_list.count(mid) != 0)
            {
                  std::string ccode = mid_list[mid].code;
                  if (mid_country.count(ccode) != 0)
                  {
                        //image * img = &mid_country[ccode];
                        screen->draw_image(&mid_country[ccode], out_rect.left() + 2, out_rect.top(), VALIGN_TOP | HALIGN_LEFT);
                  }
            }
            else
            { // no mid info found, just out mid code
                  screen->draw_text(FONT_NORMAL, out_rect.left() + headers[0], out_rect.top() - TEXT_MARGIN, string_format("%d", mid), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
            }
            /*
                      struct mid_struct_s
                      {
                            std::string code, country;
                      };

                      extern std::map<int, mid_struct_s> mid_list;
                      extern std::map<std::string, image> mid_country;
                      */

            screen->draw_text(FONT_NORMAL, out_rect.left() + headers[1], out_rect.top() - TEXT_MARGIN, string_format("%d", mmsi[i]), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
            screen->draw_text(FONT_NORMAL, out_rect.left() + headers[2], out_rect.top() - TEXT_MARGIN, vessels[mmsi[i]].shipname, VALIGN_TOP | HALIGN_LEFT, clBlack, clNone);
            screen->draw_text(FONT_NORMAL, out_rect.right() + headers[3], out_rect.top() - TEXT_MARGIN, string_format("%d", vessels[mmsi[i]].distance), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);

            touchman->add_rect(TOUCH_GROUP_SHIPLIST, mmsi[i], out_rect);
            out_rect.sub(0, LINE_HEIGHT);
            if (out_rect.bottom() < SHIPLIST_RECT.bottom())
                  break;
      }

}
void draw_track_info(int32 x, int32 y)
{// draw last tracking
      int32 col_pos[6] = { 0,75,150,250,350 };
      int32 sessid, time, dist;
      //uint64_t timestart, timeend;      double dist;
      // tracks header
      screen->draw_text(FONT_NORMAL, x + col_pos[1], y, "ID", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[2], y, "sec", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      screen->draw_text(FONT_NORMAL, x + col_pos[3], y, "dist", VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
      y -= 4 + screen->get_font_height(FONT_NORMAL);

      if (mysql->exec_prepared(PREPARED_GPS_TOTAL))
            return;


      while (mysql->fetch()) {
            sessid = mysql->get_int("sessid");
            time = mysql->get_int("time");
            dist = mysql->get_int("dist");

            screen->draw_text(FONT_NORMAL, x + col_pos[1], y, string_format("#%d", sessid), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[2], y, time_diff(time), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            screen->draw_text(FONT_NORMAL, x + col_pos[3], y, string_format("%d", dist), VALIGN_TOP | HALIGN_RIGHT, clBlack, clNone);
            y -= 4 + screen->get_font_height(FONT_NORMAL);

      }
      //      gps_session_id = driver->get_int("sessid") + 1;

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
      if (!vessels.count(display_win_param)) {
            printf("[i] draw_infowindow_vessel: mmsi %d not found.\n", display_win_param);
            return;
      }
      vessel* v = &vessels[display_win_param];
      int32 x = WINDOW_RECT.left() + 5, y = WINDOW_RECT.top() - 5;
      screen->draw_text(FONT_MONOMEDIUM, x, y, "VESSEL Info", VALIGN_TOP | HALIGN_LEFT, clNone, clNone); y -= 10 + screen->get_font_height(FONT_MONOMEDIUM);
      screen->draw_text(FONT_NORMAL, x, y, string_format("MMSI: %d", display_win_param), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
      screen->draw_text(FONT_NORMAL, x, y, string_format("GPS: %.6f,%.6f", v->gps.x, v->gps.y), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
      screen->draw_text(FONT_NORMAL, x, y, string_format("COG: %f", v->course), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
      screen->draw_text(FONT_NORMAL, x, y, string_format("Heading: %f", v->heading), VALIGN_TOP | HALIGN_LEFT, clBlack, clNone); y -= 4 + screen->get_font_height(FONT_NORMAL);
}
void draw_infowindow()
{
      if (display_win_mode == DISPLAY_WIN_NONE)
            return;
      // draw window
      //const int32 lh = 12;
      screen->fill_rect(WINDOW_RECT, clWhite | clTransparency12);
      screen->rectangle(WINDOW_RECT, clBlack);

      if (display_win_mode == DISPLAY_WIN_INFO)
      {// global info (time/sattelites etc)
            switch (display_win_param)
            {
                  case 0:draw_infowindow_global(); break;
                  case 1:draw_timewindow_global(); break;
            }
      }
      else if (display_win_mode == DISPLAY_WIN_VESSEL)
      {
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
            draw_grid(own_vessel.relative() ? own_vessel.heading() : 0.0);
            draw_vessels(0xcd8183, clBlack);
            draw_vessels_info();

            // plus and minus images
            screen->draw_image(img_minus, ZOOM_BTN_MARGIN, screen->height() - ZOOM_BTN_MARGIN, HALIGN_LEFT | VALIGN_TOP, 150);
            screen->draw_image(img_plus, SHIPLIST_RECT.left() - ZOOM_BTN_MARGIN, screen->height() - ZOOM_BTN_MARGIN, HALIGN_RIGHT | VALIGN_TOP, 150);

            draw_infoline();

            screen->draw_text(FONT_NORMAL,
                  SHIPLIST_RECT.left() + 5,
                  SHIPLIST_RECT.bottom() + 5,
                  string_format("ZOOM #%d %f", zoom_index, zoom), VALIGN_BOTTOM | HALIGN_LEFT, clBlack, clNone);

            draw_infowindow();
      }
      catch (...)
      {
            std::cerr << "[E] draw_frame" << std::endl;
      }
}
////////////////////////////////////////////////////////////
int32 process_touches() {

      std::string msg;
      int32 group_id, area_id;
      touches_coords_s t;
      //touchman->check_point(30, 300, gi, name);
      while (touchscr->pop(t)) {
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
                              display_win_mode = DISPLAY_WIN_INFO;
                              display_win_param = area_id;

                              // deactivate all touch groups,except window group
                              touchman->set_groups_active(0);
                              if (touchman->set_group_active(TOUCH_GROUP_INFOWINDOW, 1))
                                    printf("[TOUCH] error set active %d to group %d", 1, TOUCH_GROUP_INFOWINDOW);
                              break;
                        }
                        case TOUCH_GROUP_INFOWINDOW:
                        { // hide info window
                              display_win_mode = DISPLAY_WIN_NONE;
                              // enable all touch groups, disable window group
                              touchman->set_groups_active(1);
                              if (touchman->set_group_active(TOUCH_GROUP_INFOWINDOW, 0))
                                    printf("[TOUCH] error set active %d to group %d", 0, TOUCH_GROUP_INFOWINDOW);
                              break;
                        }
                        case TOUCH_GROUP_SHIPLIST:
                        {// show vessel info window
                              display_win_mode = DISPLAY_WIN_VESSEL;
                              display_win_param = area_id;
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
      return 0;




}
////////////////////////////////////////////////////////////
void video_loop_start() {
      /*
      const int FPS = 60;
      unsigned long long ns_delay = 1000000000L / FPS, diff, frames_total = 0;
      struct timespec frame_start, frame_end, measure_start;

      struct timespec sleep_timer = { 0,0 }, sleep_dummy;
        */



        //int frames = 0;

      std::string fps_str = "";
      bool update_required;

      while (true)
      {
            update_required = false;
            update_required |= update_time() > 0;
            update_required |= update_nmea() > 0;
            update_required |= update_map_data(next_map_update) > 0;
            update_required |= process_touches() > 0;

            if (update_required)
            {
                  draw_frame();
                  touchman->debug(screen);
                  screen->flip();
            }
            //            frames++;
            //fps_str = "none";            screen->draw_text(FONT_NORMAL, 5, 5, fps_str, 0xFFFF00, VALIGN_BOTTOM | HALIGN_LEFT);
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
      screen = new video_driver(buff, 1); // debug purpose = 1 buffer, production value = 5
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
      screen->load_font(FONT_NUMS, data_path("/img/nums.png"));
      screen->set_font_interval(FONT_MONOMEDIUM, 2);


      img_minus = new image(data_path("/img/minus_v2.png"));
      img_plus = new image(data_path("/img/plus.png"));
      min_fit = imin(CENTER.x, CENTER.y) - 20;
      circle_radius = min_fit / 5;
      map_update_coeff = ZOOM_RANGE[max_zoom_index - 1] / min_fit * 1.05;
}
void init_touch(CSimpleIniA* ini) {

      touchscr = new   touchscreen(ini, screen->width(), screen->height());
      touchman = new touch_manager();

      touchman->add_group(TOUCH_GROUP_ZOOM, 15);
      IntRect rct;
      rct = { ZOOM_BTN_MARGIN,
            screen->height() - ZOOM_BTN_MARGIN - img_minus->height(),
            ZOOM_BTN_MARGIN + img_minus->width(),
            screen->height() - ZOOM_BTN_MARGIN
      };
      touchman->add_circle(TOUCH_GROUP_ZOOM, TOUCH_AREA_ZOOMOUT, rct.center(), 40);

      rct = { SHIPLIST_RECT.left() - ZOOM_BTN_MARGIN - img_plus->width(),
            screen->height() - ZOOM_BTN_MARGIN - img_plus->height(),
            SHIPLIST_RECT.left() - ZOOM_BTN_MARGIN,
            screen->height() - ZOOM_BTN_MARGIN };
      touchman->add_circle(TOUCH_GROUP_ZOOM, TOUCH_AREA_ZOOMIN, rct.center(), 40);


      touchman->add_group(TOUCH_GROUP_SHIPSHAPE, 5);
      touchman->add_group(TOUCH_GROUP_SHIPLIST, 12);
      //touchman->add_rect(TOUCH_GROUP_SHIPLIST, "test", { 10,20,30,40 });

      touchman->add_group(TOUCH_GROUP_INFOLINE, 10);
      //  touchman->add_rect(TOUCH_GROUP_INFOLINE, 0, { SCREEN_RECT.left(),SCREEN_RECT.bottom(),CENTER_X ,40 });
  //      touchman->add_rect(TOUCH_GROUP_INFOLINE, 1, { CENTER_X,SCREEN_RECT.bottom(),SCREEN_RECT.right(),40 });

      touchman->add_group(TOUCH_GROUP_INFOWINDOW, 30, 0);
      touchman->add_rect(TOUCH_GROUP_INFOWINDOW, 0, WINDOW_RECT);

      touchman->set_enabled(1); // tmp!!!
}
void init_database() {
      mysql = new mysql_driver("127.0.0.1", "map_reader", "map_reader", "ais");
      init_db(mysql);
      //mysql->exec_file(data_path("/sql/test/test.sql"));

      /*if (mysql->get_last_error_str()) {
            printf("mysql init error.\n%s\n", mysql->get_last_error_str());
            return;

      }

      */
}


int main()
{
      try {
            CSimpleIniA ini;
            std::string ini_filename = data_path("/options.ini");
            SI_Error rc = ini.LoadFile(ini_filename.c_str());
            if (rc < 0)
            {
                  printf("Error while loading ini-file: %s\n", ini_filename.c_str());
                  return EXIT_FAILURE;
            }



            const char* buff;
            buff = ini.GetValue("main", "video", "/dev/fb0");
            init_video(buff);
            init_touch(&ini);
            prepare_infoline(SHIPLIST_RECT.left());
            init_database();
            init_sock(&ini);

            // simulate GPS pos
            own_vessel.set_pos({ 21.470805989743354,60.16783241839092 }, position_type_e::gll);
            map_update_coeff = 1;



            zoom_changed(zoom_index);
            next_map_update = 0;


            //touchscr->simulate_click(360, 20);// simulate info bar click


            video_loop_start();
            return 0;
      }


      catch (sql::SQLException& e) {
            std::cout << "# ERR: SQLException in " << __FILE__;
            std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
            std::cout << "# ERR: " << e.what();
            std::cout << " (MySQL error code: " << e.getErrorCode();
            std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
      }
      catch (const std::runtime_error& re)
      {
            // speciffic handling for runtime_error
            std::cerr << "[E] Runtime error: " << re.what() << std::endl;
      }
      catch (const std::exception& ex)
      {
            // speciffic handling for all exceptions extending std::exception, except
            // std::runtime_error which is handled explicitly
            std::cerr << "[E] Error occurred: " << ex.what() << std::endl;
      }
      catch (...)
      {
            // catch any other errors (that we have no information about)
            std::cerr << "[E] Unknown failure occurred. Possible memory corruption" << std::endl;
      }


      return 0;
}
