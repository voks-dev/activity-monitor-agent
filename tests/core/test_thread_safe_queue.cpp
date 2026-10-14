#include "core/thread_safe_queue.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <optional>
#include <thread>

class ThreadSafeQueueTest : public ::testing::Test {
 protected:
  const std::size_t maxSize{5};
  ThreadSafeQueue<int> queue_{maxSize};

  void assertInvariants() const {
    EXPECT_LE(queue_.size(), maxSize);
  }

  void TearDown() override {
    assertInvariants();
  }
};

TEST_F(ThreadSafeQueueTest, SizeReturnQueueSize) {

  queue_.push(1);
  EXPECT_EQ(queue_.size(), 1);

  queue_.push(2);
  EXPECT_EQ(queue_.size(), 2);

  queue_.pop();
  EXPECT_EQ(queue_.size(), 1);
}

TEST_F(ThreadSafeQueueTest, StopIsIdempotent) {

  queue_.push(1);
  queue_.push(2);

  queue_.stop();
  queue_.stop();

  EXPECT_EQ(queue_.pop(), 1);
  EXPECT_EQ(queue_.pop(), 2);
  EXPECT_EQ(queue_.pop(), std::nullopt);

  queue_.push(3);
  EXPECT_EQ(queue_.size(), 0);
};

TEST_F(ThreadSafeQueueTest, PushSkipWhenStoppedTrue) {

  queue_.stop();
  queue_.push(3);

  EXPECT_EQ(queue_.size(), 0);
}

TEST_F(ThreadSafeQueueTest, PushIncreasesSize) {

  queue_.push(1);
  EXPECT_EQ(queue_.size(), 1);

  queue_.push(2);
  EXPECT_EQ(queue_.size(), 2);
}

TEST_F(ThreadSafeQueueTest, PushStoresCorrectValue) {

  queue_.push(1);
  EXPECT_EQ(queue_.size(), 1);

  const auto result{queue_.peekAll()};

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 1);
}

TEST_F(ThreadSafeQueueTest, PopReturnOldestAndRemovesIt) {

  queue_.push(1);
  queue_.push(2);

  EXPECT_EQ(queue_.pop(), 1);
  EXPECT_EQ(queue_.size(), 1);

  EXPECT_EQ(queue_.pop(), 2);
  EXPECT_EQ(queue_.size(), 0);
}

TEST_F(ThreadSafeQueueTest, PopReturnNulloptWhenEmptyAndStopped) {

  queue_.stop();

  EXPECT_EQ(queue_.size(), 0);
  EXPECT_EQ(queue_.pop(), std::nullopt);
}

TEST_F(ThreadSafeQueueTest, PopReturnElementWhenNotEmptyAndStopped) {
  queue_.push(1);
  queue_.stop();

  EXPECT_EQ(queue_.pop(), 1);
}

TEST_F(ThreadSafeQueueTest, ExtractAllDeleteAndReturnAllObjectFromQueue) {

  queue_.push(1);
  queue_.push(2);
  queue_.push(3);

  const auto result{queue_.extractAll()};

  EXPECT_EQ(queue_.size(), 0);
  EXPECT_EQ(result.size(), 3);
}

TEST_F(ThreadSafeQueueTest, PeekAllReturnsAllElementsWithoutModifyingQueue) {
  queue_.push(1);
  queue_.push(2);
  queue_.push(3);

  const auto result{queue_.peekAll()};

  EXPECT_EQ(queue_.size(), 3);
  EXPECT_EQ(result.size(), 3);

  EXPECT_EQ(queue_.peekAll(), result);
}

TEST_F(ThreadSafeQueueTest, RemoveFrontDeletesNFromFront) {
  queue_.push(1);
  queue_.push(2);
  queue_.push(3);
  queue_.push(4);
  queue_.push(5);

  queue_.removeFront(3);
  EXPECT_EQ(queue_.size(), 2);
}

TEST_F(ThreadSafeQueueTest, RemoveFrontDeletesNCorrectValueFromFront) {
  queue_.push(1);
  queue_.push(2);
  queue_.push(3);
  queue_.push(4);
  queue_.push(5);

  queue_.removeFront(3);
  EXPECT_EQ(queue_.size(), 2);

  const auto result{queue_.peekAll()};

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 4);
  EXPECT_EQ(result[1], 5);
}

TEST_F(ThreadSafeQueueTest, RemoveFrontDeleteIfQueueSizeLessNValue) {
  queue_.push(1);
  queue_.push(2);
  queue_.push(3);

  queue_.removeFront(5);
  EXPECT_EQ(queue_.size(), 0);
}

TEST_F(ThreadSafeQueueTest, PushWhenFullEvictsOldest) {
  queue_.push(1);
  queue_.push(2);
  queue_.push(3);
  queue_.push(4);
  queue_.push(5);

  EXPECT_EQ(queue_.size(), 5);

  queue_.push(6);

  const auto result{queue_.peekAll()};

  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[4], 6);
}

TEST_F(ThreadSafeQueueTest, PushPopImplementFIFOSemantic) {
  queue_.push(1);
  queue_.push(2);
  queue_.push(3);

  EXPECT_EQ(queue_.pop(), 1);

  queue_.push(4);

  EXPECT_EQ(queue_.pop(), 2);
}

TEST_F(ThreadSafeQueueTest, StopUnblocksWaitingPop) {
  auto future{std::async(std::launch::async, [this] { return queue_.pop(); })};
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
            std::future_status::timeout);

  queue_.stop();

  ASSERT_EQ(future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);

  EXPECT_EQ(future.get(), std::nullopt);
}

TEST_F(ThreadSafeQueueTest, PushPopCountsMatch) {
  constexpr int count{5};
  std::atomic<int> popped{0};

  auto consumer{std::async(std::launch::async, [&] {
    while (popped < count) {
      auto value{queue_.pop()};
      if (value)
        ++popped;
      else
        break;
    }
  })};

  for (int i{0}; i < count; ++i) {
    queue_.push(i);
  }

  consumer.wait();
  EXPECT_EQ(popped.load(), count);
}