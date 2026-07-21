#include <gtest/gtest.h>

#include <cmd_registry.hpp>

using csm_cmd::CommandRegistry;

TEST(CompletionTest, EmptyPrefixReturnsAllNames)
{
  CommandRegistry registry;
  registry.registerCommand("echo", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("exit", [](const std::vector<std::string>&) { return 0; }, "");

  const auto completions = registry.getCompletions("");
  EXPECT_EQ(completions.size(), 2u);
}

TEST(CompletionTest, PrefixNarrowsCandidates)
{
  CommandRegistry registry;
  registry.registerCommand("echo", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("exit", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("version", [](const std::vector<std::string>&) { return 0; }, "");

  const auto completions = registry.getCompletions("ex");
  ASSERT_EQ(completions.size(), 1u);
  EXPECT_EQ(completions[0], "exit");
}

TEST(CompletionTest, AliasesAppearInCompletions)
{
  CommandRegistry registry;
  registry.registerCommand("list", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerAlias("ls", "list");

  const auto completions = registry.getCompletions("ls");
  ASSERT_EQ(completions.size(), 1u);
  EXPECT_EQ(completions[0], "ls");
}

TEST(CompletionTest, NoMatchReturnsEmpty)
{
  CommandRegistry registry;
  registry.registerCommand("help", [](const std::vector<std::string>&) { return 0; }, "");

  EXPECT_TRUE(registry.getCompletions("zzz").empty());
}
