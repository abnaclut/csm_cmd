#include "../../include/command_parser.hpp"

#include <cctype>

namespace csm_cmd
{

std::vector<std::string> CommandParser::parse(const std::string& line) const
{
  if (line.size() > kMaxInputLength)
  {
    throw ParseError("input exceeds maximum allowed length of " + std::to_string(kMaxInputLength));
  }

  std::vector<std::string> tokens;
  std::string current;
  bool in_token = false;
  char quote_char = '\0';
  bool in_quotes = false;

  for (std::size_t i = 0; i < line.size(); ++i)
  {
    const char c = line[i];

    if (in_quotes)
    {
      if (c == '\\' && quote_char == '"' && i + 1 < line.size())
      {
        const char next = line[i + 1];
        if (next == '"' || next == '\\' || next == '$' || next == '`')
        {
          current.push_back(next);
          ++i;
          continue;
        }
      }

      if (c == quote_char)
      {
        in_quotes = false;
        quote_char = '\0';
        continue;
      }

      current.push_back(c);
      continue;
    }

    if (c == '"' || c == '\'')
    {
      in_quotes = true;
      quote_char = c;
      in_token = true;
      continue;
    }

    if (std::isspace(static_cast<unsigned char>(c)) != 0)
    {
      if (in_token)
      {
        tokens.push_back(current);
        current.clear();
        in_token = false;
      }
      continue;
    }

    if (c == '\\' && i + 1 < line.size())
    {
      current.push_back(line[i + 1]);
      ++i;
      in_token = true;
      continue;
    }

    current.push_back(c);
    in_token = true;
  }

  if (in_quotes)
  {
    throw ParseError("unterminated quote in input");
  }

  if (in_token)
  {
    tokens.push_back(current);
  }

  return tokens;
}

}  // namespace csm_cmd
