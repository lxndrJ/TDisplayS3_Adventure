#ifndef CORE_NET_H
#define CORE_NET_H

#include <Arduino.h>

// Non-blocking WiFi + Mistral AI client for LLM-driven games.
// The game loop must never wait: poll() only checks flags, the actual
// HTTPS round-trip runs in a dedicated FreeRTOS task (TLS needs a big
// stack, so the task owns its own HTTPClient and deletes itself).
//
// Credentials come from include/secrets.h (gitignored). If the file or
// the defines are missing, the client reports NoConfig and games fall
// back to offline content.
//
// Usage:
//   net.begin();                      // once, starts WiFi connect
//   net.request(systemPrompt, userPrompt);
//   // every frame:
//   NetClient::State st = net.poll();
//   if (st == NetClient::State::Done)  use net.response();
//   if (st == NetClient::State::Error) read net.error();

class NetClient {
public:
  enum class State : uint8_t {
    Idle,           // connected (or not started), ready for a request
    NoConfig,       // secrets.h missing / empty credentials
    WifiConnecting, // WiFi association in progress
    WifiFailed,     // could not join the network
    Busy,           // HTTPS request running in the background task
    Done,           // net.response() holds the assistant text
    Error           // net.error() describes the failure
  };

  static constexpr size_t MAX_RESPONSE = 2048;

  // Starts WiFi (async). Call once at boot or game init.
  void begin();

  // true if WiFi credentials are configured (secrets.h present).
  bool configured() const;

  // true if WiFi is currently connected.
  bool connected() const;

  // WiFi signal strength in dBm, or -100 when not connected.
  int rssi() const;

  // Queue a chat completion. systemPrompt steers the model's behavior,
  // userPrompt carries the current game state. Only one request at a
  // time; returns false when busy or not ready.
  bool request(const char* systemPrompt, const char* userPrompt);

  // Advance the state machine. Call every frame.
  State poll();

  // Assistant text of the last completed request (empty otherwise).
  const char* response() const { return response_; }

  // Short error description when poll() returned Error.
  const char* error() const { return error_; }

  // Back to Idle (e.g. after Done/Error, before the next request).
  void reset();

private:
  void setError(const char* msg);

  volatile State state_     = State::Idle;
  uint32_t       wifiStart_ = 0;
  char           response_[MAX_RESPONSE] = "";
  char           error_[64]  = "";
};

extern NetClient net;

#endif
