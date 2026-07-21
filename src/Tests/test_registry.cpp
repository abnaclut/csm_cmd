#include <gtest/gtest.h>

#include <cmd_registry.hpp>

using csm_cmd::CommandError;
using csm_cmd::CommandRegistry;

TEST(CommandRegistryTest, RegistersAndExecutesCommand)
{
  CommandRegistry registry;
  registry.registerCommand("greet", [](const std::vector<std::string>& args)
  {
    return args.empty() ? 1 : 0;
  }, "greet [name]");

  EXPECT_TRUE(registry.hasCommand("greet"));
  EXPECT_EQ(registry.execute("greet", {"World"}), 0);
  EXPECT_EQ(registry.execute("greet", {}), 1);
}

TEST(CommandRegistryTest, RejectsDuplicateRegistration)
{
  CommandRegistry registry;
  registry.registerCommand("greet", [](const std::vector<std::string>&) { return 0; }, "");
  EXPECT_THROW(registry.registerCommand("greet", [](const std::vector<std::string>&) { return 0; }, ""), CommandError);
}

TEST(CommandRegistryTest, RejectsEmptyName)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.registerCommand("", [](const std::vector<std::string>&) { return 0; }, ""), CommandError);
}

TEST(CommandRegistryTest, ThrowsOnUnknownCommand)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.execute("missing", {}), CommandError);
  EXPECT_FALSE(registry.hasCommand("missing"));
}

TEST(CommandRegistryTest, AliasResolvesToTarget)
{
  CommandRegistry registry;
  registry.registerCommand("list", [](const std::vector<std::string>&) { return 42; }, "list items");
  registry.registerAlias("ls", "list");

  EXPECT_TRUE(registry.hasCommand("ls"));
  EXPECT_EQ(registry.execute("ls", {}), 42);
}

TEST(CommandRegistryTest, AliasToUnknownTargetThrows)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.registerAlias("ls", "list"), CommandError);
}

TEST(CommandRegistryTest, CompletionsAreCaseSensitivePrefixMatch)
{
  CommandRegistry registry;
  registry.registerCommand("Help", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("help", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("history", [](const std::vector<std::string>&) { return 0; }, "");

  const auto lower = registry.getCompletions("he");
  ASSERT_EQ(lower.size(), 1u);
  EXPECT_EQ(lower[0], "help");

  const auto upper = registry.getCompletions("He");
  ASSERT_EQ(upper.size(), 1u);
  EXPECT_EQ(upper[0], "Help");
}
