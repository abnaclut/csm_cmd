#ifndef CSM_CMD_LOGGER_HPP
#define CSM_CMD_LOGGER_HPP

#include <chrono>
#include <fstream>
#include <mutex>
#include <string>

namespace csm_cmd
{

enum class LogLevel
{
  kDebug,
  kInfo,
  kWarn,
  kError
};

/**
 * @brief Simple single-threaded rotating file logger.
 *
 * Writes timestamped lines to a log file and rotates the file once it
 * exceeds a configured size, keeping a bounded number of backups.
 */
class Logger
{
public:
  /**
   * @brief Access the process-wide logger instance.
   */
  static Logger& instance();

  /**
   * @brief Configure the log file path, rotation size and backup count.
   */
  void configure(const std::string& file_path, std::size_t max_bytes = 1024 * 1024, int max_backups = 3);

  /**
   * @brief Write a message at the given log level.
   */
  void log(LogLevel level, const std::string& message);

  void debug(const std::string& message);
  void info(const std::string& message);
  void warn(const std::string& message);
  void error(const std::string& message);

private:
  Logger() = default;

  void rotateIfNeeded();
  static std::string levelToString(LogLevel level);
  static std::string timestamp();

  std::ofstream stream_;
  std::string file_path_;
  std::size_t max_bytes_ = 1024 * 1024;
  int max_backups_ = 3;
  bool configured_ = false;
};

}  // namespace csm_cmd

#endif  // CSM_CMD_LOGGER_HPP
