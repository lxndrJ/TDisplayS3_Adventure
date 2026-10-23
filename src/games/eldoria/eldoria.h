#ifndef GAMES_ELDORIA_H
#define GAMES_ELDORIA_H

#include "core/game.h"
#include "core/display.h"

class EldoriaGame : public Game {
public:
  const char* name() const override { return "Eldoria"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;

  enum Ev { EV_NONE, EV_A_SHORT, EV_A_LONG, EV_B_SHORT, EV_B_LONG };

  enum {
    SC_TITLE, SC_DORF, SC_WALD, SC_MARKT, SC_AELINDRIL, SC_BERGE, SC_SPALTE,
    SC_UMWEG, SC_BRUECKE, SC_KAMPF_TROLL, SC_TROLL_SCHLAF, SC_TROLL_BESIEGT,
    SC_WACHE, SC_WARTEN, SC_INNENHOF, SC_HOEHLE, SC_SEITEN, SC_GEHEIM,
    SC_KAMPF_DRACHE, SC_SCHLAFLIED, SC_THRON, SC_GAMEOVER, SC_FLUCHT,
    SC_BRUNNEN, SC_RAETSEL, SC_TURM, SC_TURM_INNEN, SC_COUNT
  };

  enum {
    IT_SEIL, IT_APFEL, IT_AMULETT, IT_SCHLUESSEL,
    IT_TROLLZAHN, IT_KELCH, IT_AXT, IT_KRONE
  };

  static constexpr uint8_t FL_ELF    = 16;
  static constexpr uint8_t TXT_W     = 20;
  static constexpr uint8_t TXT_LINES = 5;
  static constexpr uint8_t MAX_HP    = 3;

  struct Choice {
    const char* label;
    int16_t next;
    uint8_t req;
    uint8_t use;
    int8_t  cost;
    uint8_t give;
    uint8_t setFlag;
    int8_t  hurt;
    uint8_t heal;
  };

  struct Scene {
    uint8_t portrait;
    uint8_t kind;
    const char* title;
    const char* text;
    Choice ch[4];
    const char* enemy;
    uint8_t enemyRoll;
    int16_t winNext;
    int16_t fleeNext;
    uint8_t winItem;
  };

  static const Scene& scene(uint8_t id) { return kScenes[id]; }
  static const char* itemName(uint8_t i);
  bool hasItem(uint8_t i) const { return (flags_ & (1UL << i)) != 0; }
  bool reqOk(uint8_t r) const;
  int  visibleCount(const Scene& s) const;
  const Choice* visibleAt(const Scene& s, int idx) const;

  void flash(const char* m);
  int  wrapText(const char* t, char out[][TXT_W + 1], int maxL) const;
  void gotoScene(uint16_t id);
  void runChoice(const Choice& c);
  void fightRoll();
  void handleEvent(Ev e);

  void drawStatus();
  void drawPortrait(uint8_t id);
  void drawTextPage(const Scene& s);
  void drawOptions(const Scene& s);
  void drawFooter(uint8_t kind);
  void renderInv();
  void render();

  int      hp_    = MAX_HP;
  int      gold_  = 10;
  uint32_t flags_ = 0;
  uint16_t cur_   = SC_TITLE;
  uint8_t  page_  = 0;
  uint8_t  sel_   = 0;
  uint16_t moves_ = 0;
  bool     dirty_     = true;
  bool     invOpen_   = false;
  uint16_t invReturn_ = SC_TITLE;

  char     msg_[40]     = "";
  uint32_t msgUntil_    = 0;
  char     rollMsg_[40] = "";
  char     wrapBuf_[10][TXT_W + 1];

  static const Scene kScenes[SC_COUNT];
};

#endif
