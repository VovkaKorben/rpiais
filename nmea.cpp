#include <string>
#include <iostream>
#include <map>
//#include <mysql/jdbc.h>
#include <vector>
#include <algorithm>

#include "nmea.h"

//using namespace std;






std::map<int, std::vector<vdm_field> > vdm_defs;
std::map<int, int> vdm_length;

std::map<int, mid_struct_s> mid_list;
std::map<std::string, image> mid_country;
std::map<std::string, StringArrayBulk> msg_buff;

bool check_buff(std::string sentence, StringArray* data, StringArrayBulk* bulk)
{

      int32 group_id, message_id, message_total;
      if (!parse_int(data->at(0), message_total))
            message_total = 1;
      if (!parse_int(data->at(1), message_id))
            message_id = 1;

      // only VDM sentence has a group ID, another sentences has only one group and assume it 0
      if (sentence == "VDM")
      {
            if (!parse_int(data->at(2), group_id))
                  group_id = 0;
            data->erase(data->begin(), data->begin() + 3);
      }
      else
      {
            group_id = 0;
            data->erase(data->begin(), data->begin() + 2);
      }

      // check group exists, create if not
      std::string group_name = string_format("%s_%d", sentence.c_str(), group_id);
      if (msg_buff.count(group_name) == 0)
      {
            msg_buff.emplace(std::piecewise_construct, std::forward_as_tuple(group_name), std::forward_as_tuple());
      }

      // check for message allowed to push (i.e.
      if (msg_buff[group_name].size() == (size_t)(message_id - 1))
      {
            msg_buff[group_name].push_back(*data);
      }

      // check group is full
      if (msg_buff[group_name].size() == (size_t)message_total)
      {
            // push all collected messages
            bulk->clear();
            for (auto const& msg : msg_buff[group_name])
                  (*bulk).push_back(msg);

            // delete collected group
            msg_buff[group_name].clear();
            return true;
      }
      else return false;
}



double dm_s2deg(double dm_s)
{
      int	int_part = (int)trunc(dm_s);
      return (int_part / 100 + (int_part % 100 + (dm_s - int_part)) / 60);
}
bool parse_char(const std::string& s, char& c)
{
      if (s.length() != 1)
            return false;
      c = s[0];
      // uppercase
      if (c >= 97 && c <= 122)
            c -= 32;
      return true;
}
bool parse_int(const std::string& s, int& i)
{
      i = 0;
      if (s.length() == 0)
            return false;
      for (size_t p = 0; p < s.length(); p++) {
            if (isdigit(s[p]))
                  i = i * 10 + (s[p] - 48);
            else
                  return false;
      }
      return true;

}
bool parse_double(const std::string& s, double& f)
{
      if (s.length() == 0)
            return false;

      int int_count = 0, sign = 1;
      unsigned long long int_part = 0, frac_part = 0, frac_div = 1;
      bool has_frac = false, has_sign = false;

      for (size_t p = 0; p < s.length(); p++) {

            if (isdigit(s[p]))
            {
                  int n = s[p] - 48;
                  if (has_frac)
                  {
                        frac_part = frac_part * 10 + n;
                        frac_div *= 10;
                  }
                  else
                  {
                        int_part = int_part * 10 + n;
                        int_count++;
                  }
            }
            else
                  if (s[p] == '.')
                  {
                        if (has_frac)
                              return false;
                        has_frac = true;
                  }
                  else if (s[p] == '-')
                  {
                        if (has_sign || has_frac || int_count > 0)
                              return false;
                        has_sign = true;
                        sign = -1;
                  }
                  else if (s[p] == '+')
                  {
                        if (has_sign)
                              return false;
                        has_sign = true;
                  }
      }


      f = (double)int_part;
      if (has_frac)
            f += (double)frac_part / (double)frac_div;
      if (sign == -1)
            f = -f;
      return true;
}

