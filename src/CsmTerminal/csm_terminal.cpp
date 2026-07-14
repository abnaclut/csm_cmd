#include "../../include/csm_terminal.hpp"

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>

#include "../Logger/logger.hpp"

namespace csm_cmd
{

  Terminal* Terminal::active_instance_ = nullptr;

  namespace
  {

  std::string homeDirectory()
  {
    const char* home = std::getenv("HOME");
    if (home != nullptr) { return home; }
    return ".";
  }

  }  // namespace

  Terminal::Terminal(const std::chrono::milliseconds command_timeout)
    : timeout_(command_timeout)
  {
    history_path_ = homeDirectory() + "/.csm_cmd_history";

    Logger::instance().configure(homeDirectory() + "/.csm_cmd.log");
    Logger::instance().info("Terminal initialized");

    repl_.install_window_change_handler();
    repl_.set_max_history_size(static_cast<int>(kMaxHistoryLines));
    repl_.set_completion_count_cutoff(32);
    repl_.set_word_break_characters(" \t");

    setupPrompt();
    setupBuiltins();
    setupCompletion();
    setupHighlighter();
    loadHistory();

    installSignalHandler(this);
  }

  Terminal::~Terminal()
  {
    // Flush any remaining history before saving
    flushHistoryCache();
    saveHistory();

    if (active_instance_ == this) { active_instance_ = nullptr; }
    Logger::instance().info("Terminal shutting down");
  }

  void Terminal::installSignalHandler(Terminal* self)
  {
    active_instance_ = self;
    std::signal(SIGINT, &Terminal::signalHandler);
  }

  void Terminal::signalHandler(const int signum)
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

