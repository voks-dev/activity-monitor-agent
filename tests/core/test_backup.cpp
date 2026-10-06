#include "core/backup.h"

#include "core/metrics.h"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <optional>

class BackupTest : public ::testing::Test {
 protected:
  std::filesystem::path backupPath_{};
  static std::atomic<int> counter_;
  const Metrics metric_{"2026-09-20 21:31:14", "chrome.exe", "GitHub", true};
  std::deque<Metrics> metrics_{};

  void SetUp() override {
    backupPath_ = std::filesystem::temp_directory_path() /
                  ("backup_test_" + std::to_string(++counter_) + ".json");
  }
  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(backupPath_, ec);
  }
};


std::atomic<int> BackupTest::counter_{0};


TEST_F(BackupTest, SaveBackupReturnsSuccessWhenSaveFile) {

  metrics_.push_back(metric_);

  Backup backup{backupPath_.string()};

  EXPECT_TRUE(backup.saveBackup(metrics_));
}
TEST_F(BackupTest, SaveBackupReturnsTrueOnEmptyInput) {

  Backup backup{backupPath_.string()};

  ASSERT_TRUE(metrics_.empty());
  EXPECT_TRUE(backup.saveBackup(metrics_));
}
TEST_F(BackupTest, SaveBackupReturnsFailureWhenFileNotCreate) {

  const auto invalidPath = std::filesystem::temp_directory_path() /
                           "nonexistent_dir_xyz_12345" / "backup.json";

  Backup backup{invalidPath.string()};
  metrics_.push_back(metric_);

  EXPECT_FALSE(backup.saveBackup(metrics_));
}

TEST_F(BackupTest, LoadBackupReturnsEmptyDequeWhenAllRecordsMalformed) {

  std::ofstream file(backupPath_);

  ASSERT_TRUE(file.is_open());
  file
      << R"([{"process_name": " chrome.exe ","window_title": " GitHub ","user_active": true}, 
              {"window_title": " GitHub ","user_active": true}])";
  file.close();

  Backup backup{backupPath_.string()};
  const auto result{backup.loadBackup()};

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->empty());
}
TEST_F(BackupTest, LoadBackupReturnsNulloptWhenNotFile) {

  Backup backup{backupPath_.string()};
  EXPECT_FALSE(backup.loadBackup().has_value());
}
TEST_F(BackupTest, LoadBackupReturnsNulloptWhenMalformedJson) {

  std::ofstream file(backupPath_);

  ASSERT_TRUE(file.is_open());
  file << "process_name )";
  file.close();

  Backup backup{backupPath_.string()};

  EXPECT_FALSE(backup.loadBackup().has_value());
}
TEST_F(BackupTest, LoadBackupReturnsNulloptWhenUnexpectedStructure) {

  std::ofstream file(backupPath_);

  ASSERT_TRUE(file.is_open());
  file << R"({"key": "value"})";
  file.close();

  Backup backup{backupPath_.string()};

  EXPECT_FALSE(backup.loadBackup().has_value());
}
TEST_F(BackupTest, LoadBackupSkipsMalformedRecordsButReturnsValidOnes) {

  std::ofstream file(backupPath_);

  ASSERT_TRUE(file.is_open());
  file
      << R"([{"time": "2026-09-20 21:31:14", "process_name": "chrome.exe","window_title": "GitHub","user_active": true}, 
              {"window_title": " GitHub ","user_active": true}])";
  file.close();

  Backup backup{backupPath_.string()};

  const auto result{backup.loadBackup()};

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ(result->front().processName, "chrome.exe");
}
TEST_F(BackupTest, RemoveBackupDeletesFile) {

  metrics_.push_back(metric_);

  Backup backup{backupPath_.string()};

  ASSERT_TRUE(backup.saveBackup(metrics_));
  ASSERT_TRUE(std::filesystem::exists(backupPath_));

  EXPECT_TRUE(backup.removeBackup());
  EXPECT_FALSE(std::filesystem::exists(backupPath_));
}
TEST_F(BackupTest, SaveThenLoadReturnsSameMetrics) {

  metrics_.push_back(metric_);

  Backup backup{backupPath_.string()};

  ASSERT_TRUE(backup.saveBackup(metrics_));

  const auto result{backup.loadBackup()};

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ(result->front().time, metric_.time);
  EXPECT_EQ(result->front().processName, metric_.processName);
  EXPECT_EQ(result->front().windowTitle, metric_.windowTitle);
  EXPECT_EQ(result->front().userActive, metric_.userActive);
}
