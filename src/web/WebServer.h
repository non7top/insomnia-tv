// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_WEB_WEBSERVER_H_
#define SRC_WEB_WEBSERVER_H_

#include <cstdint>
#if defined(ARDUINO)
#include <ESPAsyncWebServer.h>
#endif

#include "../discovery/SamsungTvDiscovery.h"

namespace InsomniaTV {

/**
 * @brief Handles web interface and API routes for insomniaTV.
 */
class WebServer {
public:
  /**
   * @brief Constructs the WebServer.
   * @param port The server port.
   * @param discovery Reference to the TV discovery service.
   */
  WebServer(uint16_t port, SamsungTvDiscovery& discovery);

  /**
   * @brief Starts the web server.
   */
  void begin();

private:
#if defined(ARDUINO)
  AsyncWebServer _server;
#endif
  SamsungTvDiscovery& _discovery;
  void setupRoutes();
};

}  // namespace InsomniaTV

#endif  // SRC_WEB_WEBSERVER_H_
