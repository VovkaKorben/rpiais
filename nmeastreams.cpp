#include "nmeastreams.h"



#define BUFFLEN 2048



nmea_reciever::nmea_reciever(CSimpleIniA *ini) {

      
      //CSimpleIniA::TNamesDepend sections;
      

      

      sockets_s ss;
      socklist.clear();
      char  section_name[256], type[256];
      const char* addr, * buff;
      int  sn = 0;
      uint16 port;


      do
      {
            sprintf(section_name, "socket%d", sn++);
            if (!ini->SectionExists(section_name))
                  break;

            buff = ini->GetValue(section_name, "type", "");
            size_t bufflen = strlen(buff) + 1;
            strncpy(type, buff, bufflen);
            lowercase_chararray(type, bufflen);
            addr = ini->GetValue(section_name, "addr", "127.0.0.1");
            port =(uint16) ini->GetLongValue(section_name, "port", 0);



            memset(&ss.servaddr, 0, sizeof(sockaddr_in));
            ss.servaddr.sin_family = AF_INET;
            ss.servaddr.sin_addr.s_addr = inet_addr(addr);
            ss.servaddr.sin_port = htons(port);
            printf("Try socket %s://%s:%d...", type, addr, port);
            if (strcmp(type, "udp") == 0) {
                  ss.type = SOCK_DGRAM;
                  ss.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                  fcntl(ss.sockfd, F_SETFL, O_NONBLOCK);
            }
            else if (strcmp(type, "tcp") == 0) {
                  ss.type = SOCK_STREAM;
                  ss.sockfd = socket(AF_INET, SOCK_STREAM, 0);
                  if (ss.sockfd < 0)
                  {
                        printf("OPEN failed\n");
                        continue;
                  }
                  if (connect(ss.sockfd, (struct sockaddr*)&ss.servaddr, sizeof(ss.servaddr)) < 0)
                  {
                        printf("CONNECT failed\n");
                        continue;
                  }
                  int err = fcntl(ss.sockfd, F_SETFL, O_NONBLOCK);
                  if (err == -1)
                  {
                        perror("fcntl");
                        //printf(
                  }
            }
            else
            {
                  printf("FAIL. Invalid connection type: %s\n", type);
                  continue;
            }
            printf("OK\n");

            //fcntl(sockfd, F_SETFL, O_NONBLOCK);
            socklist.push_back(ss);

      } while (true);
      //return 0;





}
nmea_reciever::~nmea_reciever() {
}

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
