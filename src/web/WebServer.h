// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_WEB_WEBSERVER_H_
#define SRC_WEB_WEBSERVER_H_

#include <ESPAsyncWebServer.h>

namespace InsomniaTV {

/**
 * @brief Handles web interface and API routes for insomniaTV.
 */
class WebServer {
public:
  WebServer(uint16_t port);
  void begin();

private:
  AsyncWebServer _server;
  void setupRoutes();
};

}  // namespace InsomniaTV

#endif  // SRC_WEB_WEBSERVER_H_
