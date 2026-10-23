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
  {10,0,"ZWERGENHALL",
   "Eldoria liegt in Nebel. Die Mondkrone der Elfenkoenigin wurde gestohlen - der Drache Morgath soll sie besitzen. Du bist Thrain, Sohn des Schmieds. Werde zum Helden!",
   {CH("Neues Spiel",SC_DORF),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {1,0,"Zwergenhall",
   "Der Aelteste Barthos nickt dir zu. 'Der Drache haust jenseits des Elfenwaldes. Nimm Gold fuer die Reise - und sei wachsam!'",
   {CH("In den Elfenwald",SC_WALD),CH("Ueber die Frostberge",SC_BERGE),CH("Zum Markt von Silberhain",SC_MARKT),CH_END},
   nullptr,0,-1,-1,0xFF},

  {2,0,"Elfenwald",
   "Zwischen uralten Baeumen begegnet dir die Waldlaeuferin Aelindril. Ihre silbernen Augen funkeln misstrauisch.",
   {CH("Freundlich begruessen",SC_AELINDRIL),CH("Zum Wunschbrunnen",SC_BRUNNEN),CH("Tiefer in den Wald",SC_RAETSEL),CH("Zurueck zum Dorf",SC_DORF)},
   nullptr,0,-1,-1,0xFF},

  {3,0,"Silberhain",
   "Haendler preisen ihre Waren an. Der Schmied haemmert eine schwere Axt. Im Staub liegt ein dickes Seil.",
   {CH_BUY("Seil kaufen (5 Gold)",IT_SEIL,5,SC_MARKT),CH_BUY("Apfel kaufen (3 Gold)",IT_APFEL,3,SC_MARKT),CH_BUY("Kampfaxt (8 Gold)",IT_AXT,8,SC_MARKT),CH("In den Wald",SC_WALD)},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Aelindril",
   "'Der Troll Grommash bewacht die Steinbruecke', sagt die Elfin leise. 'Nimm das Amulett der Koenigin - es oeffnet ihr Tor.' Sie legt ein silbernes Amulett in deine Hand.",
   {CH_GIVEF("Amulett annehmen",SC_BRUECKE,IT_AMULETT,FL_ELF),CH("Ablehnen",SC_BRUECKE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {5,0,"Frostberge",
   "Eisiger Wind pfeift ueber kahle Gipfel. Vor dir gaehnt eine breite Gletscherspalte, der Wind traegt Schneeflocken herueber.",
   {CH_USE("Seil benutzen",SC_SPALTE,IT_SEIL),CH("Umweg nehmen",SC_UMWEG),CH("Zurueck",SC_DORF),CH_END},
   nullptr,0,-1,-1,0xFF},

  {6,0,"Gletscherspalte",
   "Das Seil traegt dich sicher hinueber. Am anderen Ende, halb im Eis eingebettet, glitzert ein alter Schluessel.",
   {CH_GIVE("Schluessel nehmen",SC_UMWEG,IT_SCHLUESSEL),CH("Weiter",SC_UMWEG),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Schneetreiben",
   "Der Umweg durch den tiefen Schnee kostet Stunden. Durchgefroren, aber unversehrt, erreichst du den Wald am Fuss der Berge.",
   {CH("Weiter zur Bruecke",SC_BRUECKE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Steinbruecke",
   "Grommash der Troll hockt mitten auf der Steinbruecke und schwingt seine Keule. 'Zoll! Fuenf Gold - oder Kostprobe meiner Faust!'",
   {CH("Kaempfen",SC_KAMPF_TROLL),CH_USE("Apfel anbieten",SC_TROLL_SCHLAF,IT_APFEL),CH_COST("Fuenf Gold zahlen",SC_WACHE,5),CH("Fliehen",SC_WALD)},
   nullptr,0,-1,-1,0xFF},

  {6,1,"Kampf!",
   "Grommash stoesst mit seiner Keule zu! Wuerfle mindestens eine 4 zum Treffen. Eine Kampfaxt gibt +1 auf den Wurf.",
   {CH_END,CH_END,CH_END,CH_END},
   "Grommash",4,SC_TROLL_BESIEGT,SC_WALD,IT_TROLLZAHN},

  {6,0,"Grommash schlaeft",
   "Der Troll liegt im Gras und schnarcht laut wie ein Baer. Die Bruecke ist frei. Du schleichst vorbei.",
   {CH("Bruecke ueberqueren",SC_WACHE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {6,0,"Grommash besiegt",
   "Der Troll liegt betaeubt im Staub. Zwischen seinen Fingern haelt er noch immer einen riesigen Zahn.",
   {CH_GIVE("Trollzahn nehmen",SC_WACHE,IT_TROLLZAHN),CH("Weiter",SC_WACHE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Tor der Elfen",
   "Wache Lyrien versperrt den Weg. 'Zeigt das Amulett der Koenigin - oder kehrt heim in eure Berge!'",
   {CH_REQ("Amulett zeigen",SC_INNENHOF,IT_AMULETT),CH("Betteln",SC_WARTEN),CH("Zurueck",SC_BRUECKE),CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Tor der Elfen",
   "Lyrien ruehrt sich nicht. 'Ohne Amulett kein Einlass. So sind die Gesetze der Koenigin.'",
   {CH("Zurueck",SC_WACHE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Elfenhain",
   "Der Turm der Koenigin ragt vor dir auf. Dahinter, zwischen schwarzen Felsen, der dunkle Eingang zur Drachenhoehle.",
   {CH("In die Hoehle",SC_HOEHLE),CH("Seiteneingang suchen",SC_SEITEN),CH("Alten Turm betreten",SC_TURM),CH("Zurueck zum Tor",SC_WACHE)},
   nullptr,0,-1,-1,0xFF},

  {9,0,"Drachenhoehle",
   "Morgath, der Drache, liegt auf einem Berg aus Gold und Muenzen. Die Mondkrone funkelt auf seinem Kopf. Sein Atem riecht nach Schwefel.",
   {CH("Krone stehlen",SC_GEHEIM),CH("Drachen wecken",SC_KAMPF_DRACHE),CH_REQ("Aelindril rufen",SC_SCHLAFLIED,FL_ELF),CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Geheimer Gang",
   "Ein enger Gang hinter Wasserfaellen fuehrt tiefer in den Berg. Tropfen hallen wie Schritte.",
   {CH("Weiter",SC_GEHEIM),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {7,0,"Hinter der Krone",
   "Hinter dem Goldberg findest du eine schwere Eichentuere mit silbernem Beschlag. Daneben liegt die Mondkrone, nur eine Armlaenge entfernt.",
   {CH("Tuer oeffnen",SC_THRON),CH_GIVE("Krone nehmen und fliehen",SC_FLUCHT,IT_KRONE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {9,1,"Kampf!",
   "Feuer! Hitze! Morgaths Schwingen peitschen durch die Luft. Wuerfle mindestens eine 5 zum Treffen. Eine Kampfaxt gibt +1.",
   {CH_END,CH_END,CH_END,CH_END},
   "Morgath",5,SC_THRON,SC_SEITEN,IT_KRONE},

  {9,0,"Morgath schlaeft",
   "Aelindrils Wiegenlied wiegt den Drachen in tiefen Schlaf. Sein Atem zittert wie Rauch. Die Krone liegt erreichbar auf seinem Kopf.",
   {CH_GIVE("Krone nehmen",SC_THRON,IT_KRONE),CH("Zurueck",SC_HOEHLE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {15,2,"Thronsaal",
   "Die Elfenkoenigin nimmt die Mondkrone entgegen. 'Thrain, Kronenwaechter von Eldoria!' Posaunen ertoenen. Der Friede kehrt zurueck.",
   {CH("Neu starten",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {12,2,"Game Over",
   "Dunkelheit umfaengt dich. Zwergenhall trauert um seinen Sohn. Die Nebel von Eldoria bleiben ewig grau.",
   {CH("Nochmal versuchen",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {10,2,"Sieg!",
   "Mit der Krone im Rucksack entkommst du durch den Seitengang. Aelindril geleitet dich heim. Ganz Eldoria feiert dich als Helden!",
   {CH("Neu starten",SC_TITLE),CH_END,CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {14,0,"Wunschbrunnen",
   "Ein sprechender Fuchs sitzt an einem steinernen Brunnen. 'Ein Kraut heilt fuer zwei Gold. Fuenf Gold bringen dir einen silbernen Kelch - ein Geschenk der Elfen.'",
   {CH_HEAL("Heiltrank (2 Gold)",SC_WALD,2,1),CH_BUY("Silberkelch (5 Gold)",IT_KELCH,5,SC_WALD),CH("Nichts",SC_WALD),CH_END},
   nullptr,0,-1,-1,0xFF},

  {4,0,"Raetsel im Wald",
   "Ein Elf tritt aus dem Schatten. 'Was hat eine Stimme, aber keinen Koerper? Antwortest du falsch, kostet es dich 1 Lebenspunkt. Richtig schenke ich dir einen Schluessel.'",
   {CH_HURT("Die Stille",SC_WALD,1),CH_GIVE("Das Echo",SC_WALD,IT_SCHLUESSEL),CH("Nichts sagen",SC_WALD),CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Alter Turm",
   "Ein verwitterter Turm aus grauem Stein. Das schwere Portal ist mit einem rostigen Schloss verschlossen.",
   {CH_USE("Schluessel benutzen",SC_TURM_INNEN,IT_SCHLUESSEL),CH("Weiter in die Hoehle",SC_HOEHLE),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},

  {8,0,"Turmkammer",
   "In einer alten Truhe liegt ein silberner Kelch, verziert mit Elfenrunen. Daneben staubt ein leeres Buch.",
   {CH_GIVE("Kelch nehmen",SC_INNENHOF,IT_KELCH),CH("Zurueck",SC_INNENHOF),CH_END,CH_END},
   nullptr,0,-1,-1,0xFF},
};

const char* EldoriaGame::itemName(uint8_t i) {
  static const char* n[] = {"Seil","Apfel","Amulett","Schluessel","Trollzahn","Silberkelch","Kampfaxt","Mondkrone"};
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
  if (c.cost > 0 && gold_ < c.cost) { flash("Zu wenig Gold!"); return; }
  if (c.cost > 0) gold_ -= c.cost;
  if (c.use != 0xFF) {
    flags_ &= ~(1UL << c.use);
    char b[40]; snprintf(b, sizeof(b), "%s benutzt", itemName(c.use));
    flash(b);
  }
  if (c.give != 0xFF) {
    flags_ |= (1UL << c.give);
    char b[40]; snprintf(b, sizeof(b), "%s erhalten", itemName(c.give));
    flash(b);
  }
  if (c.setFlag != 0xFF) flags_ |= (1UL << c.setFlag);
  if (c.hurt) {
    hp_ -= c.hurt;
    flash("-1 Lebenspunkt");
    if (hp_ <= 0) { gotoScene(SC_GAMEOVER); return; }
  }
  if (c.heal && hp_ < MAX_HP) { hp_ += c.heal; if (hp_ > MAX_HP) hp_ = MAX_HP; flash("+1 Lebenspunkt"); }
  gotoScene((uint16_t)c.next);
}

void EldoriaGame::fightRoll() {
  const Scene& s = kScenes[cur_];
  int r = random(1, 7) + (hasItem(IT_AXT) ? 1 : 0);
  bool hit = r >= s.enemyRoll;
  snprintf(rollMsg_, sizeof(rollMsg_), "Wurf %d%s: %s", r, hasItem(IT_AXT) ? " (+1)" : "", hit ? "Treffer!" : "Daneben!");
  if (hit) {
    if (s.winItem != 0xFF) flags_ |= (1UL << s.winItem);
    gotoScene((uint16_t)s.winNext);
  } else {
    hp_--;
    if (hp_ <= 0) { gotoScene(SC_GAMEOVER); return; }
    flash("-1 Lebenspunkt");
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
  snprintf(b, sizeof(b), "Zuege:%d", moves_);
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
    tft.drawString("Weiter >", 6, 212);
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
  const char* f = "A: hoch | B: runter | B lang: OK";
  if (kind == 1) f = "A/B: Wuerfeln | B lang: Fliehen";
  else if (kind == 2) f = "B lang: Neustart";
  tft.drawString(f, 6, 302);
}

void EldoriaGame::renderInv() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("INVENTAR", 6, 8);
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
  if (!any) tft.drawString("(leer)", 12, y);
  char b[32];
  snprintf(b, sizeof(b), "Gold: %d   HP: %d/3   Zuege: %d", gold_, hp_, moves_);
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString(b, 6, 250);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A kurz oder B lang: Zurueck", 6, 302);
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
    snprintf(b, sizeof(b), "Zuege: %d", moves_);
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
