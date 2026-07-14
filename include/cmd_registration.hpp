#ifndef CSM_CMD_CMD_REGISTRATION_HPP
#define CSM_CMD_CMD_REGISTRATION_HPP

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "csm_terminal.hpp"
#include "command_registry.hpp"

namespace csm_cmd
{

  /**
   * @brief PUBLIC API: Command registration for external projects
   *
   * This is the recommended way to register commands in your project:
   * @code
   *   #include <csm_cmd/cmd_registration.hpp>
   *
   *   void setupCommands() {
   *       csm_cmd::regCmd("start", [](const std::vector<std::string>& args) -> int {
   *           std::cout << "Starting...\n";
   *           return 0;
   *       }, "Start the service");
   *   }
   * @endcode
   */
  class CommandRegistrar
  {
  public:
    using CommandHandler = std::function<int(const std::vector<std::string>&)>;

    /**
     * @brief Register a command globally
     * @param name Command name (e.g., "start", "stop")
     * @param handler Function to execute when command is called
     * @param description Human-readable description for help
     * @throws std::invalid_argument if name is empty or handler is null
     */
    static void regCmd(const std::string& name,
                       CommandHandler handler,
                       const std::string& description);

    /**
     * @brief Register an alias for an existing command
     * @param alias Alternative name for the command
     * @param target Original command name
     * @throws std::invalid_argument if alias or target is empty
     */
    static void regAlias(const std::string& alias, const std::string& target);

    /**
     * @brief Get all registered commands (for debugging)
     * @return Const reference to command map
     */
    static const std::unordered_map<std::string, CommandRegistry::CommandInfo>& getCommands();

    /**
     * @brief Clear all registered commands (for testing)
     */
    static void clearCommands();

    /**
     * @brief Check if terminal is initialized
     */
    static bool isInitialized();
  };

  // Convenience aliases for shorter syntax
  inline void regCmd(const std::string& name,
                     CommandRegistrar::CommandHandler handler,
                     const std::string& description)
  {
    CommandRegistrar::regCmd(name, std::move(handler), description);
  }

  inline void regAlias(const std::string& alias, const std::string& target)
  {
    CommandRegistrar::regAlias(alias, target);
  }

  // Terminal lifecycle management
  void initTerminal(std::chrono::milliseconds timeout = Terminal::kDefaultTimeout);
  void runTerminal();
  Terminal* getTerminal();
  bool isTerminalInitialized();

} // namespace csm_cmd

#endif // CSM_CMD_CMD_REGISTRATION_HPP