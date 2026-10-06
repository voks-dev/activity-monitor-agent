#include "core/metrics.h"
#include "core/metrics_json.h"

#include <gtest/gtest.h>

TEST(MetricsJson, ToJSONThenFromJSON) {
  const Metrics original{"2026-09-20 21:31:14", "chrome.exe", "GitHub", true};

  nlohmann::json j = original;
  const Metrics restored{j.get<Metrics>()};

  EXPECT_EQ(restored.time, original.time);
  EXPECT_EQ(restored.processName, original.processName);
  EXPECT_EQ(restored.windowTitle, original.windowTitle);
  EXPECT_EQ(restored.userActive, original.userActive);
}

