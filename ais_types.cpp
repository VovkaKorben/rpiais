#include "ais_types.h"

const int32 cornerX[4] = { 0,2,0,2 };
const int32 cornerY[4] = { 1,1,3,3 };
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PolarPoint IntPoint::to_polar() {

      PolarPoint tmp;

      if (x == 0)
      {
            if (y < 0)
                  tmp.angle = RAD270;
            else
                  tmp.angle = RAD90;
      }
      else
      {
            tmp.angle = std::atan(y / x);
            if (x < 0)
                  tmp.angle += RAD180;
            else if (y < 0)
                  tmp.angle += RAD360;

      }
      tmp.dist = std::sqrt(x * x + y * y);
      return tmp;
}
IntPoint IntPoint::transform(PolarPoint pp) {
      PolarPoint tmp = this->to_polar();
      tmp.add(pp);
      return tmp.to_int();
}
void IntPoint::add(IntPoint pt) {
      x += pt.x;
      y += pt.y;
}
void IntPoint::add(int32 _x, int32 _y) {
      x += _x;
      y += _y;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 IntRect::_get_coord(int32 index)
{
      switch (index)
      {
            case 1:return b;
            case 2:return r;
            case 3:return t;
            default:return l;
      }
}
void IntRect::collapse(int32 x, int32 y) {
      l += x; r -= x;      b += y; t -= y;
}
void IntRect::init(int32 x, int32 y) {
      l = r = x;      b = t = y;
}
void IntRect::modify(int32 x, int32 y) {
      l = imin(l, x);
      b = imin(b, y);
      r = imax(r, x);
      t = imax(t, y);
}
bool IntRect::is_intersect(IntRect* rct)
{
      return false;
}
IntRect IntRect::transform(PolarPoint pp) {
      IntRect rct;
      IntPoint ip;
      for (int32 n = 0; n < 4; n++)
      {
            ip.x = _get_coord(cornerX[n]);
            ip.y = _get_coord(cornerY[n]);
            ip.transform(pp);
            if (n)
                  rct.modify(ip.x, ip.y);
            else // init rect
                  rct.init(ip.x, ip.y);

      }
      return rct;
}
/*void IntRect::make(int32 left, int32 bottom, int32 right, int32 top)
{
      c[0] = left; c[1] = bottom; c[2] = right; c[3] = top;
}*/
IntPoint IntRect::center() {
      return { (r - l) / 2,(t - b) / 2 };
}
//void IntRect::collapse(const int32 x, const int32 y){}
//IntRect::IntRect(int32 l, int32 b, int32 r, int32 t)
//{      c[0] = l; c[1] = b; c[2] = r; c[3] = t;}
//void IntRect::collapse(const int32 v) {      collapse(v, v);}
//void IntRect::collapse(const int32 x, const int32 y) {      c[0] -= x; c[2] += y;      c[1] += y; c[3] -= y;}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FloatPoint FloatPoint::latlon2meter() // in format(lon, lat)
{
      double rx = (x * 20037508.34) / 180, ry;
      if (std::abs(y) >= 85.051129)
            // The value 85.051129° is the latitude at which the full projected map becomes a square
            ry = dsign(y) * std::abs(y) * 111.132952777;
      else
            ry = std::log(std::tan(((90 + y) * PI) / 360)) / (PI / 180);
      ry = (ry * 20037508.34) / 180;
      return { rx,ry };
}
IntPoint FloatPoint::to_int()
{
      return { (int32)x,(int32)y };
}
void FloatPoint::substract(FloatPoint fp) {
      x += fp.x;
      y += fp.y;
}
PolarPoint FloatPoint::to_polar() {
      PolarPoint tmp;
      if (is_zero(x))
      {
            if (y < 0)
                  tmp.angle = RAD270;
            else
                  tmp.angle = RAD90;
      }
      else
      {
            tmp.angle = std::atan(y / x);
            if (x < 0)
                  tmp.angle += RAD180;
            else if (y < 0)
                  tmp.angle += RAD360;

      }
      tmp.dist = std::sqrt(x * x + y * y);
      return tmp;
}
int32 FloatPoint::haversine(FloatPoint fp) {
      return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void _check_angle_correct;
IntPoint PolarPoint::to_int()
{
      IntPoint r;
      r.x = (int32)(dist * std::cos(angle));
      r.y = (int32)(dist * std::sin(angle));
      return r;
}
void PolarPoint::add(PolarPoint pp)
{
      dist += pp.dist;
      angle += pp.angle;
      /*while (angle >= RAD360)
            angle -= RAD360;
      while (angle < RAD0)
            angle += RAD360;*/

}
void PolarPoint::from_float(FloatPoint* fp) {
      if (is_zero(fp->x))
      {
            if (fp->y < 0)
                  angle = 270.0;
            else
                  angle = 90.0;
      }
      else
      {
            angle = std::atan(fp->y / fp->x) / PI * 180.0;
            if (fp->x < 0)
                  angle += 180.0;
            else if (fp->y < 0)
                  angle += 360.0;

      }
      dist = std::sqrt(fp->x * fp->x + fp->y * fp->y);
}
void PolarPoint::zoom(double z) {
      dist *= z;
}
void PolarPoint::rotate(double a) {
      angle += a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Poly::Poly()
{
      origin.clear();
      path_ptr.clear();
}
IntRect Poly::transform_bounds(PolarPoint pp) {
      return bounds.transform(pp);
}
IntRect Poly::get_bounds(PolarPoint pp) {
      return bounds.transform(pp);
}
void Poly::add_point(double x, double y) {

      origin.push_back({ x,y });

      if (origin.size() == 1)
            bounds.init((int32)x, (int32)y);
      else
            bounds.modify((int32)x, (int32)y);
}
void Poly::add_path() {
}
IntRect Poly::get_bounds()
{
      return IntRect();
}
void Poly::transform(PolarPoint pp, FloatPoint offset) {
}




void _insertionSort(bucketset* b) {
      double _fx, _s;
      int j, _y, _ix;
      for (int i = 1; i < b->cnt; i++)
      {
            _y = b->barr[i].y;
            _fx = b->barr[i].fx;
            _ix = b->barr[i].ix;
            _s = b->barr[i].slope;
            j = i - 1;
            while ((j >= 0) && (_ix < b->barr[j].ix))
            {
                  b->barr[j + 1].y = b->barr[j].y;
                  b->barr[j + 1].fx = b->barr[j].fx;
                  b->barr[j + 1].ix = b->barr[j].ix;
                  b->barr[j + 1].slope = b->barr[j].slope;
                  j--;
            }
            b->barr[j + 1].y = _y;
            b->barr[j + 1].fx = _fx;
            b->barr[j + 1].ix = _ix;
            b->barr[j + 1].slope = _s;
      }
}
void Poly::edge_tables_reset()
{
      aet.cnt = 0;
      for (int i = 0; i < _height; i++)
            et[i].cnt = 0;
}
void Poly::edge_store_tuple_float(bucketset* b, int y_end, double  x_start, double  slope)
{
      b->barr[b->cnt].y = y_end;
      b->barr[b->cnt].fx = x_start;
      b->barr[b->cnt].slope = slope;
      b->cnt++;
      //	_insertionSort(b);

}
void Poly::edge_store_tuple_int(bucketset* b, int y_end, double  x_start, double  slope)
{
      //if (dbg_flag)		cout << string_format("+ Added\ty: %d\tx: %.3f\ts: %.3f\n", y_end, x_start, slope);
      b->barr[b->cnt].y = y_end;
      b->barr[b->cnt].fx = x_start;
      b->barr[b->cnt].ix = int(round(x_start));
      b->barr[b->cnt].slope = slope;
      b->cnt++;
      //	_insertionSort(b);

}
void Poly::edge_store_table(IntPoint pt1, IntPoint pt2) {
      double dx, dy, slope;

      // if both points lies below or above viewable rect - edge skipped
      if ((pt1.y < 0 and pt2.y < 0) || (pt1.y >= _height and pt2.y >= _height))
            return;
      dy = pt1.y - pt2.y;

      // horizontal lines are not stored in edge table
      if (dy == 0)
            return;
      dx = pt1.x - pt2.x;

      if (is_zero(dx))
            slope = 0.0;
      else
            slope = dx / dy;

      //check if one point lies below view rect - recalculate intersection with zero scanline
      if (pt1.y < 0)
      {
            pt1.x = int(round(pt1.x - pt1.y * slope));
            pt1.y = 0;
      }
      else if (pt2.y < 0)
      {
            pt2.x = int(round(pt2.x - pt2.y * slope));
            pt2.y = 0;

      }

      int _y, sc;
      double  _x;

      if (dy > 0)
      {
            sc = pt2.y;
            _y = pt1.y;
            _x = pt2.x;
      }
      else
      {
            sc = pt1.y;
            _y = pt2.y;
            _x = pt1.x;
      }


      //if (sc < 0 || sc >= VIEW_HEIGHT)            throw exception("incorrect scanline");
      //cout << string_format("Store tuple:\ty: %d->%d\tx: %.3f\ts: %.3f", sc, _y, _x, slope) << endl;
      edge_store_tuple_float(&et[sc], _y, _x, slope);


}
void Poly::edge_update_slope(bucketset* b) {
      for (int i = 0; i < b->cnt; i++)
      {
            b->barr[i].fx += b->barr[i].slope;
            b->barr[i].ix = int(round(b->barr[i].fx));
      }
}
void Poly::edge_remove_byY(bucketset* b, int scanline_no)
{
      int i = 0, j;
      while (i < b->cnt)
      {
            if (b->barr[i].y == scanline_no)
            {
                  //if (dbg_flag)				cout << string_format("- Removed\ty: %d\tx: %.3f\n", scanline_no, b->barr[i].x);

                  for (j = i; j < b->cnt - 1; j++)
                  {
                        b->barr[j].y = b->barr[j + 1].y;
                        b->barr[j].fx = b->barr[j + 1].fx;
                        b->barr[j].ix = b->barr[j + 1].ix;
                        b->barr[j].slope = b->barr[j + 1].slope;

                  }
                  b->cnt--;
            }
            else
                  i++;
      }
}
////////////////////////////////////////////////////////////
void Poly::calc_fill(const Poly* sh) {

      IntPoint prev_point, point;
      int path_end, point_id = 0;
      for (int path_id = 0; path_id < sh->path_count; path_id++)
      {
            path_end = sh->pathindex[path_id + 1];
            prev_point = sh->work[path_end - 1];
            while (point_id < path_end)
            {
                  point = sh->work[point_id];
                  //cout << "Process line: " << point << " -> " << prev_point << endl;

                  edge_store_table(point, prev_point);
                  prev_point = point;
                  point_id++;
            }

      }
}
void Poly::draw_fill(const ARGB color)
{



      for (int scanline_no = 0; scanline_no < _height; scanline_no++)
      {
            for (int b = 0; b < et[scanline_no].cnt; b++)
            {
                  edge_store_tuple_int(
                        &aet,
                        et[scanline_no].barr[b].y,
                        et[scanline_no].barr[b].fx,
                        et[scanline_no].barr[b].slope);

            }

            edge_remove_byY(&aet, scanline_no);
            _insertionSort(&aet);


            int bucket_no = 0, coordCount = 0, x1, x2;
            while (bucket_no < aet.cnt)
            {
                  if ((coordCount % 2) == 0)
                        x1 = aet.barr[bucket_no].ix;
                  else
                  {
                        x2 = aet.barr[bucket_no].ix;
                        draw_line_fast(scanline_no, x1, x2, color);

                  }
                  coordCount++;
                  bucket_no++;
            }

            edge_update_slope(&aet);
      }

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32 gps_session_id;
satellites sat;
//std::map<int, satellite> satellites;
own_vessel_class own_vessel;
std::map<int32, vessel> vessels;
std::map <int32, map_shape>  map_shapes;



