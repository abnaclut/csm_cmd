#include <cmd_registry.hpp>

#include <algorithm>
#include <ranges>

namespace csm_cmd
{

  void CommandRegistry::registerCommand(const std::string& name, CommandHandler handler, const std::string& description)
  {
    if (name.empty())             { throw CommandError("command name must not be empty"); }
    if (commands_.contains(name)) { throw CommandError("command already registered: " + name); }

    commands_.emplace(name, CommandInfo{name, description, std::move(handler)});
  }

  void CommandRegistry::registerAlias(const std::string& alias, const std::string& target)
  {
    if (alias.empty() || target.empty()) { throw CommandError("alias and target must not be empty"); }
    if (alias == target)                 { throw CommandError("alias cannot be the same as target"); }
    if (aliases_.contains(alias))        { throw CommandError("alias already exists: " + alias); }
    if (!commands_.contains(target))     { throw CommandError("cannot alias unknown command: " + target); }

    aliases_[alias] = target;
  }

  std::string CommandRegistry::resolveAlias(const std::string& name) const
  {
    if (const auto it = aliases_.find(name); it != aliases_.end()) { return it->second; }
    return name;
  }

  bool CommandRegistry::hasCommand(const std::string& name) const
  {
    if (name.empty()) { return false; }
    return commands_.contains(resolveAlias(name));
  }

  int CommandRegistry::execute(const std::string& name, const std::vector<std::string>& args) const
  {
    const std::string resolved = resolveAlias(name);
    const auto it = commands_.find(resolved);
    if (it == commands_.end()) { throw CommandError("unknown command: " + name); }

    return it->second.handler(args);
  }

  std::string CommandRegistry::getDescription(const std::string& name) const
  {
    const auto it = commands_.find(resolveAlias(name));
    if (it == commands_.end()) { return ""; }
    return it->second.description;
  }

  std::vector<std::string> CommandRegistry::getCommandNames() const
  {
    std::vector<std::string> names;
    names.reserve(commands_.size());
    for (const auto& key : commands_ | std::views::keys) { names.push_back(key); }
    std::ranges::sort(names);
    return names;
  }

  const std::unordered_map<std::string, CommandRegistry::CommandInfo>& CommandRegistry::getCommands() const
  {
    return commands_;
  }

  std::vector<std::string> CommandRegistry::getCompletions(const std::string& prefix) const
  {
    std::vector<std::string> result;

    for (const auto& key : commands_ | std::views::keys)
    {
      if (key.compare(0, prefix.size(), prefix) == 0) { result.push_back(key); }
    }

    for (const auto& key : aliases_ | std::views::keys)
    {
      if (key.compare(0, prefix.size(), prefix) == 0) { result.push_back(key); }
    }

    std::ranges::sort(result);
    return result;
  }

    void CommandRegistry::clear() noexcept
  {
    commands_.clear();
    aliases_.clear();
  }

  unsigned int CommandRegistry::size() const noexcept { return static_cast<unsigned int>(commands_.size()); }

}  // namespace csm_cmd