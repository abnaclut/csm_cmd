#include <chrono>
#include <clocale>
#include <iostream>
#include <locale>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
  #include <fcntl.h>
  #include <io.h>
#endif

#include <csm_terminal.hpp>
#include <cmd_registration.hpp>

namespace
{

// Color constants for main.cpp (duplicate of Terminal's colors)
constexpr const char* kColorReset   = "\033[0m";
constexpr const char* kColorBold    = "\033[1m";
constexpr const char* kColorDim     = "\033[2m";
constexpr const char* kColorGreen   = "\033[32m";
constexpr const char* kColorCyan    = "\033[36m";
constexpr const char* kColorYellow  = "\033[33m";
constexpr const char* kColorRed     = "\033[31m";

constexpr const char* kColorBrightCyan    = "\033[96m";
constexpr const char* kColorBrightGreen   = "\033[92m";
constexpr const char* kColorBrightYellow  = "\033[93m";
constexpr const char* kColorBrightRed     = "\033[91m";
constexpr const char* kColorBrightMagenta = "\033[95m";

void printUsage(const char* prog_name)
{
  std::cout << kColorBrightCyan << "Usage:" << kColorReset
            << " " << prog_name << " [options]\n"
            << "\n"
            << kColorBrightYellow << "Options:" << kColorReset << "\n"
            << "  " << kColorGreen << "-nogui" << kColorReset
            << "            Run in terminal mode (default)\n"
            << "  " << kColorGreen << "--timeout-ms N" << kColorReset
            << "    Set command execution timeout in milliseconds\n"
            << "  " << kColorGreen << "-h, --help" << kColorReset
            << "        Show this help message\n"
            << "\n"
            << kColorBrightYellow << "Example:" << kColorReset << "\n"
            << "  " << prog_name << " -nogui --timeout-ms 5000\n";
}

}  // namespace

int main(int argc, char** argv)
{
#ifdef _WIN32
  // Set console to UTF-8
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  std::setlocale(LC_ALL, ".utf8");

  // Parse arguments
  std::chrono::milliseconds timeout = csm_cmd::Terminal::kDefaultTimeout;

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];

    if (arg == "-nogui")
    {
      // GUI mode not implemented, just ignore
      continue;
    }
    else if (arg == "--timeout-ms" && i + 1 < argc)
    {
      timeout = std::chrono::milliseconds(std::stoi(argv[++i]));
    }
    else if (arg == "--help" || arg == "-h")
    {
      printUsage(argv[0]);
      return 0;
    }
  }

  // Initialize terminal
  csm_cmd::initTerminal();

  auto& terminal = csm_cmd::Terminal::instance();

  // ============================================================
  // Register user commands
  // ============================================================

  terminal.registerCommand("greet",
    [](const std::vector<std::string>& args) -> int
  {
    std::string name = args.empty() ? "World" : args[0];
    std::cout << kColorBrightGreen << "Hello, " << name << "!" << kColorReset << "\n";
    return 0;
  }, "Greet someone by name");

  terminal.registerCommand("list",
    [](const std::vector<std::string>& args) -> int
  {
    std::vector<std::string> items = {
      "apple", "banana", "cherry", "date", "elderberry"
    };

    bool use_color = !args.empty() && args[0] == "--color";

    for (const auto& item : items)
    {
      if (use_color)
      {
        std::cout << kColorYellow << "•" << kColorReset << " "
                  << kColorCyan << item << kColorReset << "\n";
      }
      else
      {
        std::cout << "• " << item << "\n";
      }
    }

    return 0;
  }, "List demo items [--color for colored output]");

  terminal.registerAlias("ls", "list");

  terminal.registerCommand("slow",
    [](const std::vector<std::string>&) -> int
  {
    std::cout << kColorYellow << " Processing..." << kColorReset << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << kColorBrightGreen << "✓ Finally done!" << kColorReset << "\n";
    return 0;
  }, "Demonstrate timeout handling (sleeps 500ms)");

  terminal.registerCommand("stats",
    [](const std::vector<std::string>&) -> int
  {
    std::cout << kColorBold << kColorBrightCyan
              << "╔═══════════════════════════════════════╗\n"
              << "║           SYSTEM STATISTICS           ║\n"
              << "╚═══════════════════════════════════════╝"
              << kColorReset << "\n"
              << kColorBrightGreen << "✓" << kColorReset
              << " Memory: " << kColorBrightYellow << "8.4GB" << kColorReset << " / 16GB\n"
              << kColorBrightGreen << "✓" << kColorReset
              << " CPU:    " << kColorBrightYellow << "23%" << kColorReset << "\n"
              << kColorBrightGreen << "✓" << kColorReset
              << " Uptime: " << kColorBrightYellow << "12h 34m" << kColorReset << "\n";
    return 0;
  }, "Show system statistics (demo)");

  // ============================================================
  // Run terminal
  // ============================================================

  terminal.run();

  return 0;
}