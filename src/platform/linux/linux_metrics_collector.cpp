#include "platform/linux/linux_metrics_collector.h"

#include "core/metrics.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>

#include <chrono>
#include <format>
#include <fstream>
#include <string>
#include <filesystem>

namespace {

unsigned long getPidProperty(Display* display, Window window) {
  const Atom pidAtom{XInternAtom(display, "_NET_WM_PID", False)};
  if (pidAtom == None) {
    return 0;
  }

  Atom actualType{None};
  int actualFormat{0};
  unsigned long nItems{0};
  unsigned long bytesAfter{0};
  unsigned char* data{nullptr};

  const int status{XGetWindowProperty(display, window, pidAtom, 0, 1, False,
                                      XA_CARDINAL, &actualType, &actualFormat,
                                      &nItems, &bytesAfter, &data)};

  if (status != Success || data == nullptr || actualFormat != 32 ||
      nItems < 1) {
    if (data != nullptr) {
      XFree(data);
    }
    return 0;
  }

  const auto pid{*reinterpret_cast<unsigned long*>(data)};
  XFree(data);
  return pid;
}

std::string getProcessName(unsigned long pid) {
  if (pid == 0) {
    return {};
  }

  const std::filesystem::path exePath{"/proc/" + std::to_string(pid) + "/exe"};

  std::error_code ec;
  const auto target{std::filesystem::read_symlink(exePath, ec)};
  if (!ec && !target.empty()) {
    return target.filename().string();
  }

  const std::string commPath{"/proc/" + std::to_string(pid) + "/comm"};
  std::ifstream file(commPath);
  if (!file) {
    return {};
  }

  std::string name;
  std::getline(file, name);
  return name;
}

std::string getWindowTitle(Display* display, Window window) {

  const Atom netWmName{XInternAtom(display, "_NET_WM_NAME", False)};
  const Atom utf8String{XInternAtom(display, "UTF8_STRING", False)};

  Atom actualType{None};
  int actualFormat{0};
  unsigned long nItems{0};
  unsigned long bytesAfter{0};
  unsigned char* data{nullptr};

  const int status{XGetWindowProperty(
      display, window, netWmName, 0, 1024, False, utf8String, &actualType,
      &actualFormat, &nItems, &bytesAfter, &data)};

  if (status == Success && data != nullptr) {
    std::string title{reinterpret_cast<char*>(data), nItems};
    XFree(data);
    return title;
  }
  if (data != nullptr) {
    XFree(data);
  }

  XTextProperty textProp{};
  if (XGetWMName(display, window, &textProp) == 0) {
    return {};
  }

  std::string title;
  if (textProp.value != nullptr && textProp.nitems > 0) {
    title.assign(reinterpret_cast<char*>(textProp.value), textProp.nitems);
  }
  if (textProp.value != nullptr) {
    XFree(textProp.value);
  }

  return title;
}

Window findTopLevelWindow(Display* display, Window window) {
  Window root{None};
  Window parent{None};
  Window* children{nullptr};
  unsigned int nChildren{0};

  while (true) {
    if (XQueryTree(display, window, &root, &parent, &children, &nChildren) ==
        0) {
      break;
    }
    if (children != nullptr) {
      XFree(children);
      children = nullptr;
    }
    if (parent == root || parent == None) {
      break;
    }
    window = parent;
  }

  return window;
}

bool isUserActive(Display* display) {
  int eventBase{0};
  int errorBase{0};

  if (XScreenSaverQueryExtension(display, &eventBase, &errorBase) == 0) {
    return false;
  }

  XScreenSaverInfo* info{XScreenSaverAllocInfo()};
  if (info == nullptr) {
    return false;
  }

  constexpr unsigned long idleThresholdMs{5000};

  bool active{false};
  if (XScreenSaverQueryInfo(display, DefaultRootWindow(display), info) != 0) {
    active = info->idle <= idleThresholdMs;
  }

  XFree(info);
  return active;
}

std::string getLocalTimeString() {
  const auto now{std::chrono::system_clock::now()};
  const std::time_t time{std::chrono::system_clock::to_time_t(now)};

  std::tm local{};
  localtime_r(&time, &local);

  return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
                     local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                     local.tm_hour, local.tm_min, local.tm_sec);
}

}

Metrics LinuxMetricsCollector::collect() const {
  Metrics metrics;
  metrics.time = getLocalTimeString();

  Display* display{XOpenDisplay(nullptr)};
  if (display == nullptr) {
    return metrics;
  }

  Window focus{None};
  int revert{0};
  XGetInputFocus(display, &focus, &revert);

  if (focus != None && focus != PointerRoot) {
    const Window topLevel{findTopLevelWindow(display, focus)};

    const unsigned long pid{getPidProperty(display, topLevel)};
    metrics.processName = getProcessName(pid);
    metrics.windowTitle = getWindowTitle(display, topLevel);
  }

  metrics.userActive = isUserActive(display);

  XCloseDisplay(display);
  return metrics;
}