#include <csm_terminal.hpp>

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <logger.hpp>

namespace csm_cmd
{

  Terminal* Terminal::active_instance_ = nullptr;

  namespace
  {

  std::string homeDirectory()
  {
    const char* home = std::getenv("HOME");
  #ifdef _WIN32
    if (home == nullptr) home = std::getenv("USERPROFILE");
  #endif
    return (home != nullptr) ? home : ".";
  }

  }  // namespace

  Terminal& Terminal::instance()
  {
    static Terminal terminal;
    return terminal;
  }

  Terminal::Terminal(std::chrono::milliseconds) : timeout_(kDefaultTimeout)
  {
    history_path_ = homeDirectory() + "/.csm_cmd_history";
    log_path_ = homeDirectory() + "/.csm_cmd.log";

    Logger::instance().configure(log_path_);
    Logger::instance().info("Terminal initialized");

    repl_.install_window_change_handler();
    repl_.set_max_history_size(static_cast<int>(kMaxHistoryLines));
    repl_.set_completion_count_cutoff(32);
    repl_.set_word_break_characters(" \t");

    // Enable bracketed paste for better paste handling
    repl_.enable_bracketed_paste();

    setupPrompt();
    registerBuiltins();
    setupCompletion();
    setupHighlighter();
    loadHistory();

    installSignalHandler(this);
  }

  Terminal::~Terminal()
  {
    flushHistoryCache();
    saveHistory();

    if (active_instance_ == this)
    {
      active_instance_ = nullptr;
    }

    Logger::instance().info("Terminal shutting down");
  }

  void Terminal::installSignalHandler(Terminal* self)
  {
    active_instance_ = self;
    std::signal(SIGINT, &Terminal::signalHandler);
  }

  void Terminal::signalHandler(int signum)
  {
    (void)signum;
    if (active_instance_ != nullptr)
    {
      Logger::instance().warn("Received SIGINT, shutting down gracefully");
      active_instance_->stop();
    }
  }

  std::string Terminal::escapeOutput(const std::string& text)
  {
    std::string escaped;
    escaped.reserve(text.size());

    for (unsigned char c : text)
    {
      if (c == 0x1b)
      {
        escaped += "\\x1b";
      }
      else if (c == '\r')
      {
        escaped += "\\r";
      }
      else if (c < 0x20 && c != '\n' && c != '\t')
      {
        std::ostringstream oss;
        oss << "\\x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(c);
        escaped += oss.str();
      }
      else
      {
        escaped.push_back(static_cast<char>(c));
      }
    }

    return escaped;
  }

  void Terminal::registerCommand(const std::string& name,
                                  CommandRegistry::CommandHandler handler,
                                  const std::string& description)
  {
    registry_.registerCommand(name, std::move(handler), description);
    Logger::instance().info("Registered command: " + name);
  }

  void Terminal::registerAlias(const std::string& alias, const std::string& target)
  {
    registry_.registerAlias(alias, target);
    Logger::instance().info("Registered alias: " + alias + " -> " + target);
  }

  void Terminal::setupPrompt()
  {
    repl_.set_prompt(kPrompt);
  }

  void Terminal::setupCompletion()
  {
    repl_.set_completion_callback(
      [this](const std::string& input, int& context_len)
        -> replxx::Replxx::completions_t
    {
      context_len = static_cast<int>(input.size());
      replxx::Replxx::completions_t completions;

      for (const auto& name : registry_.getCompletions(input))
      {
        completions.emplace_back(name);
      }

      return completions;
    });
  }

  void Terminal::setupHighlighter()
  {
    repl_.set_highlighter_callback(
      [this](const std::string& input, replxx::Replxx::colors_t& colors)
    {
      if (input.empty())
      {
        return;
      }

      std::size_t first_space = input.find(' ');
      if (first_space == std::string::npos)
      {
        first_space = input.size();
      }

      const std::string command = input.substr(0, first_space);

      const replxx::Replxx::Color color = registry_.hasCommand(command)
        ? replxx::Replxx::Color::CYAN
        : replxx::Replxx::Color::RED;

      for (std::size_t i = 0; i < first_space && i < colors.size(); ++i)
      {
        colors[i] = color;
      }

      if (first_space < input.size())
      {
        for (std::size_t i = first_space + 1; i < colors.size(); ++i)
        {
          colors[i] = replxx::Replxx::Color::WHITE;
        }
      }
    });
  }

  std::string Terminal::colorize(const std::string& text, const char* color)
  {
    return std::string(color) + text + kColorReset;
  }

