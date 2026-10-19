// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"

#include <ArduinoJson.h>

#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>

#include "../config/SystemCredentials.h"
#include "../version.h"
#include "UpdateCompletion.h"

#if defined(ARDUINO)
#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>

#include "ui_page.h"  // NOLINT(build/include_subdir): generated header
#endif

#include "../sensors/SensorManager.h"

namespace InsomniaTV {

#if defined(ARDUINO)
AsyncEventSource events("/events");

namespace {
class ArduinoRebootSequencer : public IRebootSequencer {
public:
  explicit ArduinoRebootSequencer(AsyncWebServerRequest* request)
      : _request(request) {}

  void sendResponse(bool ok) override {
    AsyncWebServerResponse* res =
        _request->beginResponse(200, "text/plain", ok ? "OK" : "FAIL");
    res->addHeader("Connection", "close");
    _request->send(res);
  }

  void delayMs(uint32_t ms) override { delay(ms); }
  void restart() override { ESP.restart(); }

private:
  AsyncWebServerRequest* _request;
};
}  // namespace

WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery,
                     ConfigManager& configMgr)
    : _server(port), _discovery(discovery), _configMgr(configMgr) {
  setupRoutes();
}

void WebServer::setTvStateMachine(TvStateMachine* tvSm) {
  _tvSm = tvSm;
}

void WebServer::setSleepStateMachine(SleepStateMachine* sleepSm) {
  _sleepSm = sleepSm;
}

void WebServer::setActivityTracker(ActivityTracker* tracker) {
  _activityTracker = tracker;
}

void WebServer::setSamsungTvController(SamsungTvController* controller) {
  _samsungTv = controller;
}

void WebServer::begin() {
  _server.addHandler(&events);
  _server.begin();

  // Periodic sensor status broadcaster -- reads the cache SensorManager's
  // own poll task maintains, doesn't read hardware itself (#75).
  xTaskCreate(
      [](void* pvParameters) {
        for (;;) {
          auto sensors = SensorManager::instance().listSensors();
          for (auto const& s : sensors) {
            bool val = SensorManager::instance().getCachedValue(s->getId());
            JsonDocument doc;
            doc["id"] = s->getId();
            doc["value"] = val;
            doc["available"] = s->isAvailable();
            String buf;
            serializeJson(doc, buf);
            events.send(buf.c_str(), "sensor_update", millis());
          }
          vTaskDelay(pdMS_TO_TICKS(5000));
        }
      },
      "sensor_poll", 4096, NULL, 1, NULL);
}