// posistion
int32 _gll(StringArrayBulk* data, std::string talker)
{
      char data_valid;
      StringArray* d = &(data->at(0));
      if (parse_char(d->at(5), data_valid))
            if (data_valid == 'A')
            {
                  // vessel * ownship = vessels.get_own();
                  char lon_dir, lat_dir;
                  double lon, lat;

                  if (parse_double(d->at(0), lat) && parse_char(d->at(1), lat_dir) &&
                        parse_double(d->at(2), lon) && parse_char(d->at(3), lon_dir))
                  {
                        if (lat_dir == 'S')
                              lat = -lat;
                        if (lon_dir == 'W')
                              lon = -lon;
                  }
                  return 0;
            }
      return 1;
}
int32 _gns(StringArrayBulk* data, std::string talker)
{
      char data_valid;
      StringArray* d = &(data->at(0));
      if (parse_char(d->at(5), data_valid))
            if (data_valid == 'A')
            {
                  // vessel * ownship = vessels.get_own();
                  char lon_dir, lat_dir;
                  double lon, lat;

                  if (parse_double(d->at(0), lat) && parse_char(d->at(1), lat_dir) &&
                        parse_double(d->at(2), lon) && parse_char(d->at(3), lon_dir))
                  {
                        if (lat_dir == 'S')
                              lat = -lat;
                        if (lon_dir == 'W')
                              lon = -lon;
                  }
                  return 0;
            }
      return 1;
}
int32 _rmc(StringArrayBulk* data, std::string talker)
{

      char data_valid;
      StringArray* d = &(data->at(0));
      if (parse_char(d->at(1), data_valid))
            if (data_valid == 'A')
            {
                  //vessel * ownship = vessels.get_own();
                  char lon_dir, lat_dir;
                  double lon, lat;

                  if (parse_double(d->at(0), lat) && parse_char(d->at(1), lat_dir)
                        && parse_double(d->at(2), lon) && parse_char(d->at(3), lon_dir))
                  {
                        if (lat_dir == 'S')
                              lat = -lat;
                        if (lon_dir == 'W')
                              lon = -lon;
                        // own_vessel.set_pos
                  }
                  return 0;
            }
      return 1;
}
int32 _gga(StringArrayBulk* data, std::string talker)
{
      int32 gps_quality;
      StringArray* d = &(data->at(0));
      if (parse_int(d->at(5), gps_quality))
            if (gps_quality > 0)
            {
                  char lon_dir, lat_dir;
                  double lon, lat;

                  if (parse_double(d->at(1), lat) && parse_char(d->at(2), lat_dir) &&
                        parse_double(d->at(3), lon) && parse_char(d->at(4), lon_dir))
                  {
                        if (lat_dir == 'S')
                              lat = -lat;
                        if (lon_dir == 'W')
                              lon = -lon;
                  }
                  return 0;
            }
      return 1;
}
// satellites
int32 //GSA - GPS DOP and active satellites
_gsa(StringArrayBulk* data, std::string talker)
{
      //char selection, fix_mode;
      StringArray* d = &(data->at(0));
      parse_char(d->at(0), own_vessel.sel_mode);
      parse_char(d->at(1), own_vessel.mode);

      IntVec prn_list;
      int32 prn;
      for (int32 s = 0; s < 12; s++)
      {
            if (!parse_int(d->at(s + 2), prn))
                  break;
            prn_list.push_back(prn);
      }
      std::vector<gnss_type> systems_list = sat.get_gnss_type(&prn_list);
      if (systems_list.size() != 1)
      {
            printf("[!] GSA: satellites list from more than one system (%d)", systems_list.size());
            return 1;
      }
      sat.reset_used(systems_list[0]); // reset used flag for entire GNSS type
      sat.set_used(&prn_list); // set used flag for selected PRNs

      return 0;
}


