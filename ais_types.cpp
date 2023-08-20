#include "ais_types.h"
#include "video.h"

void IntPoint::transform(const PolarPoint& transform_value) {
      IntPoint ip(x, y);
      //ip.sub(center);
      PolarPoint pp = ip.to_polar();
      pp.angle += transform_value.angle;
      pp.dist *= transform_value.dist;
      ip = pp.to_int();
      //ip.add(center);
      x = ip.x, y = ip.y;

}
void IntPoint::add(const IntPoint* pt) {
      x += pt->x;
      y += pt->y;
}
void IntPoint::sub(const IntPoint* pt) {
      x -= pt->x;
      y -= pt->y;
}
void IntPoint::add(int32 _x, int32 _y) {
      x += _x;
      y += _y;
}
std::string IntPoint::dbg() const
{
      return string_format("%d, %d", x, y);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string IntRect::dbg()
{
      return string_format("L:%d, B:%d, R:%d, T:%d", l, b, r, t);
}
void IntRect::collapse(int32 x, int32 y) {
      l += x; r -= x;      b += y; t -= y;
}
void IntRect::zoom(double z) {
      l = (int32)(l * z);
      r = (int32)(r * z);
      t = (int32)(t * z);
      b = (int32)(b * z);
}
void IntRect::add(IntPoint o) {
      l += o.x;
      r += o.x;
      b += o.y;
      t += o.y;
}
void IntRect::add(int32 x, int32 y) {
      l += x;
      r += x;
      b += y;
      t += y;
}
void IntRect::sub(IntPoint o) {
      l -= o.x;
      r -= o.x;
      b -= o.y;
      t -= o.y;
}
void IntRect::sub(int32 x, int32 y) {
      l -= x;
      r -= x;
      b -= y;
      t -= y;
}

void IntRect::sub(FloatPoint o) {
      l -= (int32)o.x;
      r -= (int32)o.x;
      b -= (int32)o.y;
      t -= (int32)o.y;
}
void IntRect::sub(double x, double y) {
      l -= (int32)x;
      r -= (int32)x;
      b -= (int32)y;
      t -= (int32)y;
}

void IntRect::init(IntPoint* pt) {
      l = r = pt->x;      b = t = pt->y;
}
void IntRect::modify(IntPoint * pt) {
      l = imin(l, pt->x);
      b = imin(b, pt->y);
      r = imax(r, pt->x);
      t = imax(t, pt->y);
}
bool IntRect::is_intersect(const IntRect& rct)
{
      return ((l < rct.r) && (rct.l < r) && (b < rct.t) && (rct.b < t));
}
IntPoint IntRect::get_corner(int32 index)
{
      //const int32 cornerX[4] = { 0,2,2,0 };      const int32 cornerY[4] = { 1,1,3,3 };

      switch (index)
      {
            case 0:return { l,b };
            case 1:return { r,b };
            case 2:return { r,t };
            case 3:return { l,t };
            default: return { 0,0 };
      }
}
void IntRect::transform_bounds(const PolarPoint& transform_value) {
      IntRect rct;
      IntPoint ip;
      for (int32 n = 0; n < 4; n++)
      {
            ip = get_corner(n);
            ip.transform(transform_value);
            if (n)
                  rct.modify(&ip);
            else // init rect
                  rct.init(&ip);

      }
      l = rct.l;
      r = rct.r;
      b = rct.b;
      t = rct.t;
}

/*
IntRect IntRect::transform_bounds(const PolarPoint& transform_value) {
      IntRect rct;
      IntPoint ip;
      //printf("center: %s\n", center.dbg().c_str());
      for (int32 n = 0; n < 4; n++)
      {
            ip =   get_corner(n);
            //printf("%s\n", ip.dbg().c_str());
            //printf("pt in: %s\n", ip.dbg().c_str());
            // ip.x = _get_coord(cornerX[n]);            ip.y = _get_coord(cornerY[n]);
            ip.transform(transform_value);
            //printf("%s\n", ip.dbg().c_str());
            if (n)
                  rct.modify(ip.x, ip.y);
            else // init rect
                  rct.init(ip.x, ip.y);

      }
      return rct;
}*/
IntPoint IntRect::center() {
      return { l + (r - l) / 2,b + (t - b) / 2 };
}

double FloatRect::_get_coord(int32 index)
{
      switch (index)
      {
            case 1:return b;
            case 2:return r;
            case 3:return t;
            default:return l;
      }
}
std::string FloatRect::dbg()
{
      return string_format("L:%.2f, B:%.2f, R:%.2f, T:%.2f", l, b, r, t);
}
FloatRect::FloatRect(IntRect rct) {
      l = rct.left();
      b = rct.bottom();
      r = rct.right();
      t = rct.top();
}
void FloatRect::zoom(double z) {
      l *= z;
      r *= z;
      t *= z;
      b *= z;
}
void FloatRect::offset(FloatPoint o) {
      l += o.x;
      r += o.x;
      b += o.y;
      t += o.y;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FloatPoint::transform(const PolarPoint* transform_value) {



      //IntPoint ip = to_int();
      //ip.sub(center);
      PolarPoint pp;
      pp.from_float(this);
      pp.angle += transform_value->angle;
      pp.dist *= transform_value->dist;
      *this = pp.to_float();


}
std::string  FloatPoint::dbg()
{
      return string_format("x:%.7f, y:%.7f", x, y);
}
void FloatPoint::latlon2meter() // in format(lon, lat)
{
      //double tx = x / PI * 180;      double ty = y / PI * 180;

      x = (x * 20037508.34) / 180;
      if (std::abs(y) >= 85.051129)
            // The value 85.051129° is the latitude at which the full projected map becomes a square
            y = dsign(y) * std::abs(y) * 111.132952777;
      else
            y = std::log(std::tan(((90 + y) * PI) / 360)) / (PI / 180);
      y = (y * 20037508.34) / 180;
      //return { rx,ry };
}
IntPoint FloatPoint::to_int()
{
      IntPoint ip((int32) std::round(x), (int32)std::round(y));
      return ip;
}
void FloatPoint::add(FloatPoint *fp) {
      x += fp->x;
      y += fp->y;
}
void FloatPoint::sub(FloatPoint *fp) {
      x -= fp->x;
      y -= fp->y;
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
int32 FloatPoint::haversine(FloatPoint *fp) {
      
      double lat_delta = DEG2RAD(fp->y - this->y),
            lon_delta = DEG2RAD(fp->x - this->x),
            converted_lat1 = DEG2RAD(this->y),
            converted_lat2 = DEG2RAD(fp->y);

      double a =
            std::pow(std::sin(lat_delta / 2), 2) + std::cos(converted_lat1) * cos(converted_lat2) * std::pow(std::sin(lon_delta / 2), 2);

      double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
      //double d = EARTH_RADIUS * c;

      return int(EARTH_RADIUS * c);
      //return 0;
}    
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
            tmp.angle = std::atan((double)y / (double)x);
            if (x < 0)
                  tmp.angle += RAD180;
            else if (y < 0)
                  tmp.angle += RAD360;

      }
      tmp.dist = std::sqrt(static_cast<uint64_t>(x) * x + static_cast<uint64_t>(y) * y);
      return tmp;
}
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
void PolarPoint::from_float(const FloatPoint* fp) {
      if (is_zero(fp->x))
      {
            if (fp->y < 0)
                  angle = RAD270;
            else
                  angle = RAD90;
      }
      else
      {
            angle = std::atan(fp->y / fp->x) ;
            if (fp->x < 0)
                  angle += RAD180;
            else if (fp->y < 0)
                  angle += RAD360;

      }
      dist = std::sqrt(fp->x * fp->x + fp->y * fp->y);
}
FloatPoint PolarPoint::to_float() {

      FloatPoint r;
      r.x = dist * std::cos(angle);
      r.y = dist * std::sin(angle);
      return r;
}
void PolarPoint::zoom(double z) {
      dist *= z;
}
void PolarPoint::rotate(double a) {
      angle += a;
}
std::string PolarPoint::dbg()
{
      return string_format("A:%.3f, D:%.3f", angle, dist);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void Poly::add(const IntPoint& a) {
      for (size_t n = 0; n < origin.size(); n++)
      {
            work[n].x += a.x;
            work[n].y += a.y;
      }
}*/
/*
void Poly::transform_points_add(const PolarPoint& transform_value, const IntPoint& pt) {
      //IntRect rct;      IntPoint ip;
      //printf("center: %s\n", center.dbg().c_str());
      //printf("-----------------------------\n");
      for (size_t n = 0; n < origin.size(); n++)
      {
            work[n] = origin[n].transform(transform_value);
            work[n].add(CENTER);
      }
}
void Poly::transform_points_sub(const PolarPoint& transform_value,const IntPoint&pt) {
      //IntRect rct;      IntPoint ip;
      //printf("center: %s\n", center.dbg().c_str());
      //printf("-----------------------------\n");
      for (size_t n = 0; n < origin.size(); n++)
      {
            work[n] = origin[n].transform(transform_value);
            work[n].add(CENTER);
      }
}*/
/*void Poly::copy_origin_to_work() {
      for (size_t n = 0; n < origin.size(); n++)
      {
            work[n] = origin[n].to_int();

      }
}
void Poly::sub(const IntPoint& pt) {
      for (size_t n = 0; n < origin.size(); n++)
      {
            work[n].x -= pt.x;
            work[n].y -= pt.y;
      }
}
void Poly::add(const IntPoint& pt) {
      for (size_t n = 0; n < origin.size(); n++)
      {
            work[n].x += pt.x;
            work[n].y += pt.y;
      }
}
*/
void Poly::transform_points(const PolarPoint* transform_value) {
      //IntRect rct;      IntPoint ip;
      //printf("center: %s\n", center.dbg().c_str());
      //printf("-----------------------------\n");
      for (size_t n = 0; n < origin.size(); n++)
      {
            /*if (premod != nullptr)
                  fp.add(*premod);

            work[n] = origin[n].transform(*transform_value);
            if (postmod != nullptr)
                  work[n].add(*postmod);
                  */
      }
}

//int32 gps_session_id;
satellites sat;
//std::map<int, satellite> satellites;
own_vessel_class own_vessel;
std::map<int32, vessel> vessels;
std::map <int32, map_shape>  map_shapes;
poly_driver_class* polydriver;


