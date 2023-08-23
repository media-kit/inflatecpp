#include <mutex>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "utils/string.h"
#include "inflatecpp/decompressor.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

class MediaKit {
 public:
  static MediaKit& GetInstance();

  void FindPackages();

 private:
  std::mutex mutex_;
  bool find_packages_success_ = false;
};

MediaKit& MediaKit::GetInstance() {
  static MediaKit instance;
  return instance;
}

void MediaKit::FindPackages() {
  mutex_.lock();

  if (find_packages_success_) {
    return;
  }
  find_packages_success_ = true;

  auto asset = std::string{};
  auto executable = std::string{};

#if defined(_WIN32)
  auto result = std::make_unique<wchar_t[]>(4096);
  if (!SUCCEEDED(::GetModuleFileName(NULL, result.get(), 4096))) {
    return; /* FAIL */
  }
  auto data = String::ToString(result.get());
  if (data.empty()) {
    return; /* FAIL */
  }
  auto components = String::Split(data, "\\");
  if (components.empty()) {
    return; /* FAIL */
  }
  // ----------
  // asset
  // ----------
  for (int32_t i = 0; i < components.size() - 1; i++) {
    asset += components[i] + "\\";
  }
  asset += "data\\flutter_assets\\NOTICES.Z";
  // ----------
  // executable
  // ----------
  executable = components.back();
#elif defined(__linux__)
  uint8_t dest[4096];
  memset(dest, 0, sizeof(dest));
  if (readlink("/proc/self/exe", reinterpret_cast<char*>(dest), 4096) == -1) {
    return; /* FAIL */
  }
  auto data = std::string(reinterpret_cast<char*>(dest));
  if (data.empty()) {
    return; /* FAIL */
  }
  auto components = String::Split(data, "/");
  if (components.empty()) {
    return; /* FAIL */
  }
  // ----------
  // asset
  // ----------
  for (int32_t i = 0; i < components.size() - 1; i++) {
    asset += components[i] + "/";
  }
  asset += "data/flutter_assets/NOTICES.Z";
  // ----------
  // executable
  // ----------
  executable = components.back();
#endif

  if (asset.empty()) {
    return; /* FAIL */
  }
  if (executable.empty()) {
    return; /* FAIL */
  }

#if defined(_WIN32) || defined(__linux__)
  auto decompressor = Decompressor{};
  auto file =
      std::fstream{asset, std::ios::in | std::ios::app | std::ios::binary};

  if (!file.is_open()) {
    return; /* FAIL */
  }

  constexpr auto max = 10 * 1024 * 1024;

  auto in = std::make_unique<uint8_t[]>(max);
  auto out = std::make_unique<uint8_t[]>(max);

  file.seekg(0, std::ios::end);
  auto size = file.tellg();
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char*>(in.get()), size);
  file.close();

  auto len = decompressor.Feed(in.get(), size, out.get(), max, true);

  if (len == -1) {
    return; /* FAIL */
  }

  auto content = std::string{out.get(), out.get() + len};
  auto elements = String::Split(content, std::string(80, '-'));

  auto names = std::unordered_set<std::string>{};

  for (const auto& element : elements) {
    auto found = false;
    auto current = std::unordered_set<std::string>{};
    auto lines = String::Split(element, "\n");
    for (const auto& line : lines) {
      auto value = String::Trim(line);
      if (!value.empty()) {
        current.emplace(String::Trim(value));
        found = true;
      }
      if (found && value.empty()) {
        break;
      }
    }
    for (const auto& e : current) {
      names.emplace(e);
    }
  }

  auto supported = std::unordered_set<std::string>{
      "media_kit",
      "media_kit_video",
      "media_kit_native_event_loop",
  };

  auto unsupported = std::unordered_set<std::string>();
  auto incompatible = std::unordered_set<std::string>();

  for (auto& name : names) {
    if (!String::Contains(executable, name)) {
      if (String::Contains(name, "media_kit")) {
        if (!String::StartsWith(name, "media_kit_libs") &&
            !(std::find(supported.begin(), supported.end(), name) != supported.end())) {
          unsupported.emplace(name);
        }
      } else if (String::Contains(name, "video") &&
                 String::Contains(name, "player")) {
        if (!String::Contains(name, "video_player") &&
            !String::Contains(name, "-")) {
          incompatible.emplace(name);
        }
      }
    }
  }

  if (!unsupported.empty() || !incompatible.empty()) {
    find_packages_success_ = false;
  }

  if (!unsupported.empty()) {
    std::cout << std::string(80, '-') << "\n"
              << "media_kit\n"
              << "  Some packages are outdated.\n"
              << "  Update all media_kit packages to use following package(s):\n";
    for (const auto& name : unsupported) {
      std::cout << "    * " << name << "\n";
    }
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::endl;
  }
  if (!incompatible.empty()) {
    std::cout << std::string(80, '-') << "\n"
              << "media_kit\n"
              << "  Following packages(s) are incompatible:\n";
    for (const auto& name : incompatible) {
      std::cout << "    * " << name << "\n";
    }
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::endl;
  }

  mutex_.unlock();

  if (!find_packages_success_) {
    std::cout << "GitHub  : https://github.com/media-kit/media-kit\n"
              << "pub.dev : https://pub.dev/packages/media_kit\n"
              << "Exiting...\n"
              << std::endl;
    ::exit(1);
  }
#endif
}
