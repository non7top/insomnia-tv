// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_WEB_WEBSERVER_H_
#define SRC_WEB_WEBSERVER_H_

#include <cstdint>
#if defined(ARDUINO)
#include <ESPAsyncWebServer.h>
#endif

#include "../config/ConfigManager.h"
#include "../discovery/SamsungTvDiscovery.h"
#include "../tv/TvStateMachine.h"

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
   * @param configMgr Reference to the ConfigManager.
   */
  WebServer(uint16_t port, SamsungTvDiscovery& discovery,
            ConfigManager& configMgr);

  /**
   * @brief Starts the web server.
   */
  void begin();

  /**
   * @brief Wire in the TvStateMachine so /api/tv-config can read/update it.
   * Called from setup() after the state machine is created.
   */
  void setTvStateMachine(TvStateMachine* tvSm);

private:
#if defined(ARDUINO)
  AsyncWebServer _server;
#endif
  SamsungTvDiscovery& _discovery;
  ConfigManager& _configMgr;
  TvStateMachine* _tvSm = nullptr;
  void setupRoutes();
};

}  // namespace InsomniaTV

#endif  // SRC_WEB_WEBSERVER_H_
