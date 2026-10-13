#ifndef LINUX_METRICS_COLLECTOR_H
#define LINUX_METRICS_COLLECTOR_H

#include "core/interface/metrics_collector.h"
#include "core/metrics.h"

class LinuxMetricsCollector : public MetricsCollector {
 public:
  Metrics collect() const override { return { {},{},{}, true }; };
};

#endif