void WebServer::setupRoutes() {
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    // kIndexHtml is assembled from src/web/ui/ by scripts/embed_ui.py.
    AsyncWebServerResponse* res =
        request->beginResponse(200, "text/html", kIndexHtml);
    res->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    request->send(res);
  });

  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress().c_str();
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["dhcp"] = WiFi.getMode() == WIFI_STA ? "enabled" : "disabled";
    doc["ap"] = WiFi.softAPSSID();
    doc["version"] = INSOMNIATV_VERSION;
    doc["chipId"] = ESP.getEfuseMac();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["wifiStatus"] =
        WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    if (_sleepSm != nullptr) {
      const char* sleepStates[] = {"MONITORING", "RAMPING", "VERIFYING",
                                   "POWERING_OFF", "FALLBACK_OFF"};
      int ss = static_cast<int>(_sleepSm->getCurrentState());
      doc["sleepState"] = sleepStates[ss];
      doc["msSinceSleepReset"] = _sleepSm->msSinceReset();
    }
    if (_activityTracker != nullptr) {
      doc["msSinceLastIrActivity"] = _activityTracker->msSinceLastActivity();
    }

    String response;
    serializeJson(doc, response);
    AsyncWebServerResponse* res =
        request->beginResponse(200, "application/json", response);
    res->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    request->send(res);
  });

  _server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    auto sensors = SensorManager::instance().listSensors();
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (auto const& sensor : sensors) {
      JsonObject obj = array.add<JsonObject>();
      obj["id"] = sensor->getId();
      obj["type"] = sensor->getType();
      obj["available"] = sensor->isAvailable();
      obj["device_id"] = sensor->getDeviceId();
      obj["value"] = SensorManager::instance().getCachedValue(sensor->getId());
      JsonDocument cfg = sensor->getConfig();
      for (JsonPair kv : cfg.as<JsonObject>()) {
        const char* k = kv.key().c_str();
        if (strcmp(k, "id") != 0 && strcmp(k, "type") != 0)
          obj[k] = kv.value();
      }
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  _server.on(
      "/api/sensors", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        if (!request->authenticate(kSystemUsername, kSystemPassword)) {
          return request->requestAuthentication();
        }
      },
      NULL,
      [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
             size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);

        // Save to configuration
        Config cfg = _configMgr.get();
        JsonDocument sensorDoc;
        deserializeJson(sensorDoc, cfg.sensorsJson);

        // Check if it already exists to avoid duplicates
        bool found = false;
        for (JsonObject s : sensorDoc.as<JsonArray>()) {
          if (s["id"] == doc["id"]) {
            s.set(doc.as<JsonObject>());
            found = true;
            break;
          }
        }
        if (!found)
          sensorDoc.add(doc);

        serializeJson(sensorDoc, cfg.sensorsJson);
        _configMgr.set(cfg);
        _configMgr.save();

        // Re-register immediately
        SensorManager::instance().init(cfg.sensorsJson, _discovery);

        request->send(200, "application/json", "{\"status\":\"ok\"}");
      });

  _server.on("/api/sensors", HTTP_DELETE,
             [this](AsyncWebServerRequest* request) {
               if (!request->authenticate(kSystemUsername, kSystemPassword)) {
                 return request->requestAuthentication();
               }
               if (request->hasParam("id")) {
                 String id = request->getParam("id")->value();

                 // Remove from configuration
                 Config cfg = _configMgr.get();
                 JsonDocument sensorDoc;
                 deserializeJson(sensorDoc, cfg.sensorsJson);

                 JsonArray array = sensorDoc.as<JsonArray>();
                 for (size_t i = 0; i < array.size(); i++) {
                   if (array[i]["id"] == id) {
                     array.remove(i);
                     break;
                   }
                 }

                 serializeJson(sensorDoc, cfg.sensorsJson);
                 _configMgr.set(cfg);
                 _configMgr.save();

                 // Re-init so the running SensorManager reflects the updated
                 // config.
                 SensorManager::instance().init(cfg.sensorsJson, _discovery);

                 request->send(200, "application/json", "{\"status\":\"ok\"}");
                 return;
               }
               request->send(400);
             });

  _server.on(
      "/api/sensors/test", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->authenticate(kSystemUsername, kSystemPassword)) {
          return request->requestAuthentication();
        }
        if (request->hasParam("id", true)) {
          String id = request->getParam("id", true)->value();
          auto sensor = SensorManager::instance().getSensor(id.c_str());
          if (sensor) {
            bool result = sensor->read();
            String response = "{\"id\":\"" + id +
                              "\", \"value\":" + (result ? "true" : "false") +
                              "}";
            request->send(200, "application/json", response);
            return;
          }
        }
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
      });

  _server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        bool shouldReboot = !Update.hasError();
        ArduinoRebootSequencer sequencer(request);
        handleUpdateCompletion(shouldReboot, sequencer);
      },
      [](AsyncWebServerRequest* request, String filename, size_t index,
         uint8_t* data, size_t len, bool final) {
        if (!index) {
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        }
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
        if (final) {
          if (!Update.end(true)) {
            Update.printError(Serial);
          }
        }
      });

  // One-shot sensor probe without registering it in SensorManager.
  // Uses AsyncCallbackJsonWebHandler so the body is fully accumulated before
  // our handler is called — avoids the deferred-send race with _send().
  _server.on("/api/probe", HTTP_POST,
             [this](AsyncWebServerRequest* request, JsonVariant& json) {
               if (!request->authenticate(kSystemUsername, kSystemPassword)) {
                 return request->requestAuthentication();
               }
               std::string type = json["type"] | "";
               bool available = false;
               int latencyMs = -1;

               if (type == "ping") {
                 std::string target = json["target_ip"] | "";
                 if (!target.empty()) {
                   uint32_t t = millis();
                   available = Ping.ping(target.c_str(), 1);
                   latencyMs = static_cast<int>(millis() - t);
                 }
               } else if (type == "http") {
                 std::string url = json["url"] | "";
                 if (!url.empty()) {
                   HTTPClient http;
                   http.begin(url.c_str());
                   http.setTimeout(3000);
                   uint32_t t = millis();
                   int code = http.GET();
                   latencyMs = static_cast<int>(millis() - t);
                   available = (code > 0 && code < 500);
                   http.end();
                 }
               } else if (type == "upnp") {
                 std::string target = json["target_name"] | "";
                 for (auto& tv : _discovery.getDiscoveredTvs()) {
                   if (tv.name == target) {
                     available = true;
                     break;
                   }
                 }
                 latencyMs = 0;
               }

               char buf[80];
               snprintf(buf, sizeof(buf),
                        "{\"available\":%s,\"latency_ms\":%d}",
                        available ? "true" : "false", latencyMs);
               request->send(200, "application/json", buf);
             });

  _server.on("/api/scan", HTTP_POST, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    xTaskCreate(
        [](void* pvParameters) {
          auto* self = static_cast<WebServer*>(pvParameters);
          self->_discovery.clear();
          self->_discovery.scan();
          vTaskDelete(NULL);
        },
        "discovery_task", 4096, this, 1, NULL);

    request->send(200, "application/json", "{\"status\":\"scanning\"}");
  });

  _server.on("/api/discovery", HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               if (!request->authenticate(kSystemUsername, kSystemPassword)) {
                 return request->requestAuthentication();
               }
               JsonDocument doc;
               JsonArray array = doc.to<JsonArray>();
               for (const auto& tv : _discovery.getDiscoveredTvs()) {
                 JsonObject obj = array.add<JsonObject>();
                 obj["name"] = tv.name;
                 obj["model"] = tv.model;
                 obj["ip"] = tv.ip;
               }
               String response;
               serializeJson(doc, response);
               request->send(200, "application/json", response);
             });

  // GET /api/tv-config — live sensor contributions and power state
  _server.on(
      "/api/tv-config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!request->authenticate(kSystemUsername, kSystemPassword)) {
          return request->requestAuthentication();
        }
        JsonDocument doc;
        if (_tvSm != nullptr) {
          const char* states[] = {"UNKNOWN", "ON", "OFF", "TRANSITIONING"};
          int ps = static_cast<int>(_tvSm->getPowerState());
          doc["power_state"] = states[ps];
          JsonArray contribs = doc["contributions"].to<JsonArray>();
          for (const auto& c : _tvSm->getContributions()) {
            JsonObject o = contribs.add<JsonObject>();
            o["sensor_id"] = c.sensorId;
            o["enabled"] = c.enabled;
            o["available"] = c.available;
            o["vote"] = c.weightedVote;
          }
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
      });

  // POST /api/reboot — restarts after the response has been flushed
  _server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    xTaskCreate(
        [](void*) {
          vTaskDelay(pdMS_TO_TICKS(1000));
          ESP.restart();
        },
        "reboot_task", 2048, NULL, 1, NULL);
    request->send(202, "application/json", "{\"status\":\"rebooting\"}");
  });

  // GET /api/tv/control — Samsung pairing state for the UI
  _server.on(
      "/api/tv/control", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!request->authenticate(kSystemUsername, kSystemPassword)) {
          return request->requestAuthentication();
        }
        JsonDocument doc;
        if (_samsungTv != nullptr) {
          const char* states[] = {"idle", "waiting", "paired", "failed"};
          doc["host"] = _samsungTv->host();
          doc["mac"] = _samsungTv->mac();
          doc["paired"] = _samsungTv->isPaired();
          doc["pair_state"] = states[static_cast<int>(_samsungTv->pairState())];
          doc["error"] = _samsungTv->lastError();
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
      });

  // POST /api/tv/pair {"host":"<ip>"} — starts pairing; the TV shows an
  // allow prompt. Poll GET /api/tv/control for the outcome.
  _server.on("/api/tv/pair", HTTP_POST,
             [this](AsyncWebServerRequest* request, JsonVariant& json) {
               if (!request->authenticate(kSystemUsername, kSystemPassword)) {
                 return request->requestAuthentication();
               }
               std::string host = json["host"] | "";
               if (_samsungTv == nullptr || host.empty()) {
                 request->send(400, "application/json",
                               "{\"error\":\"host required\"}");
                 return;
               }
               if (_samsungTv->pairState() == PairState::Waiting) {
                 request->send(409, "application/json",
                               "{\"error\":\"pairing already in progress\"}");
                 return;
               }
               struct PairJob {
                 SamsungTvController* tv;
                 std::string host;
               };
               auto* job = new PairJob{_samsungTv, host};
               xTaskCreate(
                   [](void* pvParameters) {
                     auto* j = static_cast<PairJob*>(pvParameters);
                     j->tv->pair(j->host);
                     delete j;
                     vTaskDelete(NULL);
                   },
                   "tv_pair", 10240, job, 1, NULL);
               request->send(202, "application/json",
                             "{\"status\":\"pairing\"}");
             });

  // POST /api/tv/power-off — manual test of the network power-off
  _server.on(
      "/api/tv/power-off", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!request->authenticate(kSystemUsername, kSystemPassword)) {
          return request->requestAuthentication();
        }
        if (_samsungTv == nullptr || !_samsungTv->isPaired()) {
          request->send(409, "application/json",
                        "{\"error\":\"TV not paired\"}");
          return;
        }
        xTaskCreate(
            [](void* pvParameters) {
              static_cast<SamsungTvController*>(pvParameters)->powerOff();
              vTaskDelete(NULL);
            },
            "tv_poweroff", 10240, _samsungTv, 1, NULL);
        request->send(202, "application/json", "{\"status\":\"sent\"}");
      });

  // GET /api/behavior — the sleep-sequence settings the Config tab edits
  _server.on("/api/behavior", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    const Config& cfg = _configMgr.get();
    JsonDocument doc;
    doc["inactivity_timeout_min"] = cfg.inactivityTimeoutMin;
    doc["ramp_interval_min"] = cfg.rampIntervalMin;
    doc["max_ramp_steps_before_poweroff"] = cfg.maxRampStepsBeforePoweroff;
    doc["typical_volume"] = cfg.typicalVolume;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/behavior — any subset of the fields above; rejected as a whole
  // if one is out of range.
  _server.on("/api/behavior", HTTP_POST,
             [this](AsyncWebServerRequest* request, JsonVariant& json) {
               if (!request->authenticate(kSystemUsername, kSystemPassword)) {
                 return request->requestAuthentication();
               }
               // Clamped into the field's type so validate() rejects an
               // out-of-range value instead of the read silently falling back
               // to the old one.
               auto field = [&json](const char* key, uint32_t current,
                                    int32_t maxValue) {
                 int32_t v = json[key] | static_cast<int32_t>(current);
                 return static_cast<uint32_t>(
                     std::min<int32_t>(std::max<int32_t>(v, 0), maxValue));
               };
               Config cfg = _configMgr.get();
               cfg.inactivityTimeoutMin = field(
                   "inactivity_timeout_min", cfg.inactivityTimeoutMin, 100000);
               cfg.rampIntervalMin =
                   field("ramp_interval_min", cfg.rampIntervalMin, 100000);
               cfg.maxRampStepsBeforePoweroff = static_cast<uint8_t>(
                   field("max_ramp_steps_before_poweroff",
                         cfg.maxRampStepsBeforePoweroff, 255));
               cfg.typicalVolume = static_cast<uint8_t>(
                   field("typical_volume", cfg.typicalVolume, 255));
               std::string error;
               if (!ConfigManager::validate(cfg, error)) {
                 JsonDocument doc;
                 doc["error"] = error;
                 String response;
                 serializeJson(doc, response);
                 request->send(400, "application/json", response);
                 return;
               }
               _configMgr.set(cfg);
               _configMgr.save();
               request->send(200, "application/json", "{\"status\":\"ok\"}");
             });

  // GET /api/files/download?path=<path>
  // NOTE: specific sub-routes must be registered before GET /api/files because
  // ESPAsyncWebServer matches by prefix — /api/files would capture /api/files/*
  // if registered first.
  _server.on("/api/files/download", HTTP_GET,
             [](AsyncWebServerRequest* request) {
               if (!request->authenticate(kSystemUsername, kSystemPassword)) {
                 return request->requestAuthentication();
               }
               if (!request->hasParam("path")) {
                 request->send(400);
                 return;
               }
               String path = request->getParam("path")->value();
               if (!LittleFS.exists(path)) {
                 request->send(404);
                 return;
               }
               request->send(LittleFS, path, "application/octet-stream", true);
             });

  // GET /api/files/edit?path=<path> — return file contents as plain text
  _server.on("/api/files/edit", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    if (!request->hasParam("path")) {
      request->send(400);
      return;
    }
    String path = request->getParam("path")->value();
    if (!LittleFS.exists(path)) {
      request->send(404);
      return;
    }
    File f = LittleFS.open(path, "r");
    if (!f) {
      request->send(500);
      return;
    }
    if (f.size() > 65536) {
      f.close();
      request->send(413, "text/plain", "File too large to edit");
      return;
    }
    String content = f.readString();
    f.close();
    request->send(200, "text/plain", content);
  });

  // POST /api/files/edit — atomic save; hot-reloads config if applicable
  _server.on(
      "/api/files/edit", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        if (!request->authenticate(kSystemUsername, kSystemPassword)) {
          return request->requestAuthentication();
        }
      },
      NULL,
      [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
             size_t index, size_t total) {
        if (index == 0) {
          _editBodyBuffer = "";
          _editBodyBuffer.reserve(total);
        }
        _editBodyBuffer.concat(reinterpret_cast<const char*>(data), len);
        if (index + len != total) {
          return;  // wait for the remaining chunks
        }

        JsonDocument doc;
        deserializeJson(doc, _editBodyBuffer);
        String path = doc["path"] | "";
        String content = doc["content"] | "";
        if (path.isEmpty()) {
          request->send(400);
          return;
        }
        if (path == ConfigManager::kConfigPath) {
          // Reject non-JSON content outright rather than writing it and
          // silently corrupting the live config on the next load() (#67).
          JsonDocument probe;
          if (deserializeJson(probe, content)) {
            request->send(400, "text/plain", "Invalid config JSON");
            return;
          }
        }
        String tmp = path + ".tmp";
        File f = LittleFS.open(tmp, "w");
        if (!f) {
          Serial.printf("[WebServer] edit: failed to open %s for writing\n",
                        tmp.c_str());
          request->send(500);
          return;
        }
        f.print(content);
        f.close();
        LittleFS.remove(path);
        if (!LittleFS.rename(tmp, path)) {
          Serial.printf("[WebServer] edit: rename failed: %s -> %s\n",
                        tmp.c_str(), path.c_str());
          request->send(500);
          return;
        }
        if (path == ConfigManager::kConfigPath) {
          _configMgr.load();
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      });

  // GET /api/files — recursive LittleFS listing (must come after /api/files/*)
  _server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    std::function<void(const String&)> walk;
    walk = [&walk, &array](const String& dir) {
      File d = LittleFS.open(dir);
      if (!d || !d.isDirectory()) {
        Serial.printf("[WebServer] files: failed to open dir: %s\n",
                      dir.c_str());
        return;
      }
      File f = d.openNextFile();
      while (f) {
        String p = (dir == "/") ? String("/") + f.name() : dir + "/" + f.name();
        if (f.isDirectory()) {
          walk(p);
        } else {
          JsonObject o = array.add<JsonObject>();
          o["name"] = p;
          o["size"] = static_cast<uint32_t>(f.size());
        }
        f = d.openNextFile();
      }
    };
    walk("/");
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // DELETE /api/files?path=<path>
  _server.on("/api/files", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(kSystemUsername, kSystemPassword)) {
      return request->requestAuthentication();
    }
    if (!request->hasParam("path")) {
      request->send(400);
      return;
    }
    String path = request->getParam("path")->value();
    if (LittleFS.remove(path)) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      Serial.printf("[WebServer] delete: LittleFS.remove failed: %s\n",
                    path.c_str());
      request->send(404);
    }
  });

  // POST /api/files/upload?path=<path> — atomic multipart upload
  _server.on(
      "/api/files/upload", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      },
      [](AsyncWebServerRequest* request, String filename, size_t index,
         uint8_t* data, size_t len, bool final) {
        static File uploadFile;
        static String uploadPath;
        if (index == 0) {
          uploadPath = request->hasParam("path")
                           ? request->getParam("path")->value()
                           : "/" + filename;
          String tmp = uploadPath + ".tmp";
          LittleFS.remove(tmp);
          uploadFile = LittleFS.open(tmp, "w");
          if (!uploadFile) {
            Serial.printf("[WebServer] upload: failed to open %s for writing\n",
                          tmp.c_str());
          }
        }
        if (uploadFile) {
          uploadFile.write(data, len);
        }
        if (final) {
          if (uploadFile) {
            uploadFile.close();
            String tmp = uploadPath + ".tmp";
            LittleFS.remove(uploadPath);
            if (!LittleFS.rename(tmp, uploadPath)) {
              Serial.printf("[WebServer] upload: rename failed: %s -> %s\n",
                            tmp.c_str(), uploadPath.c_str());
            }
          } else {
            Serial.printf(
                "[WebServer] upload: no file handle at final for %s\n",
                uploadPath.c_str());
          }
        }
      });
}
#else
WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery,
                     ConfigManager& configMgr)
    : _discovery(discovery), _configMgr(configMgr) {}
void WebServer::begin() {}
void WebServer::setTvStateMachine(TvStateMachine* tvSm) {
  _tvSm = tvSm;
}
void WebServer::setSleepStateMachine(SleepStateMachine* sleepSm) {
  _sleepSm = sleepSm;
}
void WebServer::setActivityTracker(ActivityTracker* tracker) {
  _activityTracker = tracker;
}
void WebServer::setSamsungTvController(SamsungTvController* controller) {
  _samsungTv = controller;
}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
