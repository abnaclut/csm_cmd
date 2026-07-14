#ifndef CSM_CMD_CSM_TERMINAL_HPP
#define CSM_CMD_CSM_TERMINAL_HPP

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <deque>

#include <replxx.hxx>

#include "command_parser.hpp"
#include "command_registry.hpp"

namespace csm_cmd
{

/**
 * @brief Terminal with color support and command management
 */
class Terminal
{
public:
  static constexpr std::chrono::milliseconds kDefaultTimeout{100};
  static constexpr std::size_t kMaxHistoryLines = 1000;
  static constexpr std::size_t kMaxCachedHistory = 128;    // Keep last 128 commands in memory
  static constexpr std::size_t kHistoryFlushThreshold = 64; // Flush to file when cache reaches this size
  static constexpr auto kPrompt = "❯ ";

  explicit Terminal(std::chrono::milliseconds command_timeout = kDefaultTimeout);
  ~Terminal();

  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;

  /**
   * @brief Register a command with its handler and description
   */
  void registerCommand(const std::string& name,
                       CommandRegistry::CommandHandler handler,
                       const std::string& description);

  /**
   * @brief Register an alias for an existing command
   */
  void registerAlias(const std::string& alias, const std::string& target);

  /**
   * @brief Run the interactive terminal loop
   */
  void run();

  /**
   * @brief Stop the terminal loop
   */
  void stop();

private:
  // Color constants
  static constexpr const char* kColorReset = "\033[0m";
  static constexpr const char* kColorBold = "\033[1m";
  static constexpr const char* kColorDim = "\033[2m";
  static constexpr const char* kColorCyan = "\033[36m";
  static constexpr const char* kColorGreen = "\033[32m";
  static constexpr const char* kColorYellow = "\033[33m";
  static constexpr const char* kColorRed = "\033[31m";
  static constexpr const char* kColorBlue = "\033[34m";
  static constexpr const char* kColorMagenta = "\033[35m";

  // Bright variants
  static constexpr const char* kColorBrightCyan = "\033[96m";
  static constexpr const char* kColorBrightGreen = "\033[92m";
  static constexpr const char* kColorBrightYellow = "\033[93m";
  static constexpr const char* kColorBrightRed = "\033[91m";
  static constexpr const char* kColorBrightBlue = "\033[94m";
  static constexpr const char* kColorBrightMagenta = "\033[95m";

  // Background colors
  static constexpr const char* kBgDark = "\033[48;5;236m";
  static constexpr const char* kBgBlue = "\033[48;5;24m";

  void addHistoryEntry(const std::string& line);
  void setupBuiltins();
  void setupCompletion();
  void setupHighlighter();
  void setupPrompt();
  void loadHistory();
  void saveHistory();
  void flushHistoryCache();
  int dispatch(const std::vector<std::string>& tokens) const;
  int executeWithTimeout(const std::string& name, const std::vector<std::string>& args) const;

  static std::string escapeOutput(const std::string& text);
  static void installSignalHandler(Terminal* self);
  static void signalHandler(int signum);

  std::string formatHelpLine(const std::string& name, const std::string& description) const;
  std::string colorize(const std::string& text, const char* color) const;

  replxx::Replxx repl_;
  CommandRegistry registry_;
  CommandParser parser_;
  std::chrono::milliseconds timeout_;
  std::string history_path_;

  // History management with circular buffer
  std::deque<std::string> history_cache_;  // Cache for last N commands
  std::vector<std::string> pending_flush_; // Commands waiting to be written to file
  size_t total_history_count_ = 0;         // Total commands ever executed

  bool running_ = false;

  static Terminal* active_instance_;
};

}  // namespace csm_cmd

#endif  // CSM_CMD_CSM_TERMINAL_HPP