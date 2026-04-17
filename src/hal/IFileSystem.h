// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_IFILESYSTEM_H_
#define SRC_HAL_IFILESYSTEM_H_

#include <Arduino.h>

namespace InsomniaTV {

// Abstract interface for filesystem operations (FAT32/LittleFS).
// Enables config persistence, IR code storage, and web file upload.
class IFileSystem {
public:
  virtual ~IFileSystem() = default;

  // Mount the filesystem, returns true on success
  virtual bool mount() = 0;

  // Read a file and parse as JSON string (caller frees)
  virtual String readJson(const String& path) = 0;

  // Write a JSON string to file, returns true on success
  virtual bool writeJson(const String& path, const String& json) = 0;

  // Upload a file from web form, returns true on success
  virtual bool uploadFile(const String& path, const uint8_t* data,
                          size_t len) = 0;

  // Download a file to byte buffer, returns length or -1 on error
  virtual int32_t downloadFile(const String& path, uint8_t* outBuf,
                               size_t bufSize) = 0;

  // Check if a file exists
  virtual bool exists(const String& path) const = 0;

  // Remove a file, returns true on success
  virtual bool remove(const String& path) = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_IFILESYSTEM_H_
