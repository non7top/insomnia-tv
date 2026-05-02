// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_DISCOVERY_SAMSUNGTVDISCOVERY_H_
#define SRC_DISCOVERY_SAMSUNGTVDISCOVERY_H_

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace InsomniaTV {

/**
 * @brief Represents metadata for a discovered Samsung TV.
 */
struct SamsungTvInfo {
  std::string name;      ///< Friendly name of the TV
  std::string model;     ///< Model number
  std::string ip;        ///< Local IP address
  std::string location;  ///< UPnP device description URL
};

/**
 * @brief Handles SSDP/UPnP discovery of Samsung "AllShare1.0" TV units.
 */
class SamsungTvDiscovery {
public:
  SamsungTvDiscovery();

  /**
   * @brief Starts an asynchronous scan for TVs on the network.
   */
  void scan();

  /**
   * @brief Checks if a scan is currently in progress.
   */
  bool isScanning() const;

  /**
   * @brief Gets the list of discovered TVs.
   * @return Vector of SamsungTvInfo.
   */
  std::vector<SamsungTvInfo> getDiscoveredTvs() const;

  /**
   * @brief Clears the list of discovered TVs.
   */
  void clear();

protected:
  /**
   * @brief Parses an SSDP raw response and extracts the LOCATION header.
   * @param response The raw UDP packet content.
   */
  void parseSsdpResponse(const std::string& response);

  /**
   * @brief Fetches and parses the device description XML from the TV.
   * @param tv Reference to the TV info struct to populate.
   */
  virtual void fetchDeviceMetadata(SamsungTvInfo& tv);

private:
  std::vector<SamsungTvInfo> _discoveredTvs;
  mutable std::mutex _mutex;
  bool _isScanning;
};

}  // namespace InsomniaTV

#endif  // SRC_DISCOVERY_SAMSUNGTVDISCOVERY_H_
