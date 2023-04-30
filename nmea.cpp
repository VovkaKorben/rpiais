#include <string>
#include <iostream>
#include <map>
//#include <mysql/jdbc.h>
#include <vector>
#include <algorithm>

#include "nmea.h"

//using namespace std;



double dm_s2deg(double dm_s)
{
      int	int_part = (int)trunc(dm_s);
      return (int_part / 100 + (int_part % 100 + (dm_s - int_part)) / 60);
}


map<int, vector<vdm_field> > vdm_defs;
map<int, int> vdm_length;

vessels_class vessels(0);
bool vessels_class::find_by_mmsi(uint32 mmsi, size_t & index)
{
      for (index = 0; index < items.size(); index++)
            if (items[index].mmsi == mmsi)
                  return true;
      return false;
}
vessel * vessels_class::get_own()
{
      return &(items.at(0));
}
vessel * vessels_class::get_by_mmsi(uint32 mmsi)
{
      size_t i;
      if (!find_by_mmsi(mmsi, i))
      { // add a new one
            vessel v(mmsi);
            items.push_back(v);
            i = items.size() - 1;
      }
      return &items[i];
}
vessels_class::vessels_class(uint32 self_mmsi)
{
      items.clear();
      // push own vessel
      vessel v(self_mmsi);
      items.push_back(v);
}


struct msg_bulk {
      size_t total;
      map<int, StringArray> messages;
      msg_bulk()
      {
            total = 0;
            messages.clear();
      };
};
typedef map<std::string, msg_bulk> groups;
groups buff;

bool check_buff(std::string sentence, StringArray data, StringArrayBulk * bulk)
{

      int group_id, message_id, message_total;
      if (!parse_int(data[0], message_total))
            message_total = 1;
      if (!parse_int(data[1], message_id))
            message_id = 1;

      // only VDM sentence has a group ID, another sentences has only one group and assume it 0
      if (sentence == "VDM")
      {
            if (!parse_int(data[2], group_id))
                  group_id = 0;
            data.erase(data.begin(), data.begin() + 3);
      }
      else
      {
            group_id = 0;
            data.erase(data.begin(), data.begin() + 2);
      }

      // check group exists, create if not
      string group_name = string_format("%s_%d", sentence, group_id);
      if (buff.count(group_name) == 0)
      {
            msg_bulk group;
            //group = new msg_bulk();
            buff[group_name] = group;
            buff[group_name].total = message_total;
      }

      // paste new data to group
      if (buff[group_name].messages.count(message_id) != 0)
      {
            // warn: message with same ID already exists in group
      }
      buff[group_name].messages[message_id] = data;

      // check group is full
      if (buff[group_name].messages.size() == buff[group_name].total)
      {
            // push all collected messages
            for (auto const & msg : buff[group_name].messages)
                  (*bulk).push_back(msg.second);

            // delete collected group
            buff.erase(group_name);
            //print_buff();
            return true;
      }
      else return false;
}

map<int, satellite> sattelites;
own_vessel_class own_vessel;


bool parse_char(const std::string & s, char & c)
{
      if (s.length() != 1)
            return false;
      c = s[0];
      // uppercase
      if (c >= 97 && c <= 122)
            c -= 32;
      return true;
}
bool parse_int(const std::string & s, int & i)
{
      if (s.length() == 0)
            return false;
      i = 0;
      for (size_t p = 0; p < s.length(); p++) {
            if (isdigit(s[p]))
                  i = i * 10 + (s[p] - 48);
            else
                  return false;
      }
      return true;

}
bool parse_double(const std::string & s, double & f)
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

