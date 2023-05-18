#pragma once
#ifndef __TOUCH_H
#define __TOUCH_H
#include "mydefs.h"

class touch_manager
{


public:
      void add_group(std::string name, int priority);
      void clear_group(std::string name);
      void add_item(std::string group, intpt intpt);


};

#endif