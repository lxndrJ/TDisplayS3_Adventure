#include "eldoria.h"

#define CH_END         {nullptr,-1,0xFF,0xFF,0,0xFF,0xFF,0,0}
#define CH(l,n)        {l,n,0xFF,0xFF,0,0xFF,0xFF,0,0}
#define CH_REQ(l,n,r)  {l,n,r,0xFF,0,0xFF,0xFF,0,0}
#define CH_USE(l,n,i)  {l,n,i,i,0,0xFF,0xFF,0,0}
#define CH_GIVE(l,n,i) {l,n,0xFF,0xFF,0,i,0xFF,0,0}
#define CH_GIVEF(l,n,i,f) {l,n,0xFF,0xFF,0,i,f,0,0}
#define CH_COST(l,n,c) {l,n,0xFF,0xFF,(int8_t)c,0xFF,0xFF,0,0}
#define CH_BUY(l,i,c,n)   {l,n,0xFF,0xFF,(int8_t)c,i,0xFF,0,0}
#define CH_HURT(l,n,h) {l,n,0xFF,0xFF,0,0xFF,0,(int8_t)h,0}
#define CH_HEAL(l,n,c,h) {l,n,0xFF,0xFF,(int8_t)c,0xFF,0xFF,0,h}

const EldoriaGame::Scene EldoriaGame::kScenes[EldoriaGame::SC_COUNT] = {
  {10,0,"DWARVENHALL",
   "Eldoria lies in mist. The Moon Crown of the Elven Queen was stolen - the dragon Morgath is said to hold it. You are Thrain, son of the smith. Become a hero!",
   {CH("New Game",SC_DORF),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {1,0,"Dwarvenhall",
   "Elder Barthos nods at you. 'The dragon dwells beyond the Elvenwood. Take gold for the journey - and stay alert!'",
   {CH("Into the Elvenwood",SC_WALD),CH("Across the Frostpeak",SC_BERGE),CH("To the market of Silverbrook",SC_MARKT),CH_END},
   nullptr,0,-1,-1,0xFF},

  {2,0,"Elvenwood",
   "Among ancient trees you meet the ranger Aelindril. Her silver eyes gleam with suspicion.",
   {CH("Greet her kindly",SC_AELINDRIL),CH("To the Wishing Well",SC_BRUNNEN),CH("Deeper into the woods",SC_RAETSEL),CH("Back to the village",SC_DORF)},
   nullptr,0,-1,-1,0xFF},

  {3,0,"Silverbrook",
   "Merchants hawk their wares. The smith hammers a heavy axe. In the dust lies a thick rope.",
   {CH_BUY("Buy rope (5 gold)",IT_SEIL,5,SC_MARKT),CH_BUY("Buy apple (3 gold)",IT_APFEL,3,SC_MARKT),CH_BUY("Battle axe (8 gold)",IT_AXT,8,SC_MARKT),CH("Into the woods",SC_WALD)},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Aelindril",
   "'The troll Grommash guards the stone bridge', the elf whispers. 'Take the Queen's amulet - it opens her gate.' She presses a silver amulet into your hand.",
   {CH_GIVEF("Take the amulet",SC_BRUECKE,IT_AMULETT,FL_ELF),CH("Decline",SC_BRUECKE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {5,0,"Frostpeak",
   "Icy wind whistles over bare peaks. Before you yawns a wide glacier gap; the wind carries snowflakes across.",
   {CH_USE("Use the rope",SC_SPALTE,IT_SEIL),CH("Take the detour",SC_UMWEG),CH("Back",SC_DORF),CH_END},
   nullptr,0,-1,-1,0xFF},

  {6,0,"Glacier Gap",
   "The rope carries you safely across. On the far side, half set in ice, an old key glitters.",
   {CH_GIVE("Take the key",SC_UMWEG,IT_SCHLUESSEL),CH("Onward",SC_UMWEG),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Snowdrift",
   "The detour through deep snow costs hours. Chilled but unharmed, you reach the woods at the foot of the mountains.",
   {CH("On to the bridge",SC_BRUECKE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Stonebridge",
   "Grommash the troll squats in the middle of the stone bridge, swinging his club. 'Toll! Five gold - or a taste of my fist!'",
   {CH("Fight",SC_KAMPF_TROLL),CH_USE("Offer the apple",SC_TROLL_SCHLAF,IT_APFEL),CH_COST("Pay five gold",SC_WACHE,5),CH("Flee",SC_WALD)},
   nullptr,0,-1,-1,0xFF},

  {6,1,"Fight!",
   "Grommash lunges with his club! Roll at least a 4 to hit. A battle axe adds +1 to the roll.",
   {CH_END,CH_END,CH_END,CH_END},
   "Grommash",4,SC_TROLL_BESIEGT,SC_WALD,IT_TROLLZAHN},

  {6,0,"Grommash sleeps",
   "The troll lies in the grass, snoring like a bear. The bridge is clear. You sneak past.",
   {CH("Cross the bridge",SC_WACHE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {6,0,"Grommash defeated",
   "The troll lies stunned in the dust. Between his fingers he still holds a huge tooth.",
   {CH_GIVE("Take the troll tooth",SC_WACHE,IT_TROLLZAHN),CH("Onward",SC_WACHE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Elven Gate",
   "Guard Lyrien blocks the way. 'Show the Queen's amulet - or go home to your mountains!'",
   {CH_REQ("Show the amulet",SC_INNENHOF,IT_AMULETT),CH("Beg",SC_WARTEN),CH("Back",SC_BRUECKE),CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Elven Gate",
   "Lyrien does not stir. 'No amulet, no entry. Such are the Queen's laws.'",
   {CH("Back",SC_WACHE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Elven Court",
   "The Queen's tower rises before you. Behind it, between black rocks, the dark entrance to the dragon cave.",
   {CH("Into the cave",SC_HOEHLE),CH("Look for a side entrance",SC_SEITEN),CH("Enter the old tower",SC_TURM),CH("Back to the gate",SC_WACHE)},
   nullptr,0,-1,-1,0xFF},

  {9,0,"Dragon Cave",
   "Morgath the dragon lies on a mountain of gold and coins. The Moon Crown gleams on his head. His breath smells of sulfur.",
   {CH("Steal the crown",SC_GEHEIM),CH("Wake the dragon",SC_KAMPF_DRACHE),CH_REQ("Call Aelindril",SC_SCHLAFLIED,FL_ELF),CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Secret Passage",
   "A narrow passage behind waterfalls leads deeper into the mountain. Droplets echo like footsteps.",
   {CH("Onward",SC_GEHEIM),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Behind the Crown",
   "Behind the gold mound you find a heavy oak door with silver fittings. Beside it lies the Moon Crown, barely an arm's length away.",
   {CH("Open the door",SC_THRON),CH_GIVE("Take the crown and flee",SC_FLUCHT,IT_KRONE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {9,1,"Fight!",
   "Fire! Heat! Morgath's wings whip through the air. Roll at least a 5 to hit. A battle axe adds +1.",
   {CH_END,CH_END,CH_END,CH_END},
   "Morgath",5,SC_THRON,SC_SEITEN,IT_KRONE},

  {9,0,"Morgath sleeps",
   "Aelindril's lullaby weighs the dragon into deep sleep. His breath trembles like smoke. The crown lies within reach on his head.",
   {CH_GIVE("Take the crown",SC_THRON,IT_KRONE),CH("Back",SC_HOEHLE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {15,2,"Throne Hall",
   "The Elven Queen accepts the Moon Crown. 'Thrain, Crown Warden of Eldoria!' Trumpets sound. Peace returns.",
   {CH("Play again",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {12,2,"Game Over",
   "Darkness embraces you. Dwarvenhall mourns its son. The mists of Eldoria stay grey forever.",
   {CH("Try again",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {10,2,"Victory!",
   "With the crown in your pack you escape through the side passage. Aelindril escorts you home. All of Eldoria celebrates you as a hero!",
   {CH("Play again",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {14,0,"Wishing Well",
   "A talking fox sits at a stone well. 'An herb heals for two gold. Five gold bring you a silver chalice - a gift of the elves.'",
   {CH_HEAL("Healing potion (2 gold)",SC_WALD,2,1),CH_BUY("Silver chalice (5 gold)",IT_KELCH,5,SC_WALD),CH("Nothing",SC_WALD),CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Riddle in the Woods",
   "An elf steps from the shadows. 'What has a voice but no body? Answer wrong and it costs you 1 hit point. Answer right and I give you a key.'",
   {CH_HURT("Silence",SC_WALD,1),CH_GIVE("The echo",SC_WALD,IT_SCHLUESSEL),CH("Say nothing",SC_WALD),CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Old Tower",
   "A weathered tower of grey stone. The heavy portal is locked with a rusty lock.",
   {CH_USE("Use the key",SC_TURM_INNEN,IT_SCHLUESSEL),CH("Onward into the cave",SC_HOEHLE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Tower Chamber",
   "In an old chest lies a silver chalice, adorned with elven runes. Beside it, an empty book gathers dust.",
   {CH_GIVE("Take the chalice",SC_INNENHOF,IT_KELCH),CH("Back",SC_INNENHOF),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},
};

const char* EldoriaGame::itemName(uint8_t i) {
  static const char* n[] = {"Rope","Apple","Amulet","Key","Troll tooth","Silver chalice","Battle axe","Moon Crown"};
  return n[i & 7];
}

bool EldoriaGame::reqOk(uint8_t r) const {
  if (r == 0xFF) return true;
  return (flags_ & (1UL << r)) != 0;
}

int EldoriaGame::visibleCount(const Scene& s) const {
  int c = 0;
  for (int i = 0; i < 4; i++)
    if (s.ch[i].label && reqOk(s.ch[i].req)) c++;
  return c;
}

const EldoriaGame::Choice* EldoriaGame::visibleAt(const Scene& s, int idx) const {
  for (int i = 0; i < 4; i++)
    if (s.ch[i].label && reqOk(s.ch[i].req)) {
      if (idx == 0) return &s.ch[i];
      idx--;
    }
  return nullptr;
}

void EldoriaGame::flash(const char* m) {
  strncpy(msg_, m, sizeof(msg_) - 1);
  msg_[sizeof(msg_) - 1] = 0;
  msgUntil_ = millis() + 1500;
  dirty_ = true;
}

int EldoriaGame::wrapText(const char* t, char out[][TXT_W + 1], int maxL) const {
  int n = 0;
  const char* p = t;
  while (*p && n < maxL) {
    const char* start = p;
    int i = 0, lastSpace = -1;
    while (*p && i < TXT_W) {
      if (*p == ' ') lastSpace = i;
      out[n][i++] = *p++;
    }
    if (*p && i == TXT_W && lastSpace > 0) {
      out[n][lastSpace] = 0;
      p = start + lastSpace + 1;
    } else {
      out[n][i] = 0;
    }
    n++;
  }
  return n;
}

void EldoriaGame::gotoScene(uint16_t id) {
  cur_ = id;
  page_ = 0;
  sel_ = 0;
  moves_++;
  invOpen_ = false;
  rollMsg_[0] = 0;
  dirty_ = true;
}

void EldoriaGame::runChoice(const Choice& c) {
  if (c.cost > 0 && gold_ < c.cost) { flash("Not enough gold!"); return; }
  if (c.cost > 0) gold_ -= c.cost;
  if (c.use != 0xFF) {
    flags_ &= ~(1UL << c.use);
    char b[40]; snprintf(b, sizeof(b), "%s used", itemName(c.use));
    flash(b);
  }
  if (c.give != 0xFF) {
    flags_ |= (1UL << c.give);
    char b[40]; snprintf(b, sizeof(b), "%s received", itemName(c.give));
    flash(b);
  }
  if (c.setFlag != 0xFF) flags_ |= (1UL << c.setFlag);
  if (c.hurt) {
    hp_ -= c.hurt;
    flash("-1 hit point");
    if (hp_ <= 0) { gotoScene(SC_GAMEOVER); return; }
  }
  if (c.heal && hp_ < MAX_HP) { hp_ += c.heal; if (hp_ > MAX_HP) hp_ = MAX_HP; flash("+1 hit point"); }
  gotoScene((uint16_t)c.next);
}

void EldoriaGame::fightRoll() {
  const Scene& s = kScenes[cur_];
  int r = random(1, 7) + (hasItem(IT_AXT) ? 1 : 0);
  bool hit = r >= s.enemyRoll;
  snprintf(rollMsg_, sizeof(rollMsg_), "Roll %d%s: %s", r, hasItem(IT_AXT) ? " (+1)" : "", hit ? "Hit!" : "Miss!");
  if (hit) {
    if (s.winItem != 0xFF) flags_ |= (1UL << s.winItem);
    gotoScene((uint16_t)s.winNext);
  } else {
    hp_--;
    if (hp_ <= 0) { gotoScene(SC_GAMEOVER); return; }
    flash("-1 hit point");
    dirty_ = true;
  }
}

void EldoriaGame::handleEvent(Ev e) {
  if (e == EV_NONE) return;
  if (invOpen_) {
    if (e == EV_A_SHORT || e == EV_B_LONG) { invOpen_ = false; dirty_ = true; }
    return;
  }
  const Scene& s = kScenes[cur_];

  if (s.kind == 1) {
    if (e == EV_A_LONG) { invReturn_ = cur_; invOpen_ = true; dirty_ = true; }
    else if (e == EV_A_SHORT || e == EV_B_SHORT) fightRoll();
    else if (e == EV_B_LONG) gotoScene((uint16_t)s.fleeNext);
    return;
  }
  if (s.kind == 2) {
    if (e == EV_B_LONG && s.ch[0].label) runChoice(s.ch[0]);
    else if (e == EV_A_LONG) { invReturn_ = cur_; invOpen_ = true; dirty_ = true; }
    return;
  }

  int lines = wrapText(s.text, wrapBuf_, 10);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  int nv = visibleCount(s);
  if (sel_ >= (uint8_t)nv && nv > 0) sel_ = nv - 1;

  if (e == EV_A_LONG) { invReturn_ = cur_; invOpen_ = true; dirty_ = true; return; }

  if (e == EV_A_SHORT) {
    if (page_ > 0) { page_--; dirty_ = true; }
    else if (nv > 0) { sel_ = (sel_ + nv - 1) % nv; dirty_ = true; }
  } else if (e == EV_B_SHORT) {
    if (page_ < pages - 1) { page_++; dirty_ = true; }
    else if (nv > 0) { sel_ = (sel_ + 1) % nv; dirty_ = true; }
  } else if (e == EV_B_LONG) {
    if (page_ < pages - 1) { page_++; dirty_ = true; }
    else {
      const Choice* c = visibleAt(s, sel_);
      if (c) runChoice(*c);
    }
  }
}

void EldoriaGame::drawStatus() {
  TFT_eSPI& tft = gfx.tft();
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("HP", 6, 2);
  for (int i = 0; i < 3; i++)
    tft.fillCircle(24 + i * 12, 6, 4, i < hp_ ? TFT_RED : TFT_DARKGREY);
  char b[24];
  snprintf(b, sizeof(b), "Gold:%d", gold_);
  tft.drawString(b, 58, 2);
  snprintf(b, sizeof(b), "Moves:%d", moves_);
  tft.drawString(b, 108, 2);
  tft.drawFastHLine(0, 12, gfx.height(), TFT_DARKGREY);
}

void EldoriaGame::drawPortrait(uint8_t id) {
  TFT_eSPI& tft = gfx.tft();
  tft.fillRect(6, 16, 80, 80, TFT_BLACK);
  tft.drawRect(6, 16, 80, 80, TFT_DARKGREY);
  switch (id) {
    case 1:
      tft.fillTriangle(10, 50, 30, 20, 50, 50, TFT_DARKGREY);
      tft.fillTriangle(36, 50, 56, 16, 76, 50, TFT_DARKGREY);
      tft.fillRect(28, 50, 36, 42, TFT_BROWN);
      tft.fillRect(36, 62, 20, 30, TFT_BLACK);
      tft.fillTriangle(24, 50, 46, 34, 68, 50, TFT_RED);
      break;
    case 2:
      for (int i = 0; i < 3; i++) {
        int bx = 20 + i * 20;
        tft.fillRect(bx, 72, 6, 14, TFT_BROWN);
        tft.fillTriangle(bx - 6, 72, bx + 12, 72, bx + 3, 44, TFT_DARKGREEN);
        tft.fillTriangle(bx - 4, 58, bx + 10, 58, bx + 3, 36, TFT_GREEN);
      }
      break;
    case 3:
      tft.fillTriangle(10, 70, 46, 30, 82, 70, TFT_RED);
      tft.drawLine(28, 70, 46, 30, TFT_WHITE);
      tft.drawLine(46, 30, 64, 70, TFT_WHITE);
      tft.fillRect(10, 70, 72, 8, TFT_BROWN);
      break;
    case 4:
      tft.fillTriangle(20, 40, 28, 24, 32, 44, TFT_GREEN);
      tft.fillTriangle(72, 40, 64, 24, 60, 44, TFT_GREEN);
      tft.fillCircle(46, 52, 22, 0xFDB8);
      tft.fillTriangle(24, 44, 46, 20, 68, 44, TFT_GREEN);
      tft.fillCircle(38, 50, 3, TFT_BLACK);
      tft.fillCircle(54, 50, 3, TFT_BLACK);
      tft.drawLine(40, 62, 52, 62, TFT_RED);
      break;
    case 5:
      tft.fillTriangle(8, 80, 34, 36, 60, 80, TFT_DARKGREY);
      tft.fillTriangle(32, 80, 58, 28, 84, 80, TFT_LIGHTGREY);
      tft.fillTriangle(26, 44, 34, 36, 42, 44, TFT_WHITE);
      tft.fillTriangle(50, 36, 58, 28, 66, 36, TFT_WHITE);
      break;
    case 6:
      tft.fillCircle(46, 54, 26, 0x07E0);
      tft.fillCircle(36, 46, 6, TFT_WHITE);
      tft.fillCircle(56, 46, 6, TFT_WHITE);
      tft.fillCircle(36, 46, 3, TFT_BLACK);
      tft.fillCircle(56, 46, 3, TFT_BLACK);
      tft.fillTriangle(38, 66, 44, 66, 41, 74, TFT_WHITE);
      tft.fillTriangle(48, 66, 54, 66, 51, 74, TFT_WHITE);
      tft.fillCircle(46, 57, 5, 0x0400);
      break;
    case 7:
      tft.fillRect(6, 16, 80, 80, TFT_DARKGREY);
      tft.drawRect(6, 16, 80, 80, TFT_DARKGREY);
      tft.fillCircle(46, 96, 44, TFT_BLACK);
      tft.fillCircle(30, 40, 4, TFT_LIGHTGREY);
      tft.fillCircle(62, 34, 5, TFT_LIGHTGREY);
      break;
    case 8:
      tft.fillRect(34, 34, 24, 58, TFT_LIGHTGREY);
      tft.fillTriangle(28, 34, 46, 14, 64, 34, TFT_RED);
      tft.fillRect(42, 48, 8, 10, TFT_BLACK);
      tft.fillRect(42, 68, 8, 24, TFT_BLACK);
      break;
    case 9:
      tft.fillCircle(42, 56, 24, TFT_RED);
      tft.fillCircle(34, 50, 5, TFT_YELLOW);
      tft.fillCircle(50, 50, 5, TFT_YELLOW);
      tft.fillCircle(34, 50, 2, TFT_BLACK);
      tft.fillCircle(50, 50, 2, TFT_BLACK);
      tft.fillTriangle(64, 56, 84, 48, 84, 64, TFT_ORANGE);
      tft.fillTriangle(64, 56, 80, 52, 80, 60, TFT_YELLOW);
      tft.fillTriangle(24, 40, 32, 28, 34, 44, TFT_ORANGE);
      break;
    case 10:
      tft.fillRect(26, 58, 40, 18, TFT_GOLD);
      tft.fillTriangle(26, 58, 34, 58, 30, 40, TFT_GOLD);
      tft.fillTriangle(38, 58, 46, 58, 42, 38, TFT_GOLD);
      tft.fillTriangle(50, 58, 58, 58, 54, 40, TFT_GOLD);
      tft.fillCircle(32, 66, 4, TFT_RED);
      tft.fillCircle(46, 66, 4, TFT_CYAN);
      tft.fillCircle(60, 66, 4, TFT_MAGENTA);
      break;
    case 12:
      tft.fillCircle(46, 50, 24, TFT_WHITE);
      tft.fillRect(34, 66, 24, 14, TFT_WHITE);
      tft.fillCircle(37, 46, 6, TFT_BLACK);
      tft.fillCircle(55, 46, 6, TFT_BLACK);
      tft.fillRect(40, 72, 4, 8, TFT_BLACK);
      tft.fillRect(48, 72, 4, 8, TFT_BLACK);
      tft.fillCircle(46, 58, 4, TFT_BLACK);
      break;
    case 14:
      tft.fillTriangle(28, 34, 36, 20, 40, 38, TFT_ORANGE);
      tft.fillTriangle(64, 34, 56, 20, 52, 38, TFT_ORANGE);
      tft.fillCircle(46, 54, 22, TFT_ORANGE);
      tft.fillCircle(46, 62, 10, TFT_WHITE);
      tft.fillCircle(46, 58, 4, TFT_BLACK);
      tft.fillCircle(38, 50, 3, TFT_BLACK);
      tft.fillCircle(54, 50, 3, TFT_BLACK);
      break;
    case 15:
      tft.fillTriangle(26, 84, 46, 44, 66, 84, TFT_MAGENTA);
      tft.fillCircle(46, 40, 14, 0xFDB8);
      tft.fillRect(38, 26, 16, 8, TFT_GOLD);
      tft.fillTriangle(38, 26, 42, 26, 40, 18, TFT_GOLD);
      tft.fillTriangle(54, 26, 50, 26, 52, 18, TFT_GOLD);
      tft.fillCircle(41, 40, 2, TFT_BLACK);
      tft.fillCircle(51, 40, 2, TFT_BLACK);
      break;
  }
}

void EldoriaGame::drawTextPage(const Scene& s) {
  TFT_eSPI& tft = gfx.tft();
  int lines = wrapText(s.text, wrapBuf_, 10);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  if (page_ >= (uint8_t)pages) page_ = pages - 1;
  int start = page_ * TXT_LINES;
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  for (int i = 0; i < TXT_LINES; i++) {
    int li = start + i;
    tft.drawString(li < lines ? wrapBuf_[li] : "", 6, 106 + i * 17);
  }
  char pg[8];
  snprintf(pg, sizeof(pg), "%d/%d", page_ + 1, pages);
  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(pg, 148, 96);
  if (msg_[0] && millis() < msgUntil_) {
    tft.setTextColor(TFT_YELLOW);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(msg_, 85, 196);
    tft.setTextDatum(TL_DATUM);
  }
  tft.drawFastHLine(0, 204, gfx.height(), TFT_DARKGREY);
}

void EldoriaGame::drawOptions(const Scene& s) {
  TFT_eSPI& tft = gfx.tft();
  int lines = wrapText(s.text, wrapBuf_, 10);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  tft.setTextFont(1);
  if (page_ < (uint8_t)pages - 1) {
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Next >", 6, 212);
    return;
  }
  int idx = 0;
  for (int c = 0; c < 4; c++) {
    if (!s.ch[c].label || !reqOk(s.ch[c].req)) continue;
    bool srow = (idx == sel_);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(srow ? TFT_YELLOW : TFT_LIGHTGREY);
    tft.drawString(srow ? "> " : "  ", 6, 212 + idx * 20);
    tft.drawString(s.ch[c].label, 18, 212 + idx * 20);
    idx++;
  }
}

void EldoriaGame::drawFooter(uint8_t kind) {
  TFT_eSPI& tft = gfx.tft();
  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
  const char* f = "A: up | B: down | B long: OK";
  if (kind == 1) f = "A/B: Roll | B long: Flee";
  else if (kind == 2) f = "B long: Restart";
  tft.drawString(f, 6, 302);
}

void EldoriaGame::renderInv() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("INVENTORY", 6, 8);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  int y = 44;
  bool any = false;
  for (int i = 0; i < 8; i++) {
    if (hasItem(i)) {
      any = true;
      char b[32];
      snprintf(b, sizeof(b), "- %s", itemName(i));
      tft.drawString(b, 12, y);
      y += 20;
    }
  }
  if (!any) tft.drawString("(empty)", 12, y);
  char b[32];
  snprintf(b, sizeof(b), "Gold: %d   HP: %d/3   Moves: %d", gold_, hp_, moves_);
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString(b, 6, 250);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A short or B long: Back", 6, 302);
}

void EldoriaGame::render() {
  if (!dirty_) return;
  dirty_ = false;
  if (invOpen_) { renderInv(); return; }
  TFT_eSPI& tft = gfx.tft();
  const Scene& s = kScenes[cur_];
  tft.fillScreen(TFT_BLACK);
  drawStatus();
  drawPortrait(s.portrait);
  tft.setTextFont(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum(TL_DATUM);
  char t[13];
  strncpy(t, s.title, 12);
  t[12] = 0;
  tft.drawString(t, 92, 18);
  if (s.kind == 1) {
    tft.setTextColor(TFT_WHITE);
    tft.drawString(s.enemy, 92, 40);
    if (rollMsg_[0]) {
      tft.setTextColor(TFT_YELLOW);
      tft.drawString(rollMsg_, 92, 56);
    }
  }
  drawTextPage(s);
  drawOptions(s);
  if (s.kind == 2) {
    char b[20];
    snprintf(b, sizeof(b), "Moves: %d", moves_);
    tft.setTextFont(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(b, 85, 250);
    tft.setTextDatum(TL_DATUM);
  }
  drawFooter(s.kind);
}

void EldoriaGame::init() {
  hp_ = MAX_HP;
  gold_ = 10;
  flags_ = 0;
  moves_ = 0;
  cur_ = SC_TITLE;
  page_ = 0;
  sel_ = 0;
  invOpen_ = false;
  msg_[0] = 0;
  rollMsg_[0] = 0;
  dirty_ = true;
  wantsMenu = false;
}

void EldoriaGame::update(const InputState& in, uint32_t dtMs) {
  (void)dtMs;
  Ev e = EV_NONE;
  if (in.pressed[(uint8_t)Key::A])         e = EV_A_SHORT;
  else if (in.longPressed[(uint8_t)Key::A]) e = EV_A_LONG;
  else if (in.pressed[(uint8_t)Key::B])     e = EV_B_SHORT;
  else if (in.longPressed[(uint8_t)Key::B]) e = EV_B_LONG;
  if (e != EV_NONE) handleEvent(e);
  if (msg_[0] && millis() > msgUntil_) { msg_[0] = 0; dirty_ = true; }
}

void EldoriaGame::draw() {
  render();
}
