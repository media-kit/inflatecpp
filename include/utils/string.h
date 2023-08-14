#include <string>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#endif

class String {
 public:
  static std::vector<std::string> Split(std::string s, std::string delimiter) {
    size_t start = 0, end, size = delimiter.length();
    auto token = std::string{};
    auto result = std::vector<std::string>{};
    while ((end = s.find(delimiter, start)) != std::string::npos) {
      token = s.substr(start, end - start);
      start = end + size;
      result.push_back(token);
    }
    result.push_back(s.substr(start));
    return result;
  }

  static bool Contains(std::string s, std::string key) {
    return s.find(key) != std::string::npos;
  }

  static bool StartsWith(std::string s, std::string key) {
    return s.rfind(key, 0) == 0;
  }

#if defined(_WIN32)
  static std::string ToString(std::wstring data) {
    if (data.empty()) {
      return std::string();
    }
    auto size = ::WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, data.c_str(),
        static_cast<int>(data.length()), nullptr, 0, nullptr, nullptr);
    if (size == 0) {
      return std::string{};
    }
    auto result = std::string{};
    result.resize(size);
    auto converted_length = ::WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, data.c_str(),
        static_cast<int>(data.length()), result.data(), size, nullptr, nullptr);
    if (converted_length == 0) {
      return std::string{};
    }
    return result;
  }
#endif
};
