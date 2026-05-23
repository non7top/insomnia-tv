// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SYSTEM_FS_FILESYSTEMMANAGER_H_
#define SRC_SYSTEM_FS_FILESYSTEMMANAGER_H_

#include <string>

#include "../../hal/IFileSystem.h"

#if defined(ARDUINO)
#include <FS.h>
#include <LittleFS.h>
#endif

namespace InsomniaTV {

class FileSystemManager : public IFileSystem {
public:
  FileSystemManager();
  bool mount() override;
  std::string readJson(const std::string& path) override;
  bool writeJson(const std::string& path, const std::string& json) override;
  bool uploadFile(const std::string& path, const uint8_t* data,
                  size_t len) override;
  int32_t downloadFile(const std::string& path, uint8_t* outBuf,
                       size_t bufSize) override;
  bool exists(const std::string& path) const override;
  bool remove(const std::string& path) override;

private:
#if defined(ARDUINO)
  fs::FS* _fs;
#endif
};

}  // namespace InsomniaTV

#endif  // SRC_SYSTEM_FS_FILESYSTEMMANAGER_H_
