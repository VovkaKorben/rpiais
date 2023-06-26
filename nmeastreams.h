#pragma once
#ifndef __NMEASTREAMS_H
#define __NMEASTREAMS_H
#include "mydefs.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <SimpleIni.h>
#include <thread>
#include <atomic>
#include <queue>
struct sockets_s
{
      //std::atomic_int running;
      int32 running;
      int32 enabled, sockfd, type;
      uint16 port;
      //std::string ;
      const char* addr, * name;
      char type_str[256];
      struct sockaddr_in servaddr;
};


class nmea_reciever
{
private:
      std::vector<std::thread> threadlist;
public:
      nmea_reciever(CSimpleIniA* ini, StringQueue* list);
      // void rcv_thread(sockets_s  opt);
      ~nmea_reciever();
      //StringArray get_data();
};
#endif