int _gga(StringArrayBulk * data)
{
      return 0;
}
int _gsa(StringArrayBulk * data)
{
      return 0;
}
int _gll(StringArrayBulk * data)
{
      char data_valid;
      StringArray * d = &(data->at(0));
      if (parse_char(d->at(5), data_valid))
            if (data_valid == 'A')
            {
                  vessel * ownship = vessels.get_own();
                  char lon_dir, lat_dir;

                  if (parse_double(d->at(0), ownship->lat) && parse_char(d->at(1), lat_dir))
                  {
                        if (lat_dir == 'S')
                              ownship->lat = -ownship->lat;
                        ownship->flags |= VESSEL_Y;
                  }
                  if (parse_double(d->at(2), ownship->lon) && parse_char(d->at(3), lon_dir))
                  {
                        if (lon_dir == 'W')
                              ownship->lon = -ownship->lon;
                        ownship->flags |= VESSEL_X;
                  }
                  return 0;
            }
      return 1;
}
int _rmc(StringArrayBulk * data)
{

      char data_valid;
      StringArray * d = &(data->at(0));
      if (parse_char(d->at(5), data_valid))
            if (data_valid == 'A')
            {
                  vessel * ownship = vessels.get_own();
                  char lon_dir, lat_dir;

                  if (parse_double(d->at(0), ownship->lat) && parse_char(d->at(1), lat_dir))
                  {
                        if (lat_dir == 'S')
                              ownship->lat = -ownship->lat;
                        ownship->flags |= VESSEL_Y;
                  }
                  if (parse_double(d->at(2), ownship->lon) && parse_char(d->at(3), lon_dir))
                  {
                        if (lon_dir == 'W')
                              ownship->lon = -ownship->lon;
                        ownship->flags |= VESSEL_X;
                  }
                  return 0;
            }
      return 1;
}
int _gsv(StringArrayBulk * data)
{
      bool first = true;
      int sat_count, prn;
      size_t  pos;
      for (StringArray s : *data)
      {
            int tmp_count;
            parse_int(s.at(0), tmp_count);
            if (first)
            {
                  first = false;
                  sat_count = tmp_count;
            }
            else {
                  if (sat_count != tmp_count)
                  {
                        cout << "warn satellites count differ" << endl;
                  }
            }
            pos = 1;
            while ((pos + 4) < s.size())
            {
                  parse_int(s.at(pos), prn);
                  sattelites[prn].last_access = utc_ms();
                  parse_int(s.at(pos + 1), sattelites[prn].elevation);
                  parse_int(s.at(pos + 2), sattelites[prn].azimuth);
                  parse_int(s.at(pos + 3), sattelites[prn].snr);
                  pos += 4;
            }

      }
      return 0;
}
int _vdm(StringArrayBulk * data)
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
                        cout << "warn channel differs in one group" << endl;
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
            cout << "Unkn msg len, id: " << msg_id << endl;

            return -1;
      }
      else
            if (vdm_length[msg_id] != 0 && vdm_length[msg_id] != bc.get_len())
            {
                  cout << string_format("Invalid msg #%d len, %d expect, %d got.", msg_id, vdm_length[msg_id], bc.get_len()) << endl;
                  return -1;
            }
      if (vdm_defs.count(msg_id) == 0)
      {
            cout << string_format("No fields defined for msg #%d", msg_id) << endl;

            return -1;
      }

      // parsing fields
      int32 mmsi;
      vessel * v = nullptr;

      for (vdm_field f : vdm_defs[msg_id])
      {
            //cout << f.field_name << endl;

            if (f.field_name == "MMSI")
            {
                  mmsi = bc.get_int(&f);
                  if (mmsi != 0)
                        v = vessels.get_by_mmsi(mmsi);
            }
            else if (v != nullptr)
            {

                  if (f.field_name == "LAT")
                  {
                        int t = bc.get_int(&f);
                        if (t != LAT_DEFAULT)
                        {
                              v->lat = t / 600000.0;
                              v->flags |= VESSEL_Y;
                        }
                  }
                  else
                        if (f.field_name == "LON")
                        {
                              int t = bc.get_int(&f);
                              if (t != LON_DEFAULT)
                              {
                                    v->lon = t / 600000.0;
                                    v->flags |= VESSEL_X;
                              }
                        }
                        else
                              if (f.field_name == "IMO")
                                    v->imo = bc.get_int(&f);
                              else
                                    if (f.field_name == "CALLSIGN")
                                          v->callsign = bc.get_string(&f);
                                    else
                                          if (f.field_name == "SHIPNAME")
                                                v->shipname = bc.get_string(&f);
                                          else
                                                if (f.field_name == "SHIPTYPE")
                                                      v->shiptype = bc.get_int(&f);
                                                else
                                                      if (f.field_name == "TO_BOW")
                                                      {
                                                            v->T = bc.get_int(&f);
                                                            v->flags |= VESSEL_T;
                                                            v->size_changed();
                                                      }
                                                      else
                                                            if (f.field_name == "TO_STERN")
                                                            {
                                                                  v->B = bc.get_int(&f);
                                                                  v->flags |= VESSEL_B;
                                                                  v->size_changed();
                                                            }
                                                            else
                                                                  if (f.field_name == "TO_PORT")
                                                                  {
                                                                        v->L = bc.get_int(&f);
                                                                        v->flags |= VESSEL_L;
                                                                        v->size_changed();
                                                                  }
                                                                  else
                                                                        if (f.field_name == "TO_STARBOARD")
                                                                        {
                                                                              v->R = bc.get_int(&f);
                                                                              v->flags |= VESSEL_R;
                                                                              v->size_changed();
                                                                        }
                                                                        else
                                                                              if (f.field_name == "SOG")
                                                                                    v->speed = bc.get_double(&f);
                                                                              else
                                                                                    if (f.field_name == "COG")
                                                                                          v->course = bc.get_double(&f);
                                                                                    else
                                                                                          if (f.field_name == "HDG")
                                                                                          {
                                                                                                v->heading = bc.get_int(&f);
                                                                                                v->flags |= VESSEL_A;
                                                                                          }
                                                                                          else
                                                                                                cout << "No handler for field: " << f.field_name << endl;
            }
      }
      return 0;
}