int32 // GSV
_gsv(StringArrayBulk* data, std::string talker)
{
      struct sp
      {
            int32 prn, el, az, snr;
      } tsp;
      IntVec prn_list;
      std::vector<sp> buff;
      int32 first = 1, sat_count, tmp_count, pos;
      for (StringArray s : *data)
      {
            // check all message consistency
            parse_int(s.at(0), tmp_count);
            if (first)
            {
                  first = 0;
                  sat_count = tmp_count;
            }
            else {
                  if (sat_count != tmp_count)
                  {
                        printf("[!] satellites count differ, first message was %d, then %d", sat_count, tmp_count);
                  }
            }

            pos = 1;
            while ((size_t)(pos + 4) <= s.size())
            {
                  parse_int(s.at(pos++), tsp.prn);
                  parse_int(s.at(pos++), tsp.el);
                  parse_int(s.at(pos++), tsp.az);
                  parse_int(s.at(pos++), tsp.snr);
                  /* sattelites[prn].last_access = utc_ms();
                   parse_int(s.at(pos + 1), sattelites[prn].elevation);
                   parse_int(s.at(pos + 2), sattelites[prn].azimuth);
                   parse_int(s.at(pos + 3), sattelites[prn].snr);
                   */
                  buff.push_back(tsp);
                  prn_list.push_back(tsp.prn);
                  //pos += 4;
            }

      }
      std::vector<gnss_type> systems_list = sat.get_gnss_type(&prn_list);
      if (systems_list.size() != 1)
      {
            printf("[!] GSV: satellites list from more than one system (%d)\n", systems_list.size());
            for (auto x : prn_list)
                  printf("%d,", x);
            printf("\n");
            return 1;
      }
      for (auto const& s : buff)
      {
            if (sat.list.count(s.prn) == 0)
            {
                  printf("[!] GSV: satellite #%d not found in main list", s.prn);
                  continue;
            }
            sat.list[s.prn].azimuth = s.az;
            sat.list[s.prn].elevation = s.el;
            sat.list[s.prn].snr = s.snr;
            sat.list[s.prn].last_access = utc_ms();
      }
      return 0;
}
// jump table for VDM fields
enum class field_indexes
{
      mmsi, lat, lon, sog, cog, left, right, top,
      bottom, shiptype, shipname, imo, callsign, hdg
};
std::map<std::string, field_indexes> vdm_case_lut = {
    { "MMSI", field_indexes::mmsi },
    { "LAT", field_indexes::lat },
    { "LON", field_indexes::lon },
    { "SOG", field_indexes::sog },
    { "COG", field_indexes::cog },
    { "HDG", field_indexes::hdg },
    { "SHIPTYPE", field_indexes::shiptype },
    { "SHIPNAME", field_indexes::shipname },
    { "IMO", field_indexes::imo },
    { "CALLSIGN", field_indexes::callsign },
    { "TO_BOW", field_indexes::top },
    { "TO_STERN", field_indexes::bottom },
    { "TO_PORT", field_indexes::left },
    { "TO_STARBOARD", field_indexes::right },
};
// vessels
int _vdm(StringArrayBulk* data, std::string talker)
{
      bitcollector bc;
      int pad_bits;
      bool channel_set = false;
      char channel, test_channel;

      for (StringArray s : *data)
      {
            // check channel equal in all messages
            parse_char(s[0], test_channel);
            if (channel_set)
            {
                  if (channel != test_channel)
                  {
                        // warn channel differs in one group
                        std::cout << "warn channel differs in one group" << std::endl;
                        //print_data(data);				system("pause");
                        return -1;
                  }
            }
            else
            {
                  channel = test_channel;
                  channel_set = true;
            }

            // process data
            parse_int(s[2], pad_bits);
            bc.add_vdm_str(s[1], pad_bits);
      }

      // check vocabs ok
      uint32 msg_id = bc.get_raw(0, 6);
      //cout << string_format("msg id: %d", msg_id) << endl;
      if (vdm_length.count(msg_id) == 0)
      {
            std::cout << "Unkn msg len, id: " << msg_id << std::endl;

            return -1;
      }
      else
            if (vdm_length[msg_id] != 0 && vdm_length[msg_id] != bc.get_len())
            {
                  std::cout << string_format("Invalid msg #%d len, %d expect, %d got.", msg_id, vdm_length[msg_id], bc.get_len()) << std::endl;
                  return -1;
            }
      if (vdm_defs.count(msg_id) == 0)
      {
            std::cout << string_format("No fields defined for msg #%d", msg_id) << std::endl;

            return -1;
      }

      // parsing fields
      int32 mmsi, pos_collect = 0, size_collect = 0;
      vessel* v = nullptr;

      for (vdm_field f : vdm_defs[msg_id])
      {
            if (vdm_case_lut.count(f.field_name) == 0)
                  throw "unkn field";

            field_indexes field_index = vdm_case_lut[f.field_name];
            switch (field_index)
            {
                  case field_indexes::mmsi:
                  {
                        mmsi = bc.get_int(&f);
                        if (vessels.count(mmsi) == 0) // add a new
                        {
                              vessels.emplace(mmsi, vessel());
                        }
                        v = &vessels[mmsi];
                        v->eval_mid(mmsi);
                        break;
                  }
                  case field_indexes::lat:
                  {
                        int t = bc.get_int(&f);
                        if (t != 0x3412140) // magic nmea number, mean `LAT not available`
                        {
                              v->gps.y = t / 600000.0;
                              pos_collect |= 0x01;
                        }
                        break;
                  }
                  case field_indexes::lon:
                  {
                        int t = bc.get_int(&f);
                        if (t != 0x6791AC0)// magic nmea number, mean `LON not available`
                        {
                              v->gps.x = t / 600000.0;
                              pos_collect |= 0x02;
                        }
                        break;
                  }
                  case field_indexes::imo:
                  {
                        v->imo = bc.get_int(&f); break;
                  }
                  case field_indexes::callsign:
                  {
                        v->callsign = bc.get_string(&f); break;
                  }
                  case field_indexes::shipname:
                  {
                        v->shipname = bc.get_string(&f); break;
                  }
                  case field_indexes::shiptype:
                  {
                        v->shiptype = bc.get_int(&f); break;
                  }
                  case field_indexes::top:
                  {
                        v->top = bc.get_int(&f);
                        if (v->top != 0)
                              size_collect |= 0x01;
                        break;
                  }
                  case field_indexes::bottom:
                  {
                        v->bottom = bc.get_int(&f);
                        if (v->bottom != 0)
                              size_collect |= 0x02;
                        break;
                  }
                  case field_indexes::left:
                  {
                        v->left = bc.get_int(&f);
                        if (v->left != 0)
                              size_collect |= 0x04;
                        break;
                  }
                  case field_indexes::right:
                  {
                        v->right = bc.get_int(&f);
                        if (v->right != 0)
                              size_collect |= 0x08;
                        break;
                  }
                  case field_indexes::sog:
                  {
                        v->speed = bc.get_double(&f);
                        break;
                  }
                  case field_indexes::cog:
                  {
                        v->course = bc.get_double(&f);
                        break;
                  }
                  case field_indexes::hdg:
                  {
                        v->heading = bc.get_int(&f);
                        v->angle_ok = (v->heading != 511);
                        break;
                  }
            } // switch



      }

      if (!v->size_ok) { // check size already set
            v->size_ok = (size_collect == 0x0F); // if not set - look at collected values
            if (v->size_ok) // if collected values is OK - update vessel bounds
                  v->size_changed();
      }

      if (!v->pos_ok)
      {
            v->pos_ok = (pos_collect == 0x03);
            printf("mmsi: %d, pos not ok\n", mmsi);
            //v->meters = { v->lon,v->lat };
            //v->meters.latlon2meter();
      }

      return 0;
}
int _vdo(StringArrayBulk* data, std::string talker)
{
      return _vdm(data, talker);
}
std::map<std::string, nmea_talker> talkers;
std::map<std::string, nmea_sentence> sentences;
std::map<std::string, FnPtr> lut = {
      {"GLL",_gll},// GLL - Geographic Position - Latitude / Longitude: prio 1
      {"GNS",_gns},// GNS - Fix data: prio 2
      {"RMC",_rmc}, // RMC - Recommended Minimum Navigation Information, prio 3
      {"GGA",_gga}, // GGA - Global Positioning System Fix Data: prio 4

      {"GSA",_gsa}, // GSA - GPS DOP and active satellites
      {"GSV",_gsv}, // GSV - Satellites in view

      {"VDM",_vdm}, // common vessel messages
       {"VDO",_vdo},

};