    for (const unsigned char c : text)
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
        oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
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
    repl_.set_completion_callback([this](const std::string& input, int& context_len) -> replxx::Replxx::completions_t
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
    repl_.set_highlighter_callback([this](const std::string& input, replxx::Replxx::colors_t& colors)
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

      // Color the command name
      const replxx::Replxx::Color color = registry_.hasCommand(command)
        ? replxx::Replxx::Color::CYAN
        : replxx::Replxx::Color::RED;

      for (std::size_t i = 0; i < first_space && i < colors.size(); ++i)
      {
        colors[i] = color;
      }

      // Color arguments differently
      if (first_space < input.size())
      {
        for (std::size_t i = first_space + 1; i < colors.size(); ++i)
        {
          colors[i] = replxx::Replxx::Color::WHITE;
        }
      }
    });
  }

  std::string Terminal::colorize(const std::string& text, const char* color) const
  {
    return std::string(color) + text + kColorReset;
  }

  std::string Terminal::formatHelpLine(const std::string& name, const std::string& description) const
  {
    const int kColumnWidth = 20;
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

  void Terminal::setupBuiltins()
  {
    registerCommand("help", [this](const std::vector<std::string>&)
    {
      std::cout << kColorBold << kColorBrightYellow << "\n"
                << "╔══════════════════════════════════════════════════════════╗\n"
                << "║                    AVAILABLE COMMANDS                    ║\n"
                << "╚══════════════════════════════════════════════════════════╝\n"
                << kColorReset << "\n";

      const auto& names = registry_.getCommandNames();
      size_t max_name_len = 0;
      for (const auto& name : names)
      {
        max_name_len = std::max(max_name_len, name.size());
      }
      max_name_len = std::max(max_name_len, static_cast<size_t>(12));
      max_name_len = std::min(max_name_len, static_cast<size_t>(30));

      for (const auto& name : names)
      {
        std::cout << "  " << formatHelpLine(name, registry_.getDescription(name)) << "\n";
      }

      std::cout << "\n" << kColorDim << "  Tip: Use TAB for autocompletion\n"
                << "  Type 'quit' or press Ctrl+C to exit"
                << kColorReset << "\n\n";
      return 0;
    }, "List all available commands");

    registerCommand("history", [this](const std::vector<std::string>& args) -> int
    {
      bool show_all = false;
      int limit = 20; // Default show last 20 commands

      // Parse arguments
      for (const auto& arg : args)
      {
        if (arg == "--all" || arg == "-a")
        {
          show_all = true;
        }
        else if (arg == "--limit" || arg == "-l")
        {
          // Would need to parse next argument, but for simplicity we'll skip
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
        // Determine start index for display
        size_t start_index = show_all ? 0 : std::max(0, static_cast<int>(history_cache_.size()) - limit);
        size_t total_commands = total_history_count_ - history_cache_.size() + start_index + 1;

        for (size_t i = start_index; i < history_cache_.size(); ++i)
        {
          std::cout << "  " << colorize(std::to_string(total_commands + i - start_index), kColorBrightMagenta)
                    << "  " << colorize(history_cache_[i], kColorBrightCyan) << "\n";
        }

        std::cout << kColorDim << "\n  Total commands: " << total_history_count_
                  << " | Showing " << (history_cache_.size() - start_index)
                  << " of " << history_cache_.size() << " cached" << kColorReset << "\n";
      }
      std::cout << "\n";
      return 0;
    }, "Show command history [--all] [-l N]");

    registerCommand("clear", [](const std::vector<std::string>&)
    {
      std::cout << "\x1b[2J\x1b[H";
      return 0;
    }, "Clear the terminal screen");

    registerCommand("quit", [this](const std::vector<std::string>&)
    {
      std::cout << kColorBrightGreen << "Goodbye!" << kColorReset << "\n";
      stop();
      return 0;
    }, "Exit the terminal");

    registerCommand("echo", [](const std::vector<std::string>& args)
    {
      for (std::size_t i = 0; i < args.size(); ++i)
      {
        std::cout << args[i] << (i + 1 < args.size() ? " " : "");
      }
      std::cout << "\n";
      return 0;
    }, "Print arguments back to the terminal");

    registerCommand("version", [](const std::vector<std::string>&)
    {
      std::cout << kColorBrightCyan << "csm_cmd "
                << kColorBrightYellow << "1.0.0"
                << kColorReset << " - CSBot Terminal Interface\n";
      return 0;
    }, "Print terminal version");

    registerCommand("time", [](const std::vector<std::string>&)
    {
      const std::time_t now = std::time(nullptr);
      std::tm tm_buf{};
  #if defined(_WIN32)
      localtime_s(&tm_buf, &now);
  #else
      localtime_r(&now, &tm_buf);
  #endif
      std::cout << kColorBrightGreen << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
                << kColorReset << "\n";
      return 0;
    }, "Print the current date and time");

    //TODO register your commands here
  }

  void Terminal::flushHistoryCache()
  {
    if (pending_flush_.empty())
    {
      return;
    }

    // Open file in append mode
    std::ofstream history_file(history_path_, std::ios::app);
    if (!history_file.is_open())
    {
      Logger::instance().warn("Failed to open history file for flushing");
      return;
    }

    // Write all pending commands
    for (const auto& cmd : pending_flush_)
    {
      history_file << cmd << "\n";
    }

    history_file.close();
    pending_flush_.clear();

    Logger::instance().debug("Flushed " + std::to_string(pending_flush_.size()) + " commands to history file");
  }

  void Terminal::loadHistory()
  {
    // Load history from file into replxx
    repl_.history_load(history_path_);

    // Also populate our cache from the file
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

      // Keep only the last kMaxCachedHistory entries
      while (temp_cache.size() > kMaxCachedHistory)
      {
        temp_cache.pop_front();
      }
      history_cache_ = std::move(temp_cache);
    }

    Logger::instance().info("Loaded " + std::to_string(history_cache_.size()) + " history entries");
  }

  void Terminal::saveHistory()
  {
    // Final flush before saving
    flushHistoryCache();

    // Save full history from replxx
    repl_.history_save(history_path_);
    Logger::instance().info("Saved history");
  }

  void Terminal::addHistoryEntry(const std::string& line)
  {
    // Add to replxx history
    repl_.history_add(line);

    // Add to cache
    history_cache_.push_back(line);
    ++total_history_count_;

    // Keep cache size limited
    while (history_cache_.size() > kMaxCachedHistory)
    {
      // Move oldest command to pending flush
      pending_flush_.push_back(history_cache_.front());
      history_cache_.pop_front();

      // If pending flush reaches threshold, write to file
      if (pending_flush_.size() >= kHistoryFlushThreshold)
      {
        flushHistoryCache();
      }
    }
  }

  int Terminal::executeWithTimeout(const std::string& name, const std::vector<std::string>& args) const
  {
    auto future = std::async(std::launch::async, [this, name, args]()
    {
      return registry_.execute(name, args);
    });

    const auto status = future.wait_for(timeout_);
    if (status != std::future_status::ready)
    {
      Logger::instance().warn("Command timed out after " + std::to_string(timeout_.count()) + "ms: " + name);
      std::cerr << colorize("✗ Command timed out: ", kColorBrightRed)
                << colorize(name, kColorBrightYellow) << "\n";
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
      Logger::instance().warn("Rejected unknown command: " + name);
      std::cerr << colorize("✗ Unknown command: ", kColorBrightRed)
                << colorize(name, kColorBrightYellow)
                << colorize(" (not in whitelist)", kColorDim) << "\n";
      return -1;
    }

    Logger::instance().info("Executing command: " + name);

    try
    {
      return executeWithTimeout(name, args);
    }
    catch (const std::exception& ex)
    {
      Logger::instance().error("Command '" + name + "' threw: " + ex.what());
      std::cerr << colorize("✗ Command failed: ", kColorBrightRed)
                << colorize(ex.what(), kColorBrightYellow) << "\n";
      return -3;
    }
  }

  void Terminal::run()
  {
    running_ = true;

    std::cout << "(все баги закомичены хакерами)"
              << "\n╔══════════════════════════════════════════════════════════╗\n"
                << "║           WELCOME TO CSM_CMD TERMINAL v1.0.0             ║\n"
                << "║              Type 'help' to get started                  ║\n"
                << "╚══════════════════════════════════════════════════════════╝\n"
              << "\n";

    while (running_)
    {
      const char* raw_line = repl_.input(kPrompt);

      if (raw_line == nullptr) { break; }

      const std::string line(raw_line);

      if (line.empty()) { continue; }

      std::vector<std::string> tokens;
      try
      {
        tokens = parser_.parse(line);
      }
      catch (const ParseError& ex)
      {
        Logger::instance().warn(std::string("Parse error: ") + ex.what());
        std::cerr << colorize("✗ Parse error: ", kColorBrightRed)
                  << ex.what() << "\n";
        continue;
      }

      if (tokens.empty()) { continue; }

      // Add to history with efficient caching
      addHistoryEntry(line);

      dispatch(tokens);
    }

    saveHistory();
  }

  void Terminal::stop() { running_ = false; }

}  // namespace csm_cmd