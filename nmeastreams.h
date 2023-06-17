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

struct sockets_s
{
      int sockfd, type;
      struct sockaddr_in servaddr;
};


class nmea_reciever
{
private:
      std::vector<sockets_s> socklist;
public:
      nmea_reciever(CSimpleIniA * ini);
      ~nmea_reciever();
      StringArray get_data();
};
#endif