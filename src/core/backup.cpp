#include "core/backup.h"

#include "core/metrics_json.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

bool Backup::saveBackup(const std::deque<Metrics>& metrics) {
  if (metrics.empty()) {
    return true;
  }

  std::ofstream file(path_);
  if (!file) {
    return false;
  }

  const nlohmann::json arr = metrics;
  file << arr.dump(2);

  return file.good();
}

std::optional<std::deque<Metrics>> Backup::loadBackup() {
  std::ifstream file(path_);
  if (!file) {
    return std::nullopt;
  }

  const nlohmann::json arr = nlohmann::json::parse(file, nullptr, false);
  if (arr.is_discarded()) {
    return std::nullopt;
  }
  if (!arr.is_array()) {
    return std::nullopt;
  }

  std::size_t skippedMetric{0};
  std::deque<Metrics> result{};
  for (const auto& item : arr) {
    if (!item.is_object()) {
      ++skippedMetric;
      continue;
    }
    try {
      result.push_back(item.get<Metrics>());
    } catch (const nlohmann::json::exception&) {
      ++skippedMetric;
    }
  }
  if (skippedMetric > 0) {
    std::cerr << "[backup] loaded: " << result.size()
              << ", skipped: " << skippedMetric << '\n';
  }

  return result;
}

bool Backup::removeBackup() {
  std::error_code ec{};
  std::filesystem::remove(path_, ec);

  return !ec;
}