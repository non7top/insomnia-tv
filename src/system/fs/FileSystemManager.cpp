// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "FileSystemManager.h"

#include <Arduino.h>

namespace InsomniaTV {

FileSystemManager::FileSystemManager() : _fs(&LittleFS) {}

bool FileSystemManager::mount() {
#if defined(ARDUINO)
  return LittleFS.begin(true);
#else
  return false;
#endif
}

std::string FileSystemManager::readJson(const std::string& path) {
#if defined(ARDUINO)
  File file = _fs->open(path.c_str(), "r");
  if (!file)
    return "";
  String content = file.readString();
  file.close();
  return content.c_str();
#else
  return "";
#endif
}

bool FileSystemManager::writeJson(const std::string& path,
                                  const std::string& json) {
#if defined(ARDUINO)
  File file = _fs->open(path.c_str(), "w");
  if (!file)
    return false;
  file.print(json.c_str());
  file.close();
  return true;
#else
  return false;
#endif
}

bool FileSystemManager::uploadFile(const std::string& path, const uint8_t* data,
                                   size_t len) {
#if defined(ARDUINO)
  File file = _fs->open(path.c_str(), "w");
  if (!file)
    return false;
  file.write(data, len);
  file.close();
  return true;
#else
  return false;
#endif
}

int32_t FileSystemManager::downloadFile(const std::string& path,
                                        uint8_t* outBuf, size_t bufSize) {
#if defined(ARDUINO)
  File file = _fs->open(path.c_str(), "r");
  if (!file)
    return -1;
  size_t len = file.read(outBuf, bufSize);
  file.close();
  return static_cast<int32_t>(len);
#else
  return -1;
#endif
}

bool FileSystemManager::exists(const std::string& path) const {
#if defined(ARDUINO)
  return _fs->exists(path.c_str());
#else
  return false;
#endif
}

bool FileSystemManager::remove(const std::string& path) {
#if defined(ARDUINO)
  return _fs->remove(path.c_str());
#else
  return false;
#endif
}

}  // namespace InsomniaTV
