#include "core/agent.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <mutex>
#include <utility>

void Agent::collectorLoop() {
  try {
    while (!stopRequested_) {
      queue_.push(collector_.collect());

      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_for(lock, std::chrono::seconds(5),
                   [this] { return stopRequested_.load(); });
    }
  } catch (const std::exception& ex) {
    std::cerr << "[collector] exception: " << ex.what() << "- signaling stop\n";
    stopRequested_ = true;
    cv_.notify_all();
    return;

  } catch (...) {
    std::cerr << "[collector] unknown exception - signaling stop\n";
    stopRequested_ = true;
    cv_.notify_all();
    return;
  }
}

void Agent::transportLoop() {
  try {
    auto lastFlush{std::chrono::steady_clock::now()};

    while (!stopRequested_) {
      const auto batch{queue_.peekAll()};
      const auto now{std::chrono::steady_clock::now()};
      const auto elapsed{
          std::chrono::duration_cast<std::chrono::seconds>(now - lastFlush)
              .count()};

      const bool timeToFlush{elapsed >= 30};
      const bool enoughToFlush{batch.size() >= 10};

      if (batch.empty() || (!timeToFlush && !enoughToFlush)) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(100),
                     [this] { return stopRequested_.load(); });
        continue;
      }

      const std::string packet{packetBuilder_.build(batch)};
      if (transport_.send(packet)) {
        queue_.removeFront(batch.size());
        lastFlush = now;
        continue;
      }

      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_for(lock, std::chrono::seconds(10),
                   [this] { return stopRequested_.load(); });
    }
  } catch (const std::exception& ex) {
    std::cerr << "[transport] exception: " << ex.what() << "- signaling stop\n";
    stopRequested_ = true;
    cv_.notify_all();
    return;
  } catch (...) {
    std::cerr << "[transport] unknown exception - signaling stop\n";
    stopRequested_ = true;
    cv_.notify_all();
    return;
  }
}

void Agent::start() {
  collectorThread_ = std::thread([this] { collectorLoop(); });
  transportThread_ = std::thread([this] { transportLoop(); });
}

void Agent::stop() noexcept {
  stopRequested_ = true;
  cv_.notify_all();
  queue_.stop();

  try {
    if (collectorThread_.joinable()) {
      collectorThread_.join();
    }
  } catch (const std::exception& ex) {
    std::cerr << "[agent] join(collector) failed: " << ex.what()
              << " - continuing\n";
  } catch (...) {
    std::cerr << "[agent] join(collector) failed: unknown exception"
              << " - continuing\n";
  }

  try {
    if (transportThread_.joinable()) {
      transportThread_.join();
    }
  } catch (const std::exception& ex) {
    std::cerr << "[agent] join(transport) failed: " << ex.what()
              << " - continuing\n";
  } catch (...) {
    std::cerr << "[agent] join(transport) failed: unknown exception"
              << " - continuing\n";
  }
}