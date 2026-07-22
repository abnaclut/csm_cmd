#include <cmd_registration.hpp>
#include <logger.hpp>

#include <mutex>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace csm_cmd
{

  namespace
  {
    // Global state
    Terminal* g_terminal = nullptr;
    std::mutex g_terminal_mutex;
    bool g_initialized = false;

    // Pending registrations (for commands registered before terminal init)
    struct PendingCommand
    {
      std::string name;
      CommandRegistrar::CommandHandler handler;
      std::string description;
    };

    struct PendingAlias
    {
      std::string alias;
      std::string target;
    };

    std::vector<PendingCommand> g_pending_commands;
    std::vector<PendingAlias> g_pending_aliases;
    std::mutex g_pending_mutex;

    void applyPendingRegistrations()
    {
      std::lock_guard<std::mutex> lock(g_pending_mutex);

      if (!g_terminal) { return; }

      auto& registry = g_terminal->getRegistry();

      for (auto& pending : g_pending_commands)
      {
        try
        {
          registry.registerCommand(pending.name, std::move(pending.handler), pending.description);
          Logger::instance().info("Applied pending command: " + pending.name);
        }
        catch (const std::exception& e)
        {
          Logger::instance().error("Failed to apply pending command '" + pending.name + "': " + e.what());
        }
      }
      g_pending_commands.clear();

      for (auto& pending : g_pending_aliases)
      {
        try
        {
          registry.registerAlias(pending.alias, pending.target);
          Logger::instance().info("Applied pending alias: " + pending.alias + " -> " + pending.target);
        }
        catch (const std::exception& e)
        {
          Logger::instance().error("Failed to apply pending alias '" + pending.alias + "': " + e.what());
        }
      }
      g_pending_aliases.clear();
    }

  }  // anonymous namespace

  // CommandRegistrar implementation
  void CommandRegistrar::regCmd(const std::string& name, CommandHandler handler, const std::string& description)
  {
    if (name.empty()) { throw std::invalid_argument("Command name cannot be empty"); }
    if (!handler)     { throw std::invalid_argument("Command handler cannot be null"); }

    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_initialized && g_terminal)
    {
      auto& registry = g_terminal->getRegistry();

      // Check if command already exists
      if (registry.hasCommand(name)) { throw std::runtime_error("Command already exists: " + name); }

      // Direct registration
      registry.registerCommand(name, std::move(handler), description);
      Logger::instance().info("Registered command: " + name);
    }
    else
    {
      // Deferred registration
      std::lock_guard<std::mutex> pending_lock(g_pending_mutex);
      g_pending_commands.push_back({name, std::move(handler), description});
      Logger::instance().debug("Queued command for later registration: " + name);
    }
  }

  void CommandRegistrar::regAlias(const std::string& alias, const std::string& target)
  {
    if (alias.empty() || target.empty()) { throw std::invalid_argument("Alias and target cannot be empty"); }

    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_initialized && g_terminal)
    {
      auto& registry = g_terminal->getRegistry();

      // Check if target exists
      if (!registry.hasCommand(target)) { throw std::runtime_error("Target command not found: " + target); }
      // Check if alias already exists
      if (registry.hasCommand(alias)) { throw std::runtime_error("Alias already exists: " + alias); }

      registry.registerAlias(alias, target);
      Logger::instance().info("Registered alias: " + alias + " -> " + target);
    }
    else
    {
      std::lock_guard<std::mutex> pending_lock(g_pending_mutex);
      g_pending_aliases.push_back({alias, target});
      Logger::instance().debug("Queued alias for later registration: " + alias + " -> " + target);
    }
  }

  std::unordered_map<std::string, CommandRegistry::CommandInfo> CommandRegistrar::getCommands()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_terminal)
    {
      auto& registry = g_terminal->getRegistry();
      return registry.getCommands();  // return copy
    }

    return {};
  }

  void CommandRegistrar::clearCommands()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    std::lock_guard<std::mutex> pending_lock(g_pending_mutex);

    g_pending_commands.clear();
    g_pending_aliases.clear();

    if (g_terminal)
    {
      auto& registry = g_terminal->getRegistry();
      registry.clear();
    }

    Logger::instance().debug("All commands cleared");
  }

  bool CommandRegistrar::isInitialized()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    return g_initialized && g_terminal != nullptr;
  }

  // Terminal lifecycle functions
  void initTerminal()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (!g_initialized)
    {
      g_terminal = &Terminal::instance();
      g_initialized = true;

      // Apply any pending registrations
      applyPendingRegistrations();

      const auto& registry = g_terminal->getRegistry();
      const auto count = registry.size();
      Logger::instance().info("Terminal initialized with " + std::to_string(count) + " commands");
    }
  }

  void runTerminal()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_initialized && g_terminal) { g_terminal->run(); }
    else { Logger::instance().warn("Attempted to run terminal but it was not initialized"); }
  }

  Terminal* getTerminal()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    return g_terminal;
  }

  bool isTerminalInitialized()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    return g_initialized && g_terminal != nullptr;
  }

}  // namespace csm_cmd