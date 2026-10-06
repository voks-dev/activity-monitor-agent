#include "core/packet_builder.h"

#include "core/metrics.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <deque>
#include <string>

class PacketBuilderTest : public ::testing::Test {
 protected:
  const std::string agentId_{"DESKTOP-XXXXXXX"};
  const Metrics metric_{"2026-09-20 21:31:14", "chrome.exe", "GitHub", true};

  const std::deque<Metrics> metrics_{metric_};

  PacketBuilder builder_{agentId_};
};

TEST_F(PacketBuilderTest, BuildReturnsJSONString) {

  const auto j =
      nlohmann::json::parse(builder_.build(metrics_), nullptr, false);

  EXPECT_FALSE(j.is_discarded());
}

TEST_F(PacketBuilderTest, BuildContainsSpecificFields) {

  const auto j =
      nlohmann::json::parse(builder_.build(metrics_), nullptr, false);

  ASSERT_FALSE(j.is_discarded());
  EXPECT_TRUE(j.contains("agent_id"));
  EXPECT_TRUE(j.contains("timestamp"));
  EXPECT_TRUE(j.contains("payload"));
}

TEST_F(PacketBuilderTest, BuildReflectsInputValues) {

  const auto j =
      nlohmann::json::parse(builder_.build(metrics_), nullptr, false);
  ASSERT_FALSE(j.is_discarded());

  ASSERT_TRUE(j["agent_id"].is_string());
  EXPECT_EQ(j["agent_id"].get<std::string>(), agentId_);

  ASSERT_TRUE(j["payload"].is_array());
  EXPECT_EQ(j["payload"].size(), metrics_.size());
}