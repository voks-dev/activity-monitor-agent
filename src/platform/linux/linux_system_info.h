#ifndef LINUX_SYSTEM_INFO_H
#define LINUX_SYSTEM_INFO_H

#include "core/interface/system_info.h"

#include <string>

class LinuxSystemInfo : public SystemInfo {
 public:
  std::string getComputerName() const override { return {}; };
};

#endif