# CSM-CMD: CSBot cmd module

Терминальный обработчик команд для режима --nogui на базе replxx.

## Фичи

- Цветной вывод с поддержкой ANSI
- Автодополнение команд по Tab (и обработка стрелок)
- История команд с сохранением в файл
- Алиасы для команд
- Таймаут выполнения команд
- Ротация логов
- Защита от ANSI-инъекций
- Легко интегрировать
## Сборка
```
./rebuild.bat
```
```
Опции сборки:

BUILD_TERMINAL_TESTS    - Сборка тестов (ON по умолчанию)
BUILD_CSM_CMD_EXAMPLES  - Сборка примеров (OFF по умолчанию)

Требования:

CMake >= 3.16
Компилятор с поддержкой C++20 (можно легко понизить но придется рефакторить)
Доступ к сети (FetchContent скачивает replxx и googletest)
```
## Запуск

```
./build/bin/csm_terminal
./build/bin/csm_terminal --timeout-ms 200
./build/bin/csm_terminal --help
```

```
Встроенные команды:

  help      - Список всех доступных команд
  history   - Показать историю команд
  clear     - Очистить экран терминала
  quit      - Выйти из терминала
  echo      - Вывести аргументы обратно
  version   - Версия терминала
  time      - Текущая дата и время

Демонстрационные команды(для тестов, будут удалятся и добавлятся):

  greet     - Приветствие
  list      - Список элементов
  ls        - Алиас для list
  slow      - Команда с задержкой (демонстрация таймаута)
  stats     - Демонстрация системной статистики
```
## Интеграция

Через add_subdirectory:
```
add_subdirectory(third_party/csm_cmd)
target_link_libraries(your_app PRIVATE csm_cmd)
```
Через FetchContent:
```
include(FetchContent)
FetchContent_Declare(
csm_cmd
GIT_REPOSITORY https://github.com/abnaclut/CSM-CMD.git
GIT_TAG main
)
FetchContent_MakeAvailable(csm_cmd)
target_link_libraries(your_app PRIVATE csm_cmd)
```
Через find_package (после установки):
```
find_package(csm_cmd REQUIRED)
target_link_libraries(your_app PRIVATE csm_cmd)
```
## Использование в коде
```
Публичный API (рекомендуемый):

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

Внутренний API (для тестирования):

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
Регистрация команд с аргументами:
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
Обработка ошибок в командах:
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


## Безопасность

- Максимальная длина ввода - 4096 символов
- Выполняются только явно зарегистрированные команды
- Нет вызовов system() или exec*()
- Вывод экранируется для защиты от ANSI-инъекций
- Каждая команда выполняется с таймаутом (по умолчанию 100 мс)
- Логи пишутся в ~/.csm_cmd.log с ротацией (макс. 1 МБ, до 3 резервных копий)
- История сохраняется в ~/.csm_cmd_history (макс. 1000 строк)
- Нормально работает Ctrl+C (SIGINT)

## Структура
```
csm_cmd/
├── CMakeLists.txt
├── README.md
├── include/
│   └── csm_cmd/
│       ├── cmd_registration.hpp
│       ├── csm_terminal.hpp
│       ├── command_parser.hpp
│       └── command_registry.hpp
├── src/
│   ├── CmdParser/
│   │   └── command_parser.cpp
│   ├── CmdReg/
│   │   ├── command_registry.cpp
│   │   └── cmd_registration.cpp
│   ├── CsmTerminal/
│   │   └── csm_terminal.cpp
│   ├── Logger/
│   │   ├── logger.cpp
│   │   └── logger.hpp
│   ├── Tests/
│   │   ├── test_parser.cpp
│   │   ├── test_registry.cpp
│   │   ├── test_completion.cpp
│   │   └── test_timeout.cpp
│   └── main.cpp
└── examples/
    ├── simple/
    │   └── main.cpp
    └── custom/
        └── main.cpp
    todo: add more
```
[LICENSE](LICENSE)

Задать вопрос: abnaclut@gmail.com