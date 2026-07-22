#ifndef CSM_CMD_COMMAND_REGISTRY_HPP
#define CSM_CMD_COMMAND_REGISTRY_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace csm_cmd
{

  /**
   * @brief Thrown when executing an unregistered or malformed command.
   */
  class CommandError final : public std::runtime_error
  {
  public:
    explicit CommandError(const std::string& message) : std::runtime_error(message) {}
  };

  /**
   * @brief Command registry that maintains a whitelist of executable commands
   */
  class CommandRegistry
  {
  public:
    using CommandHandler = std::function<int(const std::vector<std::string>&)>;

    struct CommandInfo
    {
      std::string name;
      std::string description;
      CommandHandler handler;
    };

    /**
     * @brief Register a new command
     * @throws CommandError if name is empty or already registered
     */
    void registerCommand(const std::string& name, CommandHandler handler, const std::string& description);

    /**
     * @brief Register an alias for an existing command
     * @throws CommandError if target command doesn't exist
     */
    void registerAlias(const std::string& alias, const std::string& target);

    /**
     * @brief Check if a command or alias exists
     */
    bool hasCommand(const std::string& name) const;

    /**
     * @brief Execute a command by name
     * @throws CommandError if command not registered
     */
    int execute(const std::string& name, const std::vector<std::string>& args) const;

    /**
     * @brief Get command description
     */
    [[nodiscard]] std::string getDescription(const std::string& name) const;

    /**
     * @brief Get all registered command names, sorted alphabetically
     */
    [[nodiscard]] std::vector<std::string> getCommandNames() const;

    /**
     * @brief Get a map of registered commands (name; description)
     */
    [[nodiscard]] const std::unordered_map<std::string, CommandInfo>& getCommands() const;

    /**
     * @brief Get completions for a given prefix
     */
    [[nodiscard]] std::vector<std::string> getCompletions(const std::string& prefix) const;

    /**
       * @brief Clear command registry.
       */
    void clear() noexcept;

    /**
       * @brief Get the registered command count.
       */
    unsigned int size() const noexcept;

  private:
    std::string resolveAlias(const std::string& name) const;

    std::unordered_map<std::string, CommandInfo> commands_;
    std::unordered_map<std::string, std::string> aliases_;
  };

}  // namespace csm_cmd

#endif  // CSM_CMD_COMMAND_REGISTRY_HPP