#ifndef GAMES_NEONSHARD_NEONSHARD_H
#define GAMES_NEONSHARD_NEONSHARD_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"
#include "art.h"

// NEON SHARD - cyberpunk point-and-click in landscape mode (320x170).
// Comic story: courier Vex finds a data shard with a free AI and must
// decide what freedom costs.
// Controls: A short = previous target, B short = next target,
// B long = action/continue, A long = (in dialog) fast-forward.
class NeonShardGame : public Game {
public:
  const char* name() const override { return "Neon Shard"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;

  enum Phase : uint8_t { PH_TITLE, PH_SCENE, PH_DIALOG, PH_END };

  enum : uint8_t {
    SC_ALLEY, SC_BAR, SC_SUBWAY, SC_ROOF, SC_ENDA, SC_ENDB,
    SC_COUNT
  };
  enum : uint8_t {
    ART_TITLE, ART_ALLEY, ART_BAR, ART_SUBWAY, ART_ROOF,
    ART_ENDA, ART_ENDB, ART_COUNT
  };

  // Hotspot IDs (unique per scene, meaning in interact())
  enum : uint8_t {
    H_NONE,
    H_START,
    H_ALLEY_DRONE, H_ALLEY_RIKO, H_ALLEY_BAR, H_ALLEY_SUBWAY,
    H_BAR_MAMA,    H_BAR_STREET, H_BAR_SUBWAY,
    H_SUB_CHEN,    H_SUB_STREET, H_SUB_ROOF,
    H_ROOF_ANTENNA, H_ROOF_SUBWAY,
    H_END_EXIT
  };

  // Click zone in the landscape image (320x170)
  struct Hotspot {
    uint8_t     id;
    int16_t     x, y, w, h;
    const char* label;
  };

  struct SceneDef {
    uint8_t   art;
    uint8_t   nHot;
    Hotspot   hot[4];
  };

  static const SceneDef& scene(uint8_t id) { return kScenes[id]; }

  // story state
  bool hasShard_  = false;
  bool metMama_   = false;
  bool tipMama_   = false;
  bool metChen_   = false;

  void startStory();
  void gotoScene(uint8_t sc);
  void decodeArt(uint8_t art);
  void interact(uint8_t hs);
  void dialogEnd(uint8_t dlgId, uint8_t choice);
  // Dialog: lines (text; "Name: " prefix is colored) and optional
  // choices at the end. dialogEnd() branches the story.
  void beginDialog(uint8_t dlgId, const char* const* lines, uint8_t nLines,
                   const char* const* choices, uint8_t nChoices);
  void drawScene(TFT_eSprite& fb);
  void drawDialog(TFT_eSprite& fb);
  void drawTitle(TFT_eSprite& fb);
  void drawEnd(TFT_eSprite& fb);

  // dialog instances
  enum : uint8_t {
    DLG_DRONE, DLG_DRONE2, DLG_RIKO, DLG_RIKO2,
    DLG_MAMA_GATE, DLG_MAMA, DLG_CHEN_GATE, DLG_CHEN,
    DLG_ANTENNA_GATE, DLG_ANTENNA, DLG_NO_TOWER,
    DLG_FLAVOR = 0xFF   // text only, no branching at the end
  };

  Phase     phase_   = PH_TITLE;
  uint8_t   scene_   = SC_ALLEY;
  uint8_t   sel_     = 0;      // selected hotspot
  // dialog state
  uint8_t   dlgId_        = 0;
  const char* const* dlgLines_     = nullptr;
  uint8_t   dlgLineCount_ = 0;
  uint8_t   dlgIdx_       = 0;   // current line
  const char* const* dlgChoices_   = nullptr;
  uint8_t   dlgChoiceCount_ = 0;
  uint8_t   dlgChoice_    = 0;

  // Framebuffer (landscape) and decoder buffer for the scene art
  TFT_eSprite fb_{&gfx.tft()};
  uint16_t* bg_ = nullptr;     // 320x170, PSRAM preferred

  static const SceneDef kScenes[SC_COUNT];
};

#endif