int32 parse_nmea(std::string nmea_str)
{
      try {
            //printf("NMEA: %s\n", nmea_str.c_str());
            unsigned int r = NMEA_GOOD;
            if (nmea_str.length() == 0)
                  return (r | NMEA_EMPTY);
            if (nmea_str[0] != '!' && nmea_str[0] != '$')
                  r |= NMEA_WRONG_HEADER;

            // process asterisk
            size_t  asterisk_pos = nmea_str.find_first_of('*');
            if (asterisk_pos == std::string::npos)
                  return (r | NMEA_NO_ASTERISK);
            if (asterisk_pos >= (nmea_str.length() - 2))
                  return (r | NMEA_NO_CHECKSUM);

            // calc checksum
            uint8 cs = 0;
            size_t pos = 1;
            while (pos < asterisk_pos)
                  cs ^= (uint8)nmea_str[pos++];
            std::string calculated_cs = string_format("%.2X", cs),
                  origin_cs = nmea_str.substr(asterisk_pos + 1, 2);
            if (calculated_cs != origin_cs)
                  return  NMEA_BAD_CHECKSUM;


            // explode string to array
            nmea_str = nmea_str.substr(1, asterisk_pos - 1);
            StringArray data = str_split(nmea_str.c_str());
            if (data.size() == 0)
                  return NMEA_NO_DATA;

            // extract header
            std::string header = data[0];
            std::transform(header.begin(), header.end(), header.begin(), ::toupper);

            // extract talker and sentence from header
            std::string _sentence = header.substr(header.length() - 3, 3),
                  _talker = header.substr(0, header.length() - 3);
            if (talkers.count(_talker) == 0)
                  r |= NMEA_UNKNOWN_TALKER;
            if (sentences.count(_sentence) == 0)
                  return (r | NMEA_UNKNOWN_SENTENCE);

#ifndef NDEBUG
            std::string collect = "";
            for (const auto& a : data)
            {
                  collect += a + "`";
            }
            std::cout << collect.c_str() << std::endl;
#endif

            // push data to buffer, for non-grouped sentences - instantly process them
            data.erase(data.begin());
            StringArrayBulk bulk;



            if (sentences[_sentence].grouped)
            { // multiline messages
                  if (check_buff(_sentence, &data, &bulk))
                  {
                        r |= sentences[_sentence].handler(&bulk, _talker);
                  }
            }
            else
            { // for single line messages            
                  bulk.push_back(data);

                  r |= sentences[_sentence].handler(&bulk, _talker);
            }


            return r;
      }
      catch (...)
      {
            std::cerr << "[E] parse_nmea" << std::endl;
            return 404;
      }
}


