#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include <cmd_registry.hpp>

using csm_cmd::CommandRegistry;

namespace
{

int runWithTimeout(CommandRegistry& registry, const std::string& name, std::chrono::milliseconds timeout)
{
  auto future = std::async(std::launch::async, [&registry, name]()
  {
    return registry.execute(name, {});
  });

  if (future.wait_for(timeout) != std::future_status::ready)
  {
    return -2;
  }
  return future.get();
}

}  // namespace

TEST(TimeoutTest, FastCommandCompletesWithinDefaultTimeout)
{
  CommandRegistry registry;
  registry.registerCommand("fast", [](const std::vector<std::string>&) { return 0; }, "");

  const int result = runWithTimeout(registry, "fast", std::chrono::milliseconds(100));
  EXPECT_EQ(result, 0);
}

TEST(TimeoutTest, SlowCommandExceedsTimeout)
{
  CommandRegistry registry;
  registry.registerCommand("slow", [](const std::vector<std::string>&)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return 0;
  }, "");

  const auto start = std::chrono::steady_clock::now();
  const int result = runWithTimeout(registry, "slow", std::chrono::milliseconds(50));
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_EQ(result, -2);
  EXPECT_LT(elapsed, std::chrono::milliseconds(300));
}
