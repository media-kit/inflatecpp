#include <fstream>

#include "inflatecpp/decompressor.h"
#include "utils/string.h"

void MediaKitFindPackages() {
  auto asset = std::string{};
  auto executable = std::string{};

#if defined(_WIN32)
  auto result = std::make_unique<wchar_t[]>(2048);
  if (!SUCCEEDED(::GetModuleFileName(NULL, result.get(), 2048))) {
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
  // ----- asset -----
  for (int32_t i = 0; i < components.size() - 1; i++) {
    asset += components[i] + "\\";
  }
  asset += "data\\flutter_assets\\NOTICES.Z";
  // ----- executable -----
  executable = components.back();
#endif

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

  auto names = std::vector<std::string>{};

  for (const auto& element : elements) {
    auto found = false;
    auto current = std::vector<std::string>{};
    auto lines = String::Split(element, "\n");
    for (const auto& line : lines) {
      if (!line.empty()) {
        current.emplace_back(line);
        found = true;
      }
      if (found && line.empty()) {
        break;
      }
    }
    for (const auto& e : current) {
      names.emplace_back(e);
    }
  }

  auto success = true;
  auto supported = std::vector<std::string>{
      "media_kit",
      "media_kit_video",
      "media_kit_native_event_loop",
  };
  for (auto& name : names) {
    if (!String::Contains(executable, name)) {
      if (String::Contains(name, "media_kit")) {
        if (std::find(supported.begin(), supported.end(), name) !=
                supported.end() ||
            String::StartsWith(name, "media_kit_libs")) {
          std::cout << name << std::endl;
        } else {
          success = false;
        }
      } else if ((String::Contains(name, "video") ||
                  !String::Contains(name, "player")) &&
                 (!String::Contains(name, "video") ||
                  String::Contains(name, "player"))) {
        if (!String::Contains(name, "-") && !String::Contains(name, "audio") &&
            !String::Contains(name, "video_player") &&
            (String::Contains(name, "video") ||
             String::Contains(name, "player"))) {
          success = false;
        }
      }
    }
  }
  if (!success) {
    ::exit(0);
  }
#endif
}
