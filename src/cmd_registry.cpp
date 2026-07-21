#include <cmd_registry.hpp>

#include <algorithm>

namespace csm_cmd
{

void CommandRegistry::registerCommand(const std::string& name, CommandHandler handler, const std::string& description)
{
  if (name.empty())
  {
    throw CommandError("command name must not be empty");
  }

  if (commands_.find(name) != commands_.end())
  {
    throw CommandError("command already registered: " + name);
  }

  commands_.emplace(name, CommandInfo{name, description, std::move(handler)});
}

void CommandRegistry::registerAlias(const std::string& alias, const std::string& target)
{
  if (commands_.find(target) == commands_.end())
  {
    throw CommandError("cannot alias unknown command: " + target);
  }

  aliases_[alias] = target;
}

std::string CommandRegistry::resolveAlias(const std::string& name) const
{
  const auto it = aliases_.find(name);
  if (it != aliases_.end())
  {
    return it->second;
  }
  return name;
}

bool CommandRegistry::hasCommand(const std::string& name) const
{
  return commands_.find(resolveAlias(name)) != commands_.end();
}

int CommandRegistry::execute(const std::string& name, const std::vector<std::string>& args) const
{
  const std::string resolved = resolveAlias(name);
  const auto it = commands_.find(resolved);
  if (it == commands_.end())
  {
    throw CommandError("unknown command: " + name);
  }

  return it->second.handler(args);
}

std::string CommandRegistry::getDescription(const std::string& name) const
{
  const auto it = commands_.find(resolveAlias(name));
  if (it == commands_.end())
  {
    return "";
  }
  return it->second.description;
}

std::vector<std::string> CommandRegistry::getCommandNames() const
{
  std::vector<std::string> names;
  names.reserve(commands_.size());
  for (const auto& entry : commands_)
  {
    names.push_back(entry.first);
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> CommandRegistry::getCompletions(const std::string& prefix) const
{
  std::vector<std::string> result;

  for (const auto& entry : commands_)
  {
    if (entry.first.compare(0, prefix.size(), prefix) == 0)
    {
      result.push_back(entry.first);
    }
  }

  for (const auto& entry : aliases_)
  {
    if (entry.first.compare(0, prefix.size(), prefix) == 0)
    {
      result.push_back(entry.first);
    }
  }

  std::sort(result.begin(), result.end());
  return result;
}

}  // namespace csm_cmd