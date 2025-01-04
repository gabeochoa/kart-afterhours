
#pragma once

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
