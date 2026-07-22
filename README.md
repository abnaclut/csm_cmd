# CSM-CMD: CSBot cmd module

**Terminal command handler for --nogui mode based on replxx.**

---

## Features

- Color output with ANSI support
- Tab autocompletion with arrow key handling
- Command history with file persistence
- Command aliases
- Command execution timeout
- Log rotation
- ANSI injection protection
- Easy integration

---

## Build

```
./rebuild.bat
```
```

Build options:
___________________________________________________________________
| Option                   | Description                | Default |
|--------------------------|----------------------------|---------|
| `BUILD_TERMINAL_TESTS`   | Build unit tests           | ON      |
| `BUILD_CSM_CMD_EXAMPLES` | Build example applications | OFF     |
|-----------------------------------------------------------------|
Requirements:

- CMake >= 3.16
- C++20 compiler (can be downgraded with a bit of refactoring)
- Internet access (FetchContent downloads replxx and googletest)
```
## Run

```
./build/bin/csm_terminal.exe
./build/bin/csm_cmd_tests.exe

(in case someone has a reason to run a terminal module that is useless by itself)
```

```
В
Built-in commands:
___________________________________________
| Command   | Description                 |
|-----------|-----------------------------|
| `help`    | List all available commands |
| `history` | Show command history        |
| `clear`   | Clear terminal screen       |
| `quit`    | Exit terminal               |
| `exit`    | Exit terminal               |
| `echo`    | Echo arguments              |
| `version` | Show terminal version       |
| `time`    | Current date and time       |
|-----------------------------------------|

Test commands (for testing, will be removed/added randomly):
*Note: they are removed via .clear() before rigistering your commands. 

| Command | Description                       |
|---------|-----------------------------------|
| `greet` | Hello world                       |
| `list`  | List items                        |
| `ls`    | Alias for list                    |
| `slow`  | Command with delay                |
| `stats` | System statistics demo            |
|---------------------------------------------|
```
## Integration

### Via add_subdirectory

```cmake
add_subdirectory(third_party/csm_cmd)
target_link_libraries(your_app PRIVATE csm_cmd)
```
Via FetchContent:
```
include(FetchContent)
FetchContent_Declare(
    csm_cmd
    GIT_REPOSITORY https://github.com/abnaclut/csm_cmd.git
    GIT_TAG main
)
FetchContent_MakeAvailable(csm_cmd)
target_link_libraries(your_app PRIVATE csm_cmd)
```
Via find_package (after installation):
```
find_package(csm_cmd REQUIRED)
target_link_libraries(your_app PRIVATE csm_cmd)
```
## Usage
```
Public API (recommended):

#include <csm_cmd/cmd_registration.hpp>

void registerMyCommands()
{
  using namespace csm_cmd;

  regCmd("start", [](const std::vector<std::string>& args) -> int
  {
    std::cout << "Starting service...\n";
    return 0;
  }, "Start the service");

  regCmd("stop", [](const std::vector<std::string>& args) -> int
  {
    std::cout << "Stopping service...\n";
    return 0;
  }, "Stop the service");

  regAlias("run", "start");
}

int main()
{
  registerMyCommands();
  csm_cmd::initTerminal();
  csm_cmd::runTerminal();
  return 0;
}

Internal API (for testing):

#include <csm_cmd/csm_terminal.hpp>

void testTerminal()
{
  csm_cmd::Terminal terminal;
  terminal.registerCommand("test", [](const std::vector<std::string>& args) -> int
  {
    std::cout << "Test command executed\n";
    return 0;
  }, "Test command");
}
```
### Registering commands with arguments:
```
regCmd("set", [](const std::vector<std::string>& args) -> int
{
  if (args.size() < 2)
  {
    std::cerr << "Usage: set <key> <value>\n";
    return 1;
  }

  std::cout << "Setting " << args[0] << " = " << args[1] << "\n"; //logic here
  return 0;
}, "Set configuration value");
```
###     Error handling in commands:
```
regCmd("divide", [](const std::vector<std::string>& args) -> int
{
  if (args.size() < 2)
  {
    std::cerr << "Usage: divide <a> <b>\n";
    return 1;
  }

  try
  {
    int a = std::stoi(args[0]); //https://en.cppreference.com/cpp/string/basic_string/stol
    int b = std::stoi(args[1]);
    
    if (b == 0)
    {
      std::cerr << "Error: division by zero\n";
      return 1;
    }
  
    std::cout << "Result: " << (a / b) << "\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }
}, "Divide two numbers");
```


### Safety

 - Maximum input length: 4096 characters

 - Only explicitly registered commands can be executed

 - No system() or exec*() calls

 - Output is escaped to prevent ANSI injection

 - Each command runs with a timeout (default: 100 ms)

 - Logs written to ~/.csm_cmd.log with rotation (max 1 MB, up to 3 backups)

 - History saved to ~/.csm_cmd_history (max 1000 lines)

 - Ctrl+C (SIGINT) handled gracefully

 - will use spdlog in future

### Structure
```
csm_cmd/
├── CMakeLists.txt
├── README.md
├── include/
│   └── csm_cmd/
│       ├── cmd_registration.hpp
│       ├── csm_terminal.hpp
│       ├── cmd_parser.hpp
│       └── cmd_registry.hpp
├── src/
│   ├── cmd_parser.cpp
│   ├── cmd_registry.cpp
│   ├── cmd_registration.cpp
│   ├── csm_terminal.cpp
│   ├── logger.cpp
│   ├── main.cpp
│   └── tests/
│       ├── test_parser.cpp
│       ├── test_registry.cpp
│       ├── test_completion.cpp
│       └── test_timeout.cpp
├── examples/ todo
└── docs/ todo (!!!)
```
[LICENSE](LICENSE)

Ask a question: abnaclut@gmail.com