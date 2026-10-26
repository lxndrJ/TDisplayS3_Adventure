#include <TFT_eSPI.h>

TFT_eSPI tft;

#define PIN_POWER_ON 15
#define PIN_BTN_A    14
#define PIN_BTN_B    0

#define LONG_A_MS    1200
#define LONG_B_MS    700
#define DEBOUNCE_MS  30

#define IT_SEIL      0
#define IT_APFEL     1
#define IT_AMULETT   2
#define IT_SCHLUESSEL 3
#define IT_TROLLZAHN 4
#define IT_KELCH     5
#define IT_AXT       6
#define IT_KRONE     7
#define FL_ELF       16

#define TXT_W        20
#define TXT_LINES    5

enum Ev { EV_NONE, EV_A_SHORT, EV_A_LONG, EV_B_SHORT, EV_B_LONG };

enum {
  SC_TITLE, SC_DORF, SC_WALD, SC_MARKT, SC_AELINDRIL, SC_BERGE, SC_SPALTE,
  SC_UMWEG, SC_BRUECKE, SC_KAMPF_TROLL, SC_TROLL_SCHLAF, SC_TROLL_BESIEGT,
  SC_WACHE, SC_WARTEN, SC_INNENHOF, SC_HOEHLE, SC_SEITEN, SC_GEHEIM,
  SC_KAMPF_DRACHE, SC_SCHLAFLIED, SC_THRON, SC_GAMEOVER, SC_FLUCHT,
  SC_BRUNNEN, SC_RAETSEL, SC_TURM, SC_TURM_INNEN, SC_COUNT
};

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

#define CH_END         {nullptr,-1,0xFF,0xFF,0,0xFF,0xFF,0,0}
#define CH(l,n)        {l,n,0xFF,0xFF,0,0xFF,0xFF,0,0}
#define CH_REQ(l,n,r)  {l,n,r,0xFF,0,0xFF,0xFF,0,0}
#define CH_USE(l,n,i)  {l,n,i,i,0,0xFF,0,0}
#define CH_GIVE(l,n,i) {l,n,0xFF,0xFF,0,i,0xFF,0,0}
#define CH_GIVEF(l,n,i,f) {l,n,0xFF,0xFF,0,i,f,0,0}
#define CH_COST(l,n,c) {l,n,0xFF,0xFF,(int8_t)c,0xFF,0xFF,0,0}
#define CH_BUY(l,i,c,n)   {l,n,0xFF,0xFF,(int8_t)c,i,0xFF,0,0}
#define CH_HURT(l,n,h) {l,n,0xFF,0xFF,0,0xFF,0,(int8_t)h,0}
#define CH_HEAL(l,n,c,h) {l,n,0xFF,0xFF,(int8_t)c,0xFF,0xFF,0,h}

