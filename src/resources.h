
#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "rl.h"

inline const char *GetAssetsDirectory() {
  static char assetDir[1024] = {0};

  // 1. Check environment variable
  const char *assetsEnv = getenv("RESOURCES");
  if (assetsEnv != NULL) {
    strcpy(assetDir, assetsEnv);

    // Ensure trailing slash.
    if (assetDir[strlen(assetDir) - 1] != '/') {
      strcat(assetDir, "/");
    }
    return assetDir;
  }

  // 2. Check working directory
  char wd[1024] = {0};
  strcpy(wd, raylib::GetWorkingDirectory());
  char *d = strcat(wd, "/resources/");
  if (raylib::DirectoryExists(d)) {
    strcpy(assetDir, d);
    return assetDir;
  }

  // 3. Seek out git root
  char searchDir[1024] = {0};
  strcpy(searchDir, raylib::GetWorkingDirectory());
  for (int i = 0; i < 10; i++) {
    char gitDir[1024] = {0};
    strcpy(gitDir, searchDir);
    strcat(gitDir, "/.git");
    if (raylib::DirectoryExists(gitDir)) {
      strcpy(assetDir, searchDir);
      return strcat(assetDir, "/resources/");
    }
    strcpy(searchDir, raylib::GetPrevDirectoryPath(searchDir));
  }

  return "/Users/gabeochoa/p/kart-afterhours/resources/";
}
// Use this path to load the asset immediately, once called again original value
// is replaced!
inline const char *GetAssetPath(const char *filename) {
  static char path[1024] = {0};
  strcpy(path, GetAssetsDirectory());
  strcat(path, filename);
  std::cout << "Loading asset: " << path << std::endl;
  return path;
}

namespace fs = std::filesystem;

struct FilesConfig {
  std::string_view root_folder;
  std::string_view settings_file_name;
};

SINGLETON_FWD(Files)
struct Files {
  SINGLETON(Files)

  std::string root = "cart_chaos";
  std::string settings_file = "settings.bin";

  [[nodiscard]] fs::path game_folder() const;
  bool ensure_game_folder_exists();
  [[nodiscard]] fs::path settings_filepath() const;
  [[nodiscard]] fs::path resource_folder() const;
  [[nodiscard]] fs::path game_controller_db() const;
  [[nodiscard]] std::string fetch_resource_path(std::string_view group,
                                                std::string_view name) const;
  void for_resources_in_group(
      std::string_view group,
      // TODO replace with struct?
      const std::function<void(std::string, std::string, std::string)> &) const;
  void for_resources_in_folder(
      std::string_view group, std::string_view folder,
      const std::function<void(std::string, std::string)> &) const;

  // TODO add a full cleanup to write folders in case we need to reset

  void folder_locations() const;

private:
  Files();
};