  std::string Terminal::formatHelpLine(const std::string& name,
                                        const std::string& description)
  {
    constexpr int kColumnWidth = 20;
    std::string padded_name = name;

    if (padded_name.size() < static_cast<size_t>(kColumnWidth))
    {
      padded_name.append(kColumnWidth - padded_name.size(), ' ');
    }
    else
    {
      padded_name.resize(kColumnWidth - 3);
      padded_name += "...";
    }

    return colorize(padded_name, kColorBrightCyan) + " " + description;
  }

  void Terminal::registerBuiltins()
  {
    // Core builtins - always available
    registerCommand("help",
      [this](const std::vector<std::string>&) -> int
    {
      std::cout << kColorBold << kColorBrightYellow << "\n"
                << "╔══════════════════════════════════════════════════════════╗\n"
                << "║                    AVAILABLE COMMANDS                    ║\n"
                << "╚══════════════════════════════════════════════════════════╝\n"
                << kColorReset << "\n";

      const auto& names = registry_.getCommandNames();
      for (const auto& name : names)
      {
        std::cout << "  " << formatHelpLine(name, registry_.getDescription(name))
                  << "\n";
      }

      std::cout << "\n" << kColorDim << "  Tip: Use TAB for autocompletion\n"
                << "  Type 'quit' or press Ctrl+C to exit"
                << kColorReset << "\n\n";
      return 0;
    }, "List all available commands");

    registerCommand("clear",
      [](const std::vector<std::string>&) -> int
    {
      std::cout << "\x1b[2J\x1b[H";
      return 0;
    }, "Clear the terminal screen");

    registerCommand("quit",
      [this](const std::vector<std::string>&) -> int
    {
      std::cout << kColorBrightGreen << "Goodbye!" << kColorReset << "\n";
      stop();
      return 0;
    }, "Exit the terminal");

    registerCommand("exit",
      [this](const std::vector<std::string>&) -> int
    {
      std::cout << kColorBrightGreen << "Goodbye!" << kColorReset << "\n";
      stop();
      return 0;
    }, "Exit the terminal");

    registerCommand("history",
      [this](const std::vector<std::string>& args) -> int
    {
      bool show_all = false;
      int limit = 20;

      for (const auto& arg : args)
      {
        if (arg == "--all" || arg == "-a")
        {
          show_all = true;
        }
      }

      std::cout << kColorBold << kColorBrightYellow
                << "\n╔══════════════════════════════════════════════════════════╗\n"
                  << "║                    COMMAND HISTORY                       ║\n"
                  << "╚══════════════════════════════════════════════════════════╝\n"
                << kColorReset << "\n";

      if (history_cache_.empty())
      {
        std::cout << colorize("  History is empty", kColorDim) << "\n";
      }
      else
      {
        size_t start_index = show_all ? 0 :
          std::max(0, static_cast<int>(history_cache_.size()) - limit);

        size_t total_commands = total_history_count_
          - history_cache_.size() + start_index + 1;

        for (size_t i = start_index; i < history_cache_.size(); ++i)
        {
          std::cout << "  "
                    << colorize(std::to_string(total_commands + i - start_index),
                                kColorBrightMagenta)
                    << "  "
                    << colorize(history_cache_[i], kColorBrightCyan)
                    << "\n";
        }

        std::cout << kColorDim << "\n  Total: " << total_history_count_
                  << " | Showing " << (history_cache_.size() - start_index)
                  << " of " << history_cache_.size() << kColorReset << "\n";
      }

      std::cout << "\n";
      return 0;
    }, "Show command history [--all]");

    registerCommand("echo",
      [](const std::vector<std::string>& args) -> int
    {
      for (size_t i = 0; i < args.size(); ++i)
      {
        std::cout << args[i] << (i + 1 < args.size() ? " " : "");
      }
      std::cout << "\n";
      return 0;
    }, "Print arguments back to the terminal");

    registerCommand("version",
      [](const std::vector<std::string>&) -> int
    {
      std::cout << kColorBrightCyan << "csm_cmd "
                << kColorBrightYellow << "1.1.0"
                << kColorReset << " - CSM CMD Terminal Interface\n";
      return 0;
    }, "Print terminal version");

    registerCommand("time",
      [](const std::vector<std::string>&) -> int
    {
      const std::time_t now = std::time(nullptr);
      std::tm tm_buf{};
  #if defined(_WIN32)
      localtime_s(&tm_buf, &now);
  #else
      localtime_r(&now, &tm_buf);
  #endif
      std::cout << kColorBrightGreen << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << kColorReset << "\n";
      return 0;
    }, "Print the current date and time");

    // Test commands (only available in debug builds)
  #ifdef CSM_CMD_BUILD_TESTS
    registerCommand("test_echo",
      [](const std::vector<std::string>& args) -> int
    {
      std::cout << "Test echo received " << args.size() << " args:\n";
      for (size_t i = 0; i < args.size(); ++i)
      {
        std::cout << "  [" << i << "] '" << args[i] << "'\n";
      }
      return 0;
    }, "Test command for debugging");

    registerCommand("test_error",
      [](const std::vector<std::string>&) -> int
    {
      throw std::runtime_error("This is a test error");
    }, "Test command that throws an error");

    registerCommand("test_timeout",
      [](const std::vector<std::string>&) -> int
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      return 0;
    }, "Test command that simulates timeout");
  #endif
  }

  void Terminal::flushHistoryCache()
  {
    if (pending_flush_.empty()) { return; }

    std::ofstream history_file(history_path_, std::ios::app);
    if (!history_file.is_open())
    {
      Logger::instance().warn("Failed to open history file for flushing");
      return;
    }

    for (const auto& cmd : pending_flush_) { history_file << cmd << "\n"; }

    history_file.close();

    const size_t count = pending_flush_.size();
    pending_flush_.clear();

    Logger::instance().debug("Flushed " + std::to_string(count) + " commands to history file");
  }

  void Terminal::loadHistory()
  {
    repl_.history_load(history_path_);

    std::ifstream history_file(history_path_);
    if (history_file.is_open())
    {
      std::string line;
      std::deque<std::string> temp_cache;

      while (std::getline(history_file, line))
      {
        if (!line.empty())
        {
          temp_cache.push_back(line);
          ++total_history_count_;
        }
      }
      history_file.close();

      while (temp_cache.size() > kMaxCachedHistory) { temp_cache.pop_front(); }

      history_cache_ = std::move(temp_cache);
    }

    Logger::instance().info("Loaded " + std::to_string(history_cache_.size()) + " history entries");
  }

  void Terminal::saveHistory()
  {
    flushHistoryCache();
    repl_.history_save(history_path_);
    Logger::instance().info("Saved history");
  }

  void Terminal::addHistoryEntry(const std::string& line)
  {
    repl_.history_add(line);

    history_cache_.push_back(line);
    ++total_history_count_;

    while (history_cache_.size() > kMaxCachedHistory)
    {
      pending_flush_.push_back(history_cache_.front());
      history_cache_.pop_front();

      if (pending_flush_.size() >= kHistoryFlushThreshold) { flushHistoryCache(); }
    }
  }

  int Terminal::executeWithTimeout(const std::string& name, const std::vector<std::string>& args) const
  {
    auto future = std::async(std::launch::async, [this, name, args]() -> int { return registry_.execute(name, args); });

    const auto status = future.wait_for(timeout_);

    if (status != std::future_status::ready)
    {
      Logger::instance().warn("Command timed out: " + name + " (" + std::to_string(timeout_.count()) + "ms)");
      std::cerr << colorize("✗ Command timed out: ", kColorBrightRed) << colorize(name, kColorBrightYellow) << "\n";
      return -2;
    }

    return future.get();
  }

  int Terminal::dispatch(const std::vector<std::string>& tokens) const
  {
    if (tokens.empty()) { return 0; }

    const std::string& name = tokens.front();
    const std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    if (!registry_.hasCommand(name))
    {
      Logger::instance().warn("Unknown command: " + name);
      std::cerr << colorize("✗ Unknown command: ", kColorBrightRed) << colorize(name, kColorBrightYellow) << colorize(" (not registered)", kColorDim) << "\n";
      return -1;
    }

    Logger::instance().info("Executing: " + name + " (" + std::to_string(args.size()) + " args)");

    try { return executeWithTimeout(name, args); }
    catch (const std::exception& ex)
    {
      Logger::instance().error("Command '" + name + "' failed: " + ex.what());
      std::cerr << colorize("✗ Command failed: ", kColorBrightRed) << colorize(ex.what(), kColorBrightYellow) << "\n";
      return -3;
    }
  }

  void Terminal::run()
  {
    running_ = true;

    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n"
                << "║           CSM_CMD TERMINAL v1.1.0                        ║\n"
                << "║           Type 'help' to get started                     ║\n"
                << "╚══════════════════════════════════════════════════════════╝\n";

    while (running_)
    {
      const char* raw_line = repl_.input(kPrompt);

      if (raw_line == nullptr) { break; }
      const std::string line(raw_line);

      if (line.empty()) { continue; }
      std::vector<std::string> tokens;

      try { tokens = parser_.parse(line); }
      catch (const ParseError& ex)
      {
        Logger::instance().warn("Parse error: " + std::string(ex.what()));
        std::cerr << colorize("✗ Parse error: ", kColorBrightRed) << ex.what() << "\n";
        continue;
      }

      if (tokens.empty()) { continue; }

      addHistoryEntry(line);
      dispatch(tokens);
    }

    saveHistory();
  }

  void Terminal::stop() { running_ = false; }

}  // namespace csm_cmd