const Scene scenes[SC_COUNT] = {
  {10,0,"DWARVENHALL",
   "Eldoria lies in the mist. The Moon Crown of the Elven Queen was stolen - the dragon Morgath is said to possess it. You are Thrain, son of the smith. Become a hero!",
   {CH("New Game",SC_DORF),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {1,0,"Dwarvenhall",
   "Elder Barthos nods at you. 'The dragon dwells beyond the Elven Wood. Take gold for the journey - and be watchful!'",
   {CH("Into the Elven Wood",SC_WALD),CH("Over the Frost Peaks",SC_BERGE),CH("To the market of Silvershade",SC_MARKT),CH_END},
   nullptr,0,-1,-1,0xFF},

  {2,0,"Elven Wood",
   "Among ancient trees you meet the ranger Aelindril. Her silver eyes flash distrustfully.",
   {CH("Greet her kindly",SC_AELINDRIL),CH("To the wishing well",SC_BRUNNEN),CH("Deeper into the wood",SC_RAETSEL),CH("Back to the village",SC_DORF)},
   nullptr,0,-1,-1,0xFF},

  {3,0,"Silvershade",
   "Merchants praise their wares. The smith hammers a heavy axe. A thick rope lies in the dust.",
   {CH_BUY("Buy rope (5 gold)",IT_SEIL,5,SC_MARKT),CH_BUY("Buy apple (3 gold)",IT_APFEL,3,SC_MARKT),CH_BUY("Battle axe (8 gold)",IT_AXT,8,SC_MARKT),CH("Into the wood",SC_WALD)},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Aelindril",
   "'The troll Grommash guards the Stonebridge,' the elf says softly. 'Take the Queen's amulet - it opens her gate.' She places a silver amulet in your hand.",
   {CH_GIVEF("Accept the amulet",SC_BRUECKE,IT_AMULETT,FL_ELF),CH("Decline",SC_BRUECKE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {5,0,"Frost Peaks",
   "Icy wind whistles over bare peaks. Before you yawns a wide glacier gap; the wind carries snowflakes across.",
   {CH_USE("Use the rope",SC_SPALTE,IT_SEIL),CH("Take the detour",SC_UMWEG),CH("Back",SC_DORF),CH_END},
   nullptr,0,-1,-1,0xFF},

  {6,0,"Glacier Gap",
   "The rope carries you safely across. At the far end, half embedded in ice, an old key glints.",
   {CH_GIVE("Take the key",SC_UMWEG,IT_SCHLUESSEL),CH("Continue",SC_UMWEG),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Snowdrift",
   "The detour through deep snow costs hours. Freezing but unharmed, you reach the wood at the foot of the mountains.",
   {CH("On to the bridge",SC_BRUECKE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Stonebridge",
   "Grommash the troll crouches in the middle of the Stonebridge, swinging his club. 'Toll! Five gold - or a taste of my fist!'",
   {CH("Fight",SC_KAMPF_TROLL),CH_USE("Offer the apple",SC_TROLL_SCHLAF,IT_APFEL),CH_COST("Pay five gold",SC_WACHE,5),CH("Flee",SC_WALD)},
   nullptr,0,-1,-1,0xFF},

  {6,1,"Fight!",
   "Grommash lunges with his club! Roll at least a 4 to hit. A battle axe gives +1 to the roll.",
   {CH_END,CH_END,CH_END,CH_END},
   "Grommash",4,SC_TROLL_BESIEGT,SC_WALD,IT_TROLLZAHN},

  {6,0,"Grommash Sleeps",
   "The troll lies in the grass, snoring loud as a bear. The bridge is free. You sneak past.",
   {CH("Cross the bridge",SC_WACHE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {6,0,"Grommash Defeated",
   "The troll lies stunned in the dust. Between his fingers he still holds a huge tooth.",
   {CH_GIVE("Take the troll tooth",SC_WACHE,IT_TROLLZAHN),CH("Continue",SC_WACHE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Gate of the Elves",
   "Guard Lyrien blocks the way. 'Show the Queen's amulet - or go home to your mountains!'",
   {CH_REQ("Show the amulet",SC_INNENHOF,IT_AMULETT),CH("Beg",SC_WARTEN),CH("Back",SC_BRUECKE),CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Gate of the Elves",
   "Lyrien does not stir. 'No amulet, no entry. Such are the laws of the Queen.'",
   {CH("Back",SC_WACHE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Elven Court",
   "The Queen's tower rises before you. Behind it, between black rocks, the dark entrance to the Dragon Cave.",
   {CH("Into the cave",SC_HOEHLE),CH("Look for a side entrance",SC_SEITEN),CH("Enter the old tower",SC_TURM),CH("Back to the gate",SC_WACHE)},
   nullptr,0,-1,-1,0xFF},

  {9,0,"Dragon Cave",
   "Morgath the dragon lies on a mountain of gold and coins. The Moon Crown gleams on his head. His breath smells of sulfur.",
   {CH("Steal the crown",SC_GEHEIM),CH("Wake the dragon",SC_KAMPF_DRACHE),CH_REQ("Call Aelindril",SC_SCHLAFLIED,FL_ELF),CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Secret Passage",
   "A narrow passage behind waterfalls leads deeper into the mountain. Drops echo like footsteps.",
   {CH("Continue",SC_GEHEIM),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Behind the Crown",
   "Behind the mound of gold you find a heavy oak door with silver fittings. Next to it lies the Moon Crown, only an arm's length away.",
   {CH("Open the door",SC_THRON),CH_GIVE("Take the crown and flee",SC_FLUCHT,IT_KRONE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {9,1,"Fight!",
   "Fire! Heat! Morgath's wings whip through the air. Roll at least a 5 to hit. A battle axe gives +1.",
   {CH_END,CH_END,CH_END,CH_END},
   "Morgath",5,SC_THRON,SC_SEITEN,IT_KRONE},

  {9,0,"Morgath Sleeps",
   "Aelindril's lullaby rocks the dragon into deep sleep. His breath trembles like smoke. The crown lies within reach on his head.",
   {CH_GIVE("Take the crown",SC_THRON,IT_KRONE),CH("Back",SC_HOEHLE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {15,2,"Throne Hall",
   "The Elven Queen accepts the Moon Crown. 'Thrain, Crown Warden of Eldoria!' Trumpets blare. Peace returns.",
   {CH("Restart",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {12,2,"Game Over",
   "Darkness engulfs you. Dwarvenhall mourns its son. The mists of Eldoria remain forever grey.",
   {CH("Try again",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {10,2,"Victory!",
   "With the crown in your pack you escape through the side passage. Aelindril escorts you home. All of Eldoria celebrates you as a hero!",
   {CH("Restart",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {14,0,"Wishing Well",
   "A talking fox sits at a stone well. 'An herb heals for two gold. Five gold bring you a silver chalice - a gift from the elves.'",
   {CH_HEAL("Healing potion (2 gold)",SC_WALD,2,1),CH_BUY("Silver chalice (5 gold)",IT_KELCH,5,SC_WALD),CH("Nothing",SC_WALD),CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Riddle in the Wood",
   "An elf steps from the shadows. 'What has a voice but no body? Answer wrong and it costs you 1 hit point. Right, and I give you a key.'",
   {CH_HURT("The Silence",SC_WALD,1),CH_GIVE("The Echo",SC_WALD,IT_SCHLUESSEL),CH("Say nothing",SC_WALD),CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Old Tower",
   "A weathered tower of grey stone. The heavy portal is locked with a rusty lock.",
   {CH_USE("Use the key",SC_TURM_INNEN,IT_SCHLUESSEL),CH("Onward into the cave",SC_HOEHLE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Tower Chamber",
   "In an old chest lies a silver chalice, decorated with elven runes. Next to it an empty book gathers dust.",
   {CH_GIVE("Take the chalice",SC_INNENHOF,IT_KELCH),CH("Back",SC_INNENHOF),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},
};

const char* itemName(uint8_t i) {
  static const char* n[] = {"Rope","Apple","Amulet","Key","Troll tooth","Silver chalice","Battle axe","Moon Crown"};
  return n[i & 7];
}

int hp = 3;
int gold = 10;
uint32_t flags = 0;
uint16_t cur = SC_TITLE;
uint8_t page = 0;
uint8_t sel = 0;
uint16_t moves = 0;
bool dirty = true;
bool invOpen = false;
uint16_t invReturn = SC_TITLE;

char msg[40] = "";
uint32_t msgUntil = 0;
char rollMsg[40] = "";
static char wrapBuf[10][TXT_W + 1];

struct Btn { uint8_t pin; bool pressed; uint32_t t0; bool longFired; uint32_t deb; };
Btn bA = {PIN_BTN_A, false, 0, false, 0};
Btn bB = {PIN_BTN_B, false, 0, false, 0};

bool hasItem(uint8_t i) { return (flags & (1UL << i)) != 0; }

void flash(const char* m) {
  strncpy(msg, m, sizeof(msg) - 1);
  msg[sizeof(msg) - 1] = 0;
  msgUntil = millis() + 1500;
  dirty = true;
}

int wrapText(const char* t, char out[][TXT_W + 1], int maxL) {
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

bool reqOk(uint8_t r) {
  if (r == 0xFF) return true;
  return (flags & (1UL << r)) != 0;
}

int visibleCount(const Scene& s) {
  int c = 0;
  for (int i = 0; i < 4; i++)
    if (s.ch[i].label && reqOk(s.ch[i].req)) c++;
  return c;
}

const Choice* visibleAt(const Scene& s, int idx) {
  for (int i = 0; i < 4; i++)
    if (s.ch[i].label && reqOk(s.ch[i].req)) {
      if (idx == 0) return &s.ch[i];
      idx--;
    }
  return nullptr;
}

void gotoScene(uint16_t id) {
  cur = id;
  page = 0;
  sel = 0;
  moves++;
  invOpen = false;
  rollMsg[0] = 0;
  dirty = true;
}

void runChoice(const Choice& c) {
  if (c.cost > 0 && gold < c.cost) { flash("Not enough gold!"); return; }
  if (c.cost > 0) gold -= c.cost;
  if (c.use != 0xFF) {
    flags &= ~(1UL << c.use);
    char b[40]; snprintf(b, sizeof(b), "%s used", itemName(c.use));
    flash(b);
  }
  if (c.give != 0xFF) {
    flags |= (1UL << c.give);
    char b[40]; snprintf(b, sizeof(b), "%s received", itemName(c.give));
    flash(b);
  }
  if (c.setFlag != 0xFF) flags |= (1UL << c.setFlag);
  if (c.hurt) {
    hp -= c.hurt;
    flash("-1 hit point");
    if (hp <= 0) { gotoScene(SC_GAMEOVER); return; }
  }
  if (c.heal && hp < 3) { hp += c.heal; if (hp > 3) hp = 3; flash("+1 hit point"); }
  gotoScene((uint16_t)c.next);
}

void fightRoll() {
  const Scene& s = scenes[cur];
  int r = random(1, 7) + (hasItem(IT_AXT) ? 1 : 0);
  bool hit = r >= s.enemyRoll;
  snprintf(rollMsg, sizeof(rollMsg), "Roll %d%s: %s", r, hasItem(IT_AXT) ? " (+1)" : "", hit ? "Hit!" : "Miss!");
  if (hit) {
    if (s.winItem != 0xFF) flags |= (1UL << s.winItem);
    gotoScene((uint16_t)s.winNext);
  } else {
    hp--;
    if (hp <= 0) { gotoScene(SC_GAMEOVER); return; }
    flash("-1 hit point");
    dirty = true;
  }
}

Ev readButtons() {
  uint32_t now = millis();
  Ev ev = EV_NONE;
  bool ra = digitalRead(PIN_BTN_A) == LOW;
  bool rb = digitalRead(PIN_BTN_B) == LOW;

  if (ra && !bA.pressed && now - bA.deb > DEBOUNCE_MS) {
    bA.pressed = true; bA.t0 = now; bA.longFired = false; bA.deb = now;
  }
  if (rb && !bB.pressed && now - bB.deb > DEBOUNCE_MS) {
    bB.pressed = true; bB.t0 = now; bB.longFired = false; bB.deb = now;
  }
  if (ra && bA.pressed && !bA.longFired && now - bA.t0 >= LONG_A_MS) {
    bA.longFired = true; ev = EV_A_LONG;
  }
  if (rb && bB.pressed && !bB.longFired && now - bB.t0 >= LONG_B_MS) {
    bB.longFired = true; ev = EV_B_LONG;
  }
  if (!ra && bA.pressed) {
    bA.pressed = false;
    if (!bA.longFired && ev == EV_NONE) ev = EV_A_SHORT;
  }
  if (!rb && bB.pressed) {
    bB.pressed = false;
    if (!bB.longFired && ev == EV_NONE) ev = EV_B_SHORT;
  }
  return ev;
}

void handleEvent(Ev e) {
  if (e == EV_NONE) return;
  if (invOpen) {
    if (e == EV_A_SHORT || e == EV_B_LONG) { invOpen = false; dirty = true; }
    return;
  }
  const Scene& s = scenes[cur];

  if (s.kind == 1) {
    if (e == EV_A_LONG) { invReturn = cur; invOpen = true; dirty = true; }
    else if (e == EV_A_SHORT || e == EV_B_SHORT) fightRoll();
    else if (e == EV_B_LONG) gotoScene((uint16_t)s.fleeNext);
    return;
  }
  if (s.kind == 2) {
    if (e == EV_B_LONG && s.ch[0].label) runChoice(s.ch[0]);
    else if (e == EV_A_LONG) { invReturn = cur; invOpen = true; dirty = true; }
    return;
  }

  int lines = wrapText(s.text, wrapBuf, 10);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  int nv = visibleCount(s);
  if (sel >= (uint8_t)nv && nv > 0) sel = nv - 1;

  if (e == EV_A_LONG) { invReturn = cur; invOpen = true; dirty = true; return; }

  if (e == EV_A_SHORT) {
    if (page > 0) { page--; dirty = true; }
    else if (nv > 0) { sel = (sel + nv - 1) % nv; dirty = true; }
  } else if (e == EV_B_SHORT) {
    if (page < pages - 1) { page++; dirty = true; }
    else if (nv > 0) { sel = (sel + 1) % nv; dirty = true; }
  } else if (e == EV_B_LONG) {
    if (page < pages - 1) { page++; dirty = true; }
    else {
      const Choice* c = visibleAt(s, sel);
      if (c) runChoice(*c);
    }
  }
}

void drawStatus() {
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("HP", 6, 2);
  for (int i = 0; i < 3; i++)
    tft.fillCircle(24 + i * 12, 6, 4, i < hp ? TFT_RED : TFT_DARKGREY);
  char b[24];
  snprintf(b, sizeof(b), "Gold:%d", gold);
  tft.drawString(b, 58, 2);
  snprintf(b, sizeof(b), "Moves:%d", moves);
  tft.drawString(b, 108, 2);
  tft.drawFastHLine(0, 12, 170, TFT_DARKGREY);
}

void drawPortrait(uint8_t id) {
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

void drawTextPage(const Scene& s) {
  int lines = wrapText(s.text, wrapBuf, 10);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  if (page >= (uint8_t)pages) page = pages - 1;
  int start = page * TXT_LINES;
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  for (int i = 0; i < TXT_LINES; i++) {
    int li = start + i;
    tft.drawString(li < lines ? wrapBuf[li] : "", 6, 106 + i * 17);
  }
  char pg[8];
  snprintf(pg, sizeof(pg), "%d/%d", page + 1, pages);
  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(pg, 148, 96);
  if (msg[0] && millis() < msgUntil) {
    tft.setTextColor(TFT_YELLOW);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(msg, 85, 196);
    tft.setTextDatum(TL_DATUM);
  }
  tft.drawFastHLine(0, 204, 170, TFT_DARKGREY);
}

void drawOptions(const Scene& s) {
  int lines = wrapText(s.text, wrapBuf, 10);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  tft.setTextFont(1);
  if (page < (uint8_t)pages - 1) {
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Next >", 6, 212);
    return;
  }
  int idx = 0;
  for (int c = 0; c < 4; c++) {
    if (!s.ch[c].label || !reqOk(s.ch[c].req)) continue;
    bool srow = (idx == sel);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(srow ? TFT_YELLOW : TFT_LIGHTGREY);
    tft.drawString(srow ? "> " : "  ", 6, 212 + idx * 20);
    tft.drawString(s.ch[c].label, 18, 212 + idx * 20);
    idx++;
  }
}

void drawFooter(uint8_t kind) {
  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
  const char* f = "A: up | B: down | B long: OK";
  if (kind == 1) f = "A/B: Roll | B long: Flee";
  else if (kind == 2) f = "B long: Restart";
  tft.drawString(f, 6, 302);
}

void renderInv() {
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
  snprintf(b, sizeof(b), "Gold: %d   HP: %d/3   Moves: %d", gold, hp, moves);
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString(b, 6, 250);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A short or B long: Back", 6, 302);
}

void render() {
  if (!dirty) return;
  dirty = false;
  if (invOpen) { renderInv(); return; }
  const Scene& s = scenes[cur];
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
    if (rollMsg[0]) {
      tft.setTextColor(TFT_YELLOW);
      tft.drawString(rollMsg, 92, 56);
    }
  }
  drawTextPage(s);
  drawOptions(s);
  if (s.kind == 2) {
    char b[20];
    snprintf(b, sizeof(b), "Moves: %d", moves);
    tft.setTextFont(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(b, 85, 250);
    tft.setTextDatum(TL_DATUM);
  }
  drawFooter(s.kind);
}

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  randomSeed(esp_random());
  hp = 3;
  gold = 10;
  flags = 0;
  moves = 0;
  cur = SC_TITLE;
  page = 0;
  sel = 0;
  dirty = true;
}

void loop() {
  Ev e = readButtons();
  if (e != EV_NONE) handleEvent(e);
  if (msg[0] && millis() > msgUntil) { msg[0] = 0; dirty = true; }
  render();
}
