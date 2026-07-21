#include "../../include/cmd_registration.hpp"
#include "../Logger/logger.hpp"

#include <mutex>
#include <memory>
#include <stdexcept>

namespace csm_cmd
{

  namespace
  {
    // Global state
    std::unique_ptr<Terminal> g_terminal;
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

    // Registered commands storage
    std::unordered_map<std::string, CommandRegistry::CommandInfo> g_registered_commands;
    std::mutex g_registered_mutex;

    void applyPendingRegistrations()
    {
      std::lock_guard<std::mutex> lock(g_pending_mutex);

      for (const auto& cmd : g_pending_commands)
      {
        if (g_terminal)
        {
          g_terminal->registerCommand(cmd.name, cmd.handler, cmd.description);
          Logger::instance().info("Applied pending command: " + cmd.name);

          std::lock_guard<std::mutex> reg_lock(g_registered_mutex);
          g_registered_commands[cmd.name] = {cmd.name, cmd.description, cmd.handler};
        }
      }
      g_pending_commands.clear();

      for (const auto& alias : g_pending_aliases)
      {
        if (g_terminal)
        {
          g_terminal->registerAlias(alias.alias, alias.target);
          Logger::instance().info("Applied pending alias: " + alias.alias + " -> " + alias.target);
        }
      }
      g_pending_aliases.clear();
    }
  }

  // CommandRegistrar implementation
  void CommandRegistrar::regCmd(const std::string& name,
                                 CommandHandler handler,
                                 const std::string& description)
  {
    if (name.empty())
    {
      throw std::invalid_argument("Command name cannot be empty");
    }

    if (!handler)
    {
      throw std::invalid_argument("Command handler cannot be null");
    }

    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_initialized && g_terminal)
    {
      // Direct registration
      g_terminal->registerCommand(name, std::move(handler), description);
      Logger::instance().info("Registered command: " + name);

      std::lock_guard<std::mutex> reg_lock(g_registered_mutex);
      g_registered_commands[name] = {name, description, handler};
    }
    else
    {
      // Deferred registration (will be applied when terminal starts)
      std::lock_guard<std::mutex> pending_lock(g_pending_mutex);
      g_pending_commands.push_back({name, std::move(handler), description});
      Logger::instance().debug("Queued command for later registration: " + name);
    }
  }

  void CommandRegistrar::regAlias(const std::string& alias, const std::string& target)
  {
    if (alias.empty() || target.empty())
    {
      throw std::invalid_argument("Alias and target cannot be empty");
    }

    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_initialized && g_terminal)
    {
      g_terminal->registerAlias(alias, target);
      Logger::instance().info("Registered alias: " + alias + " -> " + target);
    }
    else
    {
      std::lock_guard<std::mutex> pending_lock(g_pending_mutex);
      g_pending_aliases.push_back({alias, target});
      Logger::instance().debug("Queued alias for later registration: " + alias + " -> " + target);
    }
  }

  const std::unordered_map<std::string, CommandRegistry::CommandInfo>& CommandRegistrar::getCommands()
  {
    std::lock_guard<std::mutex> lock(g_registered_mutex);
    return g_registered_commands;
  }

  void CommandRegistrar::clearCommands()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    std::lock_guard<std::mutex> pending_lock(g_pending_mutex);
    std::lock_guard<std::mutex> reg_lock(g_registered_mutex);

    g_pending_commands.clear();
    g_pending_aliases.clear();
    g_registered_commands.clear();

    Logger::instance().debug("All commands cleared");
  }

  bool CommandRegistrar::isInitialized()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    return g_initialized && g_terminal != nullptr;
  }

  // Terminal lifecycle functions
  void initTerminal(std::chrono::milliseconds timeout)
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (!g_initialized)
    {
      g_terminal = std::make_unique<Terminal>(timeout);
      g_initialized = true;
      applyPendingRegistrations();
      Logger::instance().info("Terminal initialized with " +
                              std::to_string(g_registered_commands.size()) + " commands");
    }
  }

  void runTerminal()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);

    if (g_initialized && g_terminal)
    {
      g_terminal->run();
    }
    else
    {
      Logger::instance().warn("Attempted to run terminal but it was not initialized");
    }
  }

  Terminal* getTerminal()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    return g_terminal.get();
  }

  bool isTerminalInitialized()
  {
    std::lock_guard<std::mutex> lock(g_terminal_mutex);
    return g_initialized && g_terminal != nullptr;
  }

} // namespace csm_cmd