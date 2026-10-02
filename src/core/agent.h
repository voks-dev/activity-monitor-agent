#ifndef AGENT_H
#define AGENT_H

#include "core/interface/metrics_collector.h"
#include "core/interface/transport.h"
#include "core/packet_builder.h"
#include "core/thread_safe_queue.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class Agent {
 public:
  Agent(MetricsCollector& collector, ThreadSafeQueue<Metrics>& queue,
        Transport& transport, PacketBuilder& packetBuilder)
      : collector_(collector),
        queue_(queue),
        transport_(transport),
        packetBuilder_(packetBuilder) {}
  ~Agent() {
    // stop() is noexcept - no exception can escape the destructor.
    stop();
  }

  void stop() noexcept;
  void start();

 private:
  MetricsCollector& collector_;
  ThreadSafeQueue<Metrics>& queue_;
  Transport& transport_;
  PacketBuilder& packetBuilder_;
  std::thread collectorThread_{};
  std::thread transportThread_{};
  std::atomic<bool> stopRequested_{false};
  std::mutex mutex_{};
  std::condition_variable cv_{};

  void collectorLoop();
  void transportLoop();
};

#endif
