#include <iostream>
#include <string>

namespace DJS {
class Logger {
  private:
    static void Log(const std::string& message) { std::cout << message << "\n"; }

  public:
    static void Debug(const std::string& message) { Logger::Log("[DEBUG]: " + message); }
    static void Error(const std::string& message) { Logger::Log("[ERROR]: " + message); }
    static void Info(const std::string& message) { Logger::Log("[INFO]: " + message); }
};
} // namespace DJS