map<std::string, nmea_talker> talkers;
map<std::string, nmea_sentence> sentences;
map<std::string, FnPtr> lut = {
      {"GGA",_gga},
      {"GSA",_gsa},
      {"RMC",_rmc},
      {"GSV",_gsv},
      {"VDM",_vdm},
      {"GLL",_gll},
};


unsigned int parse_nmea(std::string nmea_str)
{
      unsigned int r = NMEA_GOOD;
      if (nmea_str.length() == 0)
            return (r | NMEA_EMPTY);
      if (nmea_str[0] != '!' && nmea_str[0] != '$')
            r |= NMEA_WRONG_HEADER;

      // process asterisk
      size_t  asterisk_pos = nmea_str.find_first_of('*');
      if (asterisk_pos == string::npos)
            return (r | NMEA_NO_ASTERISK);
      if (asterisk_pos >= (nmea_str.length() - 2))
            return (r | NMEA_NO_CHECKSUM);

      // calc checksum
      unsigned char cs = 0;
      size_t pos = 1;
      while (pos < asterisk_pos)
            cs ^= nmea_str[pos++];
      string calculated_cs = string_format("%.2X", cs),
            origin_cs = nmea_str.substr(asterisk_pos + 1, 2);
      if (calculated_cs != origin_cs)
            r |= NMEA_BAD_CHECKSUM;


      // explode string to array
      nmea_str = nmea_str.substr(1, asterisk_pos - 1);
      StringArray data = str_split(nmea_str.c_str());
      if (data.size() == 0)
            return NMEA_NO_DATA;

      // extract header
      string header = data[0];
      std::transform(header.begin(), header.end(), header.begin(), ::toupper);

      // extract talker and sentence from header
      string _sentence = header.substr(header.length() - 3, 3),
            _talker = header.substr(0, header.length() - 3);
      if (talkers.count(_talker) == 0)
            r |= NMEA_UNKNOWN_TALKER;
      if (sentences.count(_sentence) == 0)
            return (r | NMEA_UNKNOWN_SENTENCE);

      // push data to buffer, for non-grouped sentences - instantly process them
      data.erase(data.begin());
      StringArrayBulk bulk;
      if (sentences[_sentence].grouped)
      {
            if (check_buff(_sentence, data, &bulk))
            {
                  r |= sentences[_sentence].handler(&bulk);
                  /*if (r)
                  {
                        cout << id << "\n";
                        print_data(&bulk);
                        system("pause");
                  }*/
            }
      }
      else
      {
            //	bulk = new StringArrayBulk;
            bulk.push_back(data);
            r |= sentences[_sentence].handler(&bulk);
      }


      return r;
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
            cout << string_format("bitcollector->add_bits, value %d (%.2f bits) exceeds size maximum allowed %d (%d bits)", data, log2(data), test_bit - 1, data_len) << "\n";
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
int32 bitcollector::get_int(const vdm_field * f)
{
      int32 r = get_raw(f->start, f->len);
      uint32  sign_bit = 1 << (f->len - 1);
      if (f->type == 1 && (r & sign_bit))
            r = (r & ~sign_bit) | 0x80000000;
      return r;
}
double bitcollector::get_double(const vdm_field * f)
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
std::string bitcollector::get_string(const vdm_field * f)
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
      for (auto & ch : data)
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
StringArray bitcollector::create_vdm(int & group_id)
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
                  cs ^= c;
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

