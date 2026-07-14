#ifndef CSM_CMD_COMMAND_PARSER_HPP
#define CSM_CMD_COMMAND_PARSER_HPP

#include <stdexcept>
#include <string>
#include <vector>

namespace csm_cmd
{

/**
 * @brief Thrown by CommandParser when the input line is malformed.
 */
class ParseError : public std::runtime_error
{
public:
  explicit ParseError(const std::string& message)
    : std::runtime_error(message)
  {
  }
};

/**
 * @brief Splits a raw input line into shell-like tokens.
 *
 * Supports single and double quoting and backslash escaping inside double
 * quotes. Enforces a maximum input length to avoid pathological inputs.
 */
class CommandParser
{
public:
  static constexpr std::size_t kMaxInputLength = 4096;

  /**
   * @brief Parse a line into whitespace separated tokens honoring quotes.
   * @throws ParseError if the line exceeds kMaxInputLength or contains an
   *         unterminated quote.
   */
  std::vector<std::string> parse(const std::string& line) const;
};

}  // namespace csm_cmd

#endif  // CSM_CMD_COMMAND_PARSER_HPP
