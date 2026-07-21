#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <csm_terminal.hpp>
#include <clocale>
#include <locale>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

namespace
{
  void printUsage(const char* prog_name)
  {
    std::cout << "\033[1;36mUsage:\033[0m " << prog_name << " [options]\n"
              << "\n"
              << "\033[1;33mOptions:\033[0m\n"
              << "  \033[32m-nogui\033[0m            Run in terminal mode (default)\n"
              << "  \033[32m--timeout-ms N\033[0m    Set command execution timeout in milliseconds\n"
              << "  \033[32m-h, --help\033[0m        Show this help message\n"
              << "\n"
              << "\033[1;33mExample:\033[0m\n"
              << "  " << prog_name << " -nogui --timeout-ms 5000\n";
  }
}  // namespace

int main(const int argc, char** argv)
{
#ifdef _WIN32
 SetConsoleOutputCP(CP_UTF8);
 SetConsoleCP(CP_UTF8);
  // Set stdout to UTF-8
  //_setmode(_fileno(stdout), _O_U8TEXT);
#endif
  std::setlocale(LC_ALL, ".utf8");

  bool nogui = true;

  std::chrono::milliseconds timeout = csm_cmd::Terminal::kDefaultTimeout;

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];
    if (arg == "-nogui")
    {
      nogui = true;
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

  if (!nogui)
  {
    printUsage(argv[0]);
    return 1;
  }

  csm_cmd::Terminal terminal(timeout);

  // Example commands
  terminal.registerCommand("greet", [](const std::vector<std::string>& args)
  {
    std::string name = args.empty() ? "World" : args[0];
    std::cout << "\033[1;32mHello, " << name << "!\033[0m \n";
    return 0;
  }, "Greet someone by name");

  terminal.registerCommand("list", [](const std::vector<std::string>& args)
  {
    std::vector<std::string> items = {"apple", "banana", "cherry", "date", "elderberry"};

    if (!args.empty() && args[0] == "--color")
    {
      for (const auto& item : items)
      {
        std::cout << "\033[1;33m•\033[0m \033[36m" << item << "\033[0m\n";
      }
    }
    else
    {
      for (const auto& item : items)
      {
        std::cout << "• " << item << "\n";
      }
    }
    return 0;
  }, "List demo items [--color for colored output]");

  terminal.registerAlias("ls", "list");

  terminal.registerCommand("slow", [](const std::vector<std::string>&)
  {
    std::cout << "\033[33m Processing...\033[0m\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "\033[32m✓ Finally done!\033[0m\n";
    return 0;
  }, "Demonstrate timeout handling (sleeps 500ms)");

  terminal.registerCommand("stats", [](const std::vector<std::string>&)
  {
    std::cout << "\033[1;36m╔═══════════════════════════════════════╗\n"
              << "║           SYSTEM STATISTICS           ║\n"
              << "╚═══════════════════════════════════════╝\033[0m\n"
              << "\033[32m✓\033[0m Memory: \033[1;33m8.4GB\033[0m / 16GB\n"
              << "\033[32m✓\033[0m CPU:    \033[1;33m23%\033[0m\n"
              << "\033[32m✓\033[0m Uptime: \033[1;33m12h 34m\033[0m\n";
    return 0;
  }, "Show system statistics (demo)");

  terminal.run();

  return 0;
}