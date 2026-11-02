#include "net.h"

#if __has_include("../../include/secrets.h")
  #include "../../include/secrets.h"
  #define NET_HAS_SECRETS 1
#else
  #define NET_HAS_SECRETS 0
#endif

#if NET_HAS_SECRETS
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>
#endif

NetClient net;

// WiFi connect timeout. The HTTPS task enforces its own timeout via
// HTTPClient::setTimeout, so a stalled request cannot wedge the game.
static constexpr uint32_t WIFI_TIMEOUT_MS = 15000;
static constexpr uint32_t HTTP_TIMEOUT_MS = 20000;
static constexpr uint32_t TASK_STACK      = 12288;  // TLS needs a lot

#if NET_HAS_SECRETS

// The HTTPS round-trip runs here, off the game loop. It fills the
// shared response/error buffers under a mutex and then exits.
struct NetJob {
  String   body;
  String   response;
  char     error[64];
  SemaphoreHandle_t done;
};

static NetJob* g_job = nullptr;

static void netTask(void* arg) {
  NetJob* job = static_cast<NetJob*>(arg);

  WiFiClientSecure client;
  client.setInsecure();  // no CA bundle on the device; acceptable for a game
  client.setHandshakeTimeout(HTTP_TIMEOUT_MS / 1000);
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setReuse(false);

  if (!http.begin(client, "https://api.mistral.ai/v1/chat/completions")) {
    snprintf(job->error, sizeof(job->error), "http begin failed");
  } else {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + MISTRAL_API_KEY);
    int code = http.POST(job->body);
    if (code == HTTPC_ERROR_CONNECTION_FAILED || code == -1) {
      // distinguish DNS / TCP / TLS so the title screen tells the cause
      IPAddress ip;
      if (WiFi.hostByName("api.mistral.ai", ip) != 1)
        snprintf(job->error, sizeof(job->error), "dns failed");
      else if (!client.connect("api.mistral.ai", 443))
        snprintf(job->error, sizeof(job->error), "tcp/tls failed");
      else
        snprintf(job->error, sizeof(job->error), "http -1 (conn)");
    } else if (code != HTTP_CODE_OK) {
      snprintf(job->error, sizeof(job->error), "http %d", code);
    } else {
      String raw = http.getString();
      JsonDocument doc;
      if (deserializeJson(doc, raw) != DeserializationError::Ok) {
        snprintf(job->error, sizeof(job->error), "bad json");
      } else {
        const char* content = doc["choices"][0]["message"]["content"] | "";
        job->response = content;
        job->response.trim();
      }
    }
    http.end();
  }

  // Hand the finished job over to poll() via the semaphore. poll()
  // copies the result and deletes the job; the task must NOT free it
  // itself (it used to, and the game loop then polled a freed/null
  // g_job while state_ stayed Busy forever - "the storyteller thinks").
  xSemaphoreGive(job->done);
  vTaskDelete(nullptr);
}

#endif  // NET_HAS_SECRETS

void NetClient::begin() {
#if NET_HAS_SECRETS
  if (!configured()) { state_ = State::NoConfig; return; }
  // Re-startable: also called after a WifiFailed timeout or after the
  // game was quit and re-entered. WiFi.begin() is idempotent while
  // associating and restarts the association after a disconnect.
  if (WiFi.status() == WL_CONNECTED) { state_ = State::Idle; return; }
  // Full restart on every begin() while not connected: a plain
  // WiFi.begin() after a disconnect/timeout often never re-associates
  // on the S3, so disconnect first and start clean.
  WiFi.disconnect(true);   // true = erase SDK config, force re-assoc
  delay(50);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // power save off: it drops the link on this board
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiStart_ = millis();
  state_ = State::WifiConnecting;
#else
  state_ = State::NoConfig;
#endif
}

bool NetClient::configured() const {
#if NET_HAS_SECRETS
  return strlen(WIFI_SSID) > 0 && strlen(WIFI_PASSWORD) > 0
      && strlen(MISTRAL_API_KEY) > 0;
#else
  return false;
#endif
}

bool NetClient::connected() const {
#if NET_HAS_SECRETS
  return WiFi.status() == WL_CONNECTED;
#else
  return false;
#endif
}

int NetClient::rssi() const {
#if NET_HAS_SECRETS
  if (WiFi.status() != WL_CONNECTED) return -100;
  return WiFi.RSSI();
#else
  return -100;
#endif
}

bool NetClient::request(const char* systemPrompt, const char* userPrompt) {
#if NET_HAS_SECRETS
  if (state_ == State::Busy) return false;
  if (!configured()) { setError("no credentials"); return false; }
  if (!connected())  { setError("wifi offline");  return false; }
  if (g_job)         { setError("busy");          return false; }

  JsonDocument doc;
  doc["model"] = MISTRAL_MODEL;
  doc["max_tokens"] = 300;
  doc["temperature"] = 0.8;
  JsonArray msgs = doc["messages"].to<JsonArray>();
  JsonObject sys = msgs.add<JsonObject>();
  sys["role"] = "system";
  sys["content"] = systemPrompt;
  JsonObject usr = msgs.add<JsonObject>();
  usr["role"] = "user";
  usr["content"] = userPrompt;

  NetJob* job = new NetJob();
  serializeJson(doc, job->body);
  job->done = xSemaphoreCreateBinary();

  g_job = job;
  uint32_t freeHeap = esp_get_free_heap_size();
  if (freeHeap < TASK_STACK + 24 * 1024) {  // TLS also needs heap at runtime
    vSemaphoreDelete(job->done);
    delete job;
    g_job = nullptr;
    snprintf(error_, sizeof(error_), "low heap (%u B free)", freeHeap);
    state_ = State::Error;
    return false;
  }
  if (xTaskCreate(netTask, "net", TASK_STACK, job, 1, nullptr) != pdPASS) {
    vSemaphoreDelete(job->done);
    delete job;
    g_job = nullptr;
    snprintf(error_, sizeof(error_), "task create failed (heap %u)", freeHeap);
    state_ = State::Error;
    return false;
  }
  state_ = State::Busy;
  return true;
#else
  (void)systemPrompt;
  (void)userPrompt;
  setError("secrets.h missing");
  return false;
#endif
}

NetClient::State NetClient::poll() {
#if NET_HAS_SECRETS
  // Reap a finished background job exactly once. The semaphore take
  // doubles as the ownership transfer: after it succeeds, the task is
  // gone and only poll() touches the job.
  if (g_job && xSemaphoreTake(g_job->done, 0) == pdTRUE) {
    if (state_ == State::Busy) {
      if (g_job->error[0]) {
        setError(g_job->error);
      } else {
        strncpy(response_, g_job->response.c_str(), MAX_RESPONSE - 1);
        response_[MAX_RESPONSE - 1] = 0;
        state_ = State::Done;
      }
    }
    // Stale jobs (e.g. a request that finished after reset()) are
    // simply discarded; their result was never requested again.
    SemaphoreHandle_t done = g_job->done;
    delete g_job;
    g_job = nullptr;
    vSemaphoreDelete(done);
  }

  switch (state_) {
    case State::WifiConnecting:
      if (WiFi.status() == WL_CONNECTED) {
        state_ = State::Idle;
        // TLS certificate checks (and some mbedTLS paths even with
        // setInsecure) need a plausible clock: sync via NTP once.
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      } else if (millis() - wifiStart_ > WIFI_TIMEOUT_MS) {
        WiFi.disconnect();
        setError("wifi timeout");
        state_ = State::WifiFailed;
      }
      break;

    case State::WifiFailed:
      // Allow begin() to retry the association after a failure instead
      // of staying stuck in WifiFailed forever.
      break;

    default:
      break;
  }
#endif
  return state_;
}

void NetClient::setError(const char* msg) {
  strncpy(error_, msg, sizeof(error_) - 1);
  error_[sizeof(error_) - 1] = 0;
  state_ = State::Error;
}

void NetClient::reset() {
  response_[0] = 0;
  error_[0] = 0;
  if (state_ == State::Busy) return;  // job still running; poll() reaps it
  state_ = State::Idle;
}
