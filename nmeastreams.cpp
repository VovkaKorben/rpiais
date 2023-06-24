#include "nmeastreams.h"
#include <thread>

#define BUFFLEN 2048

void process_buffer(pchar buff, int32 len)
{
      int32 pos = 0, found;
       char  rn[3]= "\r\n";
      while (pos < len)
      {
            found = search_chararray(buff, rn, len, pos);

            if (pos == -1)
                  return;

            pos = found;

      }
}


void rcv_thread(sockets_s opt, StringArray* list) {
      memset(&opt.servaddr, 0, sizeof(sockaddr_in));
      opt.servaddr.sin_family = AF_INET;
      opt.servaddr.sin_addr.s_addr = inet_addr(opt.addr);
      opt.servaddr.sin_port = htons(opt.port);
      char buff[BUFFLEN], path_full[256];
      sprintf(path_full, "%s://%s:%d\0", opt.type_str, opt.addr, opt.port);
      int32 n;

      while (true)
      {
            // connect section

           // printf("Connecting to socket %s://%s:%d...\n", opt.type_str, opt.addr, opt.port);
            opt.sockfd = socket(AF_INET, opt.type, 0);
            if (opt.sockfd < 0)
            {
                  printf("OPEN failed\n");
                  continue;
            }

            switch (opt.type) {
                  case SOCK_DGRAM: {

                        fcntl(opt.sockfd, F_SETFL, O_NONBLOCK);
                        break;
                  }
                  case SOCK_STREAM: {

                        if (connect(opt.sockfd, (struct sockaddr*)&opt.servaddr, sizeof(opt.servaddr)) < 0)
                        {
                              printf("Connect to %s failed.\n", path_full);
                              continue;
                        }
                  
                  }


            }

            printf("Connected to %s\n", path_full);
            // read section
            try {
                  while (true)
                  {

                        switch (opt.type)
                        {
                              case SOCK_DGRAM: {
                                    socklen_t len;
                                    n = (int32)recvfrom(opt.sockfd,
                                          (char*)buff,
                                          BUFFLEN,
                                          MSG_DONTWAIT,
                                          (struct sockaddr*)&opt.servaddr,
                                          &len);

                                    break;
                              }
                              case  SOCK_STREAM: {
                                    n = (int32)read(opt.sockfd, (char*)buff, BUFFLEN);
                                    break;
                              }
                        }
                        // process recieved data
                        if (n > 0)
                              process_buffer(buff, n);


                        usleep(500000);
                  }

            }
            catch (...)
            {
                  printf("exception occured!");
            }
      }
}
/*



 char buff[BUFFLEN];
int n;
while (true)
{
      for (size_t sn = 0; sn < socklist.size(); sn++)

      {
            switch (socklist[sn].type)
            {
                  case SOCK_DGRAM: {
                        socklen_t len;
                        n = recvfrom(socklist[sn].sockfd, (char*)buff, BUFFLEN, MSG_DONTWAIT, (struct sockaddr*)&socklist[sn].servaddr, &len);

                        break;
                  }
                  case  SOCK_STREAM: {
                        n = read(socklist[sn].sockfd, (char*)buff, BUFFLEN);
                        break;
                  }
            }
            printf("N: %d\n", n);
      }
}
 */


nmea_reciever::nmea_reciever(CSimpleIniA* ini, PStringArray list) {
      sockets_s ss;
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

            ss.enabled = ini->GetBoolValue(section_name, "enabled", false);
            ss.name = ini->GetValue(section_name, "name", section_name);
            if (!ss.enabled)
            {
                  printf("Socket #%d (%s) disabled in INI. Set enabled=1 for activate\n", sn - 1, ss.name);
                  continue;
            }

            tmp = ini->GetValue(section_name, "type", "");
            int32 len = length_constchararray(tmp) + 1;
            strncpy(ss.type_str, tmp, len);
            lowercase_chararray(ss.type_str, len);
            if (strcmp(ss.type_str, "udp") == 0)
                  ss.type = SOCK_DGRAM;
            else if (strcmp(ss.type_str, "tcp") == 0)
                  ss.type = SOCK_STREAM;
            else {
                  ss.type = -1;
                  printf("Socket #%d (%s) unknown protocol: %s\n", sn - 1, ss.name, ss.type_str);
            }

            if (ss.type != -1) {
                  ss.addr = ini->GetValue(section_name, "addr", "127.0.0.1");
                  ss.port = (uint16)ini->GetLongValue(section_name, "port", 0);

                  threadlist.push_back(
                        std::thread(rcv_thread, ss, list)
                  );
            }

      } while (true);
}

nmea_reciever::~nmea_reciever() {
      for (size_t n = 0; n < threadlist.size(); n++)
            threadlist[n].join();

}   /*
StringArray nmea_reciever::get_data()
{

            char buff[BUFFLEN];
            int n;
            while (true)
            {
                  for (size_t sn = 0; sn < socklist.size(); sn++)

                  {
                        switch (socklist[sn].type)
                        {
                              case SOCK_DGRAM: {
                                    socklen_t len;
                                    n = recvfrom(socklist[sn].sockfd, (char*)buff, BUFFLEN, MSG_DONTWAIT, (struct sockaddr*)&socklist[sn].servaddr, &len);

                                    break;
                              }
                              case  SOCK_STREAM: {
                                    n = read(socklist[sn].sockfd, (char*)buff, BUFFLEN);
                                    break;
                              }
                        }
                        printf("N: %d\n", n);
                  }
            }

}
 */