#ifndef GLOBALS_
#define GLOBALS_

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace core {

inline constexpr uint32_t kWindowWidth = 800;
inline constexpr uint32_t kWindowHeight = 600;
inline constexpr std::string_view kAppName = "MightyEngine";
const std::vector<char const*> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> kDeviceExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName};

#ifdef MORE_LOGS
inline constexpr bool kMoreDebugging = true;
#else
inline constexpr bool kMoreDebugging = false;
#endif
#ifdef NDEBUG
inline constexpr bool kEnableValidationLayers = false;
#else
inline constexpr bool kEnableValidationLayers = true;
#endif

enum LogType { INFO, ERROR };
#define LOG(type) LogStream(type, __FILE__, __LINE__)

inline const char* LogTypeToString(LogType type) {
    switch (type) {
        case INFO:
            return "INFO";
        case ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

class LogStream {
   public:
    LogStream(LogType type, const char* file, int line)
        : type_(type), file_(file), line_(line) {}

    ~LogStream() {
        std::ostream& out = (type_ == ERROR) ? std::cerr : std::cout;
        std::filesystem::path p(file_);
        out << "[" << LogTypeToString(type_) << "] "
            << "[" << p.filename().string() << ":" << line_ << "] "
            << stream_.str();
        out.flush();
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

   private:
    LogType type_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};
}  // namespace core

#endif  // GLOBALS_