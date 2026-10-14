#ifndef PLATFORM_H
#define PLATFORM_H

#include "core/interface/metrics_collector.h"
#include "core/interface/system_info.h"

#if defined(_WIN32)
#include "platform/windows/windows_metrics_collector.h"
#include "platform/windows/windows_system_info.h"
namespace platform {
using MetricsCollector = WindowsMetricsCollector;
using SystemInfo = WindowsSystemInfo;
}  // namespace platform

#elif defined(__linux__)
#include "platform/linux/linux_metrics_collector.h"
#include "platform/linux/linux_system_info.h"
namespace platform {
using MetricsCollector = LinuxMetricsCollector;
using SystemInfo = LinuxSystemInfo;
}  // namespace platform

#endif

#endif
