#include <gtest/gtest.h>

#include <cmd_parser.hpp>

using csm_cmd::CommandParser;
using csm_cmd::ParseError;

TEST(CommandParserTest, SplitsSimpleWhitespace)
{
  CommandParser parser;
  const auto tokens = parser.parse("echo hello world");
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], "echo");
  EXPECT_EQ(tokens[1], "hello");
  EXPECT_EQ(tokens[2], "world");
}

TEST(CommandParserTest, HandlesDoubleQuotedSpaces)
{
  CommandParser parser;
  const auto tokens = parser.parse("echo \"hello world\"");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello world");
}

TEST(CommandParserTest, HandlesSingleQuotedSpaces)
{
  CommandParser parser;
  const auto tokens = parser.parse("echo 'hello world'");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello world");
}

TEST(CommandParserTest, HandlesEscapedCharacters)
{
  CommandParser parser;
  const auto tokens = parser.parse("echo hello\\ world");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello world");
}

TEST(CommandParserTest, HandlesEscapedQuoteInsideDoubleQuotes)
{
  CommandParser parser;
  const auto tokens = parser.parse(R"(echo "say \"hi\"")");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "say \"hi\"");
}

TEST(CommandParserTest, ThrowsOnUnterminatedQuote)
{
  CommandParser parser;
  EXPECT_THROW(parser.parse("echo \"unterminated"), ParseError);
}

TEST(CommandParserTest, ThrowsOnOversizedInput)
{
  CommandParser parser;
  const std::string huge(CommandParser::kMaxInputLength + 1, 'a');
  EXPECT_THROW(parser.parse(huge), ParseError);
}

TEST(CommandParserTest, EmptyLineReturnsNoTokens)
{
  CommandParser parser;
  EXPECT_TRUE(parser.parse("").empty());
  EXPECT_TRUE(parser.parse("   ").empty());
}