#define bitcollector_buff_len 150
#define bitcollector_buff_bits bitcollector_buff_len*8
#define bitcollector_vdm_max_payload 56



const char nmea_chars[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&\'()*+,-./0123456789:;<=>?";
int bitcollector::get_char_index(char c)
{
      for (int i = 0; i < 64; i++)
            if (nmea_chars[i] == c)
                  return i;
      return -1;
}

void bitcollector::add_bits(uint32 data, int32 data_len)
{
      if ((length + data_len) > bitcollector_buff_bits)
      {
            // warn buffer underrun
            return;
      }
      uint32 test_bit = 1 << data_len;
      if (data >= test_bit)
      {
            std::cout << string_format("bitcollector->add_bits, value %d (%.2f bits) exceeds size maximum allowed %d (%d bits)", data, log2(data), test_bit - 1, data_len) << "\n";
      }

      test_bit >>= 1;
      uint8 dst_bit = 1 << ((length & 7) ^ 7);
      while (test_bit)
      {
            /*string test_bit_str = int2bin(test_bit);
            string dst_bit_str = int2bin(dst_bit);
            string data_str = int2bin(data);
            */
            if (data & test_bit)
                  buff[length >> 3] |= dst_bit;
            dst_bit >>= 1;
            if (!dst_bit)
                  dst_bit = 0x80;
            test_bit >>= 1;
            length++;
      }
}
void bitcollector::add_string(std::string data, size_t str_len)
{
      int ch;
      for (size_t i = 0; i < data.length(); i++)
      {
            ch = get_char_index(data[i]);
            if (ch == -1)
                  ch = 32;
            add_bits(ch, 6);
      }

      // pad to string len with zeroes
      for (size_t i = data.length(); i < str_len; i++)
            add_bits(32, 6);
}
int bitcollector::get_len() { return length; };
uint32 bitcollector::get_raw(const int32 start, const int32 len)
{
      uint32 r = 0;
      uint8 src_bit = 1 << ((start & 7) ^ 7);
      uint32  dst_bit = 1 << (len - 1);
      int32 src_pos = start >> 3;
      while (dst_bit)
      {
            if (buff[src_pos] & src_bit)
                  r |= dst_bit;

            src_bit >>= 1;
            if (!src_bit)
            {
                  src_bit = 0x80;
                  src_pos++;
            }
            dst_bit >>= 1;

      }
      return r;
}
int32 bitcollector::get_int(const vdm_field* f)
{
      int32 r = get_raw(f->start, f->len);
      uint32  sign_bit = 1 << (f->len - 1);
      if (f->type == 1 && (r & sign_bit))
            r = (r & ~sign_bit) | 0x80000000;
      return r;
}
double bitcollector::get_double(const vdm_field* f)
{
      double r;
      int32 raw = get_raw(f->start, f->len);
      if (f->def == raw)
            r = NAN;
      r = (double)raw;
      if (f->exp)
            r /= (double)(f->exp);
      return r;
}
std::string bitcollector::get_string(const vdm_field* f)
{

      std::string r = "";
      int start = f->start, len = f->len, raw;
      while (len--)
      {
            raw = get_raw(start, 6);
            r += nmea_chars[raw];
            start += 6;
      }


      return r;
}
void bitcollector::add_vdm_str(std::string data, int pad)
{
      uint32 code;
      for (auto& ch : data)
      {

            code = ch - 48;
            if (code > 40) code -= 8;
            add_bits(code, 6);
      }
      length -= pad;
}
void bitcollector::clear()
{
      length = 0;
      for (int i = bitcollector_buff_len - 1; i >= 0; i--)
            buff[i] = 0;
}
StringArray bitcollector::create_vdm(int& group_id)
{

      int payload_size = length / 6;
      if (length % 6)
            payload_size++;
      int messages_count = payload_size / bitcollector_vdm_max_payload;
      if (payload_size % bitcollector_vdm_max_payload)
            messages_count++;

      StringArray r(messages_count);
      int current_msg = 0, pos = 0, space_left = bitcollector_vdm_max_payload;
      uint32 raw;

      while (pos < payload_size)
      {
            raw = get_raw(pos * 6, 6);
            raw += 48;
            if (raw >= 88)
                  raw += 8;
            r[current_msg] += char(raw);
            pos++;
            if (!(--space_left))
            {
                  // add pad

                  current_msg++;
                  space_left = bitcollector_vdm_max_payload;
            }
      }

      std::string group_str = messages_count > 1 ? string_format("%d", group_id) : "";
      uint8 cs;
      int pad;
      for (current_msg = 0; current_msg < messages_count; current_msg++)
      {


            // append header + padding
            if ((current_msg + 1) == messages_count)
                  pad = payload_size * 6 - length;
            else
                  pad = 0;
            r[current_msg] = string_format("AIVDM,%d,%d,%s,A,%s,%d", messages_count, current_msg + 1, group_str, r[current_msg].c_str(), pad);

            // calc checksum
            cs = 0;
            for (char c : r[current_msg])
                  cs ^= (uint8)c;
            r[current_msg] = string_format("!%s*%.2X", r[current_msg].c_str(), cs);

      }
      if (messages_count > 1)
      {
            group_id++;
            if (group_id == 10)
                  group_id = 1;
      }
      return r;
};
bitcollector::bitcollector()
{
      //init_tables();
      clear();

}

