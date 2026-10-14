#include "platform/windows/windows_system_info.h"

#include "platform/windows/wide_to_utf8.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <string>

std::string WindowsSystemInfo::getComputerName() const {
  // maximum computer name length + null terminator
  DWORD bufferSize{MAX_COMPUTERNAME_LENGTH + 1};

  std::wstring buffer(bufferSize, L'\0');

  if (!GetComputerNameW(buffer.data(), &bufferSize)) {
    return {};
  }

  buffer.resize(bufferSize);

  return wideToUtf8(buffer);
}