// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_WEB_WEBSERVER_H_
#define SRC_WEB_WEBSERVER_H_

#include <cstdint>
#if defined(ARDUINO)
#include <ESPAsyncWebServer.h>
#endif

namespace InsomniaTV {

/**
 * @brief Handles web interface and API routes for insomniaTV.
 *
 * Provides a RESTful API for monitoring system state and controlling basic
 * insomniaTV settings via JSON.
 */
class WebServer {
public:
    /**
     * @brief Constructs the WebServer on the specified port.
     * @param port The server port (e.g., 80).
     */
    WebServer(uint16_t port);

    /**
     * @brief Starts the web server.
     */
    void begin();
private:
  #if defined(ARDUINO)
      AsyncWebServer _server;
  #endif
      void setupRoutes();
  };
}  // namespace InsomniaTV

#endif  // SRC_WEB_WEBSERVER_H_
