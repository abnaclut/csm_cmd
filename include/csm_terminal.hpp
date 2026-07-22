#ifndef CSM_CMD_CSM_TERMINAL_HPP
#define CSM_CMD_CSM_TERMINAL_HPP

#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <replxx.hxx>

#include <cmd_parser.hpp>
#include <cmd_registry.hpp>

namespace csm_cmd
{

/**
 * @brief Terminal with color support and command management
 * @note only one instance allowed
 */
class Terminal
{
public:
  static constexpr std::chrono::milliseconds kDefaultTimeout{100};
  static constexpr std::size_t kMaxHistoryLines = 1000;
  static constexpr std::size_t kMaxCachedHistory = 128;
  static constexpr std::size_t kHistoryFlushThreshold = 64;
  static constexpr auto kPrompt = "❯ ";

  /**
   * @brief Get singleton instance
   */
  static Terminal& instance();

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

  /**
   * @brief Get registry (for testing)
   */
  CommandRegistry& getRegistry() { return registry_; }
  const CommandRegistry& getRegistry() const { return registry_; }

private:
  Terminal(std::chrono::milliseconds timeout = kDefaultTimeout);

  // Color constants
  static constexpr const char* kColorReset   = "\033[0m";
  static constexpr const char* kColorBold    = "\033[1m";
  static constexpr const char* kColorDim     = "\033[2m";
  static constexpr const char* kColorCyan    = "\033[36m";
  static constexpr const char* kColorGreen   = "\033[32m";
  static constexpr const char* kColorYellow  = "\033[33m";
  static constexpr const char* kColorRed     = "\033[31m";
  static constexpr const char* kColorBlue    = "\033[34m";
  static constexpr const char* kColorMagenta = "\033[35m";

  static constexpr const char* kColorBrightCyan    = "\033[96m";
  static constexpr const char* kColorBrightGreen   = "\033[92m";
  static constexpr const char* kColorBrightYellow  = "\033[93m";
  static constexpr const char* kColorBrightRed     = "\033[91m";
  static constexpr const char* kColorBrightBlue    = "\033[94m";
  static constexpr const char* kColorBrightMagenta = "\033[95m";

  static constexpr const char* kBgDark = "\033[48;5;236m";
  static constexpr const char* kBgBlue = "\033[48;5;24m";

  void registerBuiltins();
  void setupCompletion();
  void setupHighlighter();
  void setupPrompt();
  void loadHistory();
  void saveHistory();
  void flushHistoryCache();
  void addHistoryEntry(const std::string& line);
  int dispatch(const std::vector<std::string>& tokens) const;
  int executeWithTimeout(const std::string& name,
                         const std::vector<std::string>& args) const;

  static std::string escapeOutput(const std::string& text);
  static void installSignalHandler(Terminal* self);
  static void signalHandler(int signum);

  static std::string formatHelpLine(const std::string& name, const std::string& description);
  static std::string colorize(const std::string& text, const char* color);

  replxx::Replxx            repl_;
  CommandRegistry           registry_;
  CommandParser             parser_;
  std::chrono::milliseconds timeout_;
  std::string               history_path_;
  std::string               log_path_;

  // History management
  std::deque<std::string>  history_cache_;
  std::vector<std::string> pending_flush_;
  size_t                   total_history_count_ = 0;

  bool                     running_ = false;

  static Terminal*         active_instance_;
};

}  // namespace csm_cmd

#endif  // CSM_CMD_CSM_TERMINAL_HPP