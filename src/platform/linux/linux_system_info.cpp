#include "platform/linux/linux_system_info.h"

#include <unistd.h>

#include <array>
#include <string>

std::string LinuxSystemInfo::getComputerName() const {
  std::array<char, 256> buffer{};

  if (gethostname(buffer.data(), buffer.size()) != 0) {
    return {};
  }

  buffer.back() = '\0';

  return std::string{buffer.data()};
}