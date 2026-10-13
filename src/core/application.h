#ifndef APPLICATION_H
#define APPLICATION_H

#include "core/agent.h"
#include "core/backup.h"
#include "core/interface/transport.h"
#include "core/metrics.h"
#include "core/packet_builder.h"
#include "core/thread_safe_queue.h"
#include "transport/http/http_transport.h"
#include "platform/platform.h"

class Application {
 public:
  Application();

  void run();

 private:
  platform::SystemInfo systemInfo_;
  platform::MetricsCollector collector_;
  ThreadSafeQueue<Metrics> queue_;
  HttpTransport transport_;
  Backup backup_;
  PacketBuilder packetBuilder_;
  Agent agent_;
};

#endif