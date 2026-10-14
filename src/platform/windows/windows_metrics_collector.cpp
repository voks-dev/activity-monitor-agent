#include "platform/windows/windows_metrics_collector.h"

#include "platform/windows/wide_to_utf8.h"
#include "core/metrics.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <filesystem>
#include <format>
#include <string>
#include <string_view>

namespace {

std::string getWindowTitle(HWND hWnd) {
  const int textLength{GetWindowTextLengthW(hWnd)};
  if (textLength <= 0) {
    return {};
  }

  // textLength + null terminator
  const int bufferSize{textLength + 1};
  std::wstring buffer(bufferSize, L'\0');

  const int copied{GetWindowTextW(hWnd, buffer.data(), bufferSize)};
  if (copied <= 0) {
    return {};
  }

  buffer.resize(copied);

  return wideToUtf8(buffer);
}

std::string getProcessName(HWND hWnd) {
  DWORD pid{};
  if (GetWindowThreadProcessId(hWnd, &pid) == 0 || pid == 0) {
    return {};
  }

  // MAX_PATH * 2 buffer for long paths
  DWORD bufferSize{MAX_PATH * 2};

  std::wstring buffer(bufferSize, L'\0');

  HANDLE hProcess{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)};
  if (!hProcess) {
    return {};
  }

  // hProcess is closed before checking the result, to ensure it is released 
  // regardless of the outcome.
  const BOOL nameRetrieved{
      QueryFullProcessImageNameW(hProcess, 0, buffer.data(), &bufferSize)};

  CloseHandle(hProcess);

  if (!nameRetrieved) {
    return {};
  }

  const std::filesystem::path path{
      std::wstring_view(buffer.data(), bufferSize)};

  return wideToUtf8(path.filename().native());
}

bool isUserActive() {
  LASTINPUTINFO lii{};
  lii.cbSize = sizeof(LASTINPUTINFO);

  if (!GetLastInputInfo(&lii)) {
    return false;
  }

  const ULONGLONG idleMs{GetTickCount64() - lii.dwTime};

  constexpr ULONGLONG idleThresholdMs{5000};

  return idleMs <= idleThresholdMs;
}

std::string getLocalTimeString() {
  SYSTEMTIME systemTime{};
  GetLocalTime(&systemTime);

  return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", systemTime.wYear,
                     systemTime.wMonth, systemTime.wDay, systemTime.wHour,
                     systemTime.wMinute, systemTime.wSecond);
}

}  // namespace

Metrics WindowsMetricsCollector::collect() const {
  Metrics metrics;
  metrics.time = getLocalTimeString();

  HWND hWnd{GetForegroundWindow()};
  if (hWnd) {
    metrics.processName = getProcessName(hWnd);
    metrics.windowTitle = getWindowTitle(hWnd);
  }

  metrics.userActive = isUserActive();

  return metrics;
}
