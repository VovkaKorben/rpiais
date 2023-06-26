#include "nmeastreams.h"
#include <thread>
#include <mutex>
#include <linux/input.h>
#include "touch.h"
#include <fcntl.h>
#include "nmea.h"
#include <queue>

#define BUFFLEN 50
#define RECONNECT 5
std::mutex m;
int32 process_buffer(StringQueue * list, pchar buff, int32 len)
{
      int32 search_pos = 0, found_at;
      char  rn[3] = "\r\n", copybuff[BUFFLEN];
      m.lock();
      while (search_pos < len)
      {
            found_at = search_chararray(buff, rn, len, search_pos);

            if (found_at == -1)
                  break;
            copy_chararray(buff, copybuff, search_pos, found_at - search_pos);
           // printf("\t%s\n", copybuff);
            list->push(std::string(copybuff));
            //parse_nmea(std::string(copybuff));
            search_pos = found_at + 2;

      }
      m.unlock();
      if (search_pos < len) // move rest of data to buffer begin
      {
            int32 r = len - search_pos;
            for (int32 c = 0; c < r; c++)
                  buff[c] = buff[search_pos + c];
            return r;
      }
      else return 0;
}

uint32 sock_connect(sockets_s* opt)
{

      int32 status;
      opt->sockfd = socket(AF_INET, opt->type, 0);
      //printf("opt.sockfd: %d\n", opt->sockfd);
      if (opt->sockfd < 0)
      {
            return -1;
      }

      switch (opt->type) {
            case SOCK_DGRAM: {

                  fcntl(opt->sockfd, F_SETFL, O_NONBLOCK);
                  break;
            }
            case SOCK_STREAM: {
                  status = connect(opt->sockfd, (struct sockaddr*)&opt->servaddr, sizeof(opt->servaddr));
                  if (status < 0)
                  {
                        close(opt->sockfd);
                        return -2;
                  }
                  unsigned long nonblocking_long = 0;
                  status = ioctl(opt->sockfd, FIONBIO, &nonblocking_long);
                  if (status < 0)
                  {
                        close(opt->sockfd);
                        return -2;
                  }
            }
      }
      return 0;
}

void rcv_thread(sockets_s opt, StringQueue * list) {
      opt.running = 0;
      memset(&opt.servaddr, 0, sizeof(sockaddr_in));
      opt.servaddr.sin_family = AF_INET;
      opt.servaddr.sin_addr.s_addr = inet_addr(opt.addr);
      opt.servaddr.sin_port = htons(opt.port);
      char buff[BUFFLEN], path_full[256];
      pchar current = buff;
      int32 used = 0;
      sprintf(path_full, "%s://%s:%d\0", opt.type_str, opt.addr, opt.port);
      int32 n;

      while (true) {

            // connect section
            while (sock_connect(&opt))
            {
                  printf("Can't connect to %s (%s). Reconnect in %d second(s).\n",
                        path_full, opt.name, RECONNECT);
                  usleep(RECONNECT * 1000000);
            }
            printf("Connected to %s (%s)\n", path_full, opt.name);

            // read section
            opt.running = 1;
            try {
                  while (true)
                  {

                        // read data
                        switch (opt.type)
                        {
                              case SOCK_DGRAM: {
                                    socklen_t len;
                                    n = (int32)recvfrom(
                                          opt.sockfd,
                                          current,
                                          BUFFLEN - used,
                                          MSG_DONTWAIT,
                                          (struct sockaddr*)&opt.servaddr,
                                          &len
                                    );

                                    break;
                              }
                              case  SOCK_STREAM: {
                                    n = (int32)recv(opt.sockfd, current, BUFFLEN - used, 0);
                                    break;
                              }
                        }
//                        printf("N: %d\n", n);
                        if (n == 0)
                        {
                              opt.running = 0;
                              printf("Connection lost with %s (%s)\n", path_full, opt.name);
                              break;
                        }
                        else if (n > 0)
                        {
                              used = process_buffer(list, buff, used + n);
                              current = buff + used;
                        }
                        else
                              usleep(3000000);
                  }

            }
            catch (...)
            {
                  printf("rcv_thread: exception occured!");
            }
      }
}


nmea_reciever::nmea_reciever(CSimpleIniA* ini, StringQueue *  list) {
      sockets_s socket_opt;
      // socklist.clear();
      threadlist.clear();
      char  section_name[256];
      const char* tmp;
      int  sn = 0;
      do
      {
            sprintf(section_name, "socket%d\0", sn++);
            if (!ini->SectionExists(section_name))
                  break;

            socket_opt.enabled = ini->GetBoolValue(section_name, "enabled", false);
            socket_opt.name = ini->GetValue(section_name, "name", section_name);
            if (!socket_opt.enabled)
            {
                  printf("Socket #%d (%s) disabled in INI. Set enabled=1 for activate\n", sn - 1, socket_opt.name);
                  continue;
            }

            tmp = ini->GetValue(section_name, "type", "");
            int32 len = length_constchararray(tmp) + 1;
            strncpy(socket_opt.type_str, tmp, len);
            lowercase_chararray(socket_opt.type_str, len);
            if (strcmp(socket_opt.type_str, "udp") == 0)
                  socket_opt.type = SOCK_DGRAM;
            else if (strcmp(socket_opt.type_str, "tcp") == 0)
                  socket_opt.type = SOCK_STREAM;
            else {
                  socket_opt.type = -1;
                  printf("Socket #%d (%s) unknown protocol: %s\n", sn - 1, socket_opt.name, socket_opt.type_str);
            }

            if (socket_opt.type != -1) {
                  socket_opt.addr = ini->GetValue(section_name, "addr", "127.0.0.1");
                  socket_opt.port = (uint16)ini->GetLongValue(section_name, "port", 0);

                  threadlist.push_back(
                        std::thread(rcv_thread, socket_opt, list)
                  );
            }

      } while (true);
}

nmea_reciever::~nmea_reciever() {
      for (size_t n = 0; n < threadlist.size(); n++)
            threadlist[n].join();

}