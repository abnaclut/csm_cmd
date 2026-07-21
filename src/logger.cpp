#include "../Logger/logger.hpp"

#include <array>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace csm_cmd
{

Logger& Logger::instance()
{
  static Logger logger;
  return logger;
}

void Logger::configure(const std::string& file_path, const std::size_t max_bytes, const int max_backups)
{
  file_path_ = file_path;
  max_bytes_ = max_bytes;
  max_backups_ = max_backups;
  stream_.open(file_path_, std::ios::app);
  configured_ = true;
}

std::string Logger::levelToString(const LogLevel level)
{
  switch (level)
  {
    case LogLevel::kDebug:
      return "DEBUG";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarn:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
  }
  return "UNKNOWN";
}

std::string Logger::timestamp()
{
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf{};
#if defined(_WIN32)
  localtime_s(&tm_buf, &now_time);
#else
  localtime_r(&now_time, &tm_buf);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

void Logger::rotateIfNeeded()
{
  if (!configured_)
  {
    return;
  }

  stream_.flush();
  std::error_code ec;
  const auto size = stream_.tellp();
  if (size < 0 || static_cast<std::size_t>(size) < max_bytes_)
  {
    return;
  }

  stream_.close();

  for (int i = max_backups_ - 1; i >= 1; --i)
  {
    const std::string src = file_path_ + "." + std::to_string(i);
    const std::string dst = file_path_ + "." + std::to_string(i + 1);
    std::rename(src.c_str(), dst.c_str());
  }

  const std::string first_backup = file_path_ + ".1";
  std::rename(file_path_.c_str(), first_backup.c_str());

  stream_.open(file_path_, std::ios::app);
}

void Logger::log(const LogLevel level, const std::string& message)
{
  if (!configured_)
  {
    return;
  }

  rotateIfNeeded();
  stream_ << "[" << timestamp() << "] [" << levelToString(level) << "] " << message << "\n";
  stream_.flush();
}

void Logger::debug(const std::string& message)
{
  log(LogLevel::kDebug, message);
}

void Logger::info(const std::string& message)
{
  log(LogLevel::kInfo, message);
}

void Logger::warn(const std::string& message)
{
  log(LogLevel::kWarn, message);
}

void Logger::error(const std::string& message)
{
  log(LogLevel::kError, message);
}

}  // namespace csm_cmd
