#include "neonshard.h"

// ---------------------------------------------------------------- Konstanten

static constexpr int16_t SW = 320;   // Landscape-Breite
static constexpr int16_t SH = 170;   // Landscape-Hoehe
static constexpr int16_t BAND_Y = 100;  // Dialogband-Offset (Font 2, 16px)
static constexpr int16_t BAND_H = SH - BAND_Y;
static const uint16_t COL_BAND   = 0x0008;  // fast schwarz-blau
static const uint16_t COL_ACCENT = 0x07FF;  // Neon-Cyan
static const uint16_t COL_WARM   = 0xFFE0;  // Neon-Gelb
static const uint16_t COL_DIM    = 0x630C;  // dunkles Cyan

// ---------------------------------------------------------------- Szenen

const NeonShardGame::SceneDef NeonShardGame::kScenes[SC_COUNT] = {
  // SC_ALLEY - die Gasse mit Drone und Riko
  { ART_ALLEY, 4, {
    { H_ALLEY_DRONE, 238, 108, 76, 58, "Courier-Droide" },
    { H_ALLEY_RIKO,   48,  86, 62, 78, "Riko" },
    { H_ALLEY_BAR,     6, 116, 78, 28, "> Bar" },
    { H_ALLEY_SUBWAY, 236,116, 78, 28, "> U-Bahn" },
  }},
  // SC_BAR - Mama Bytes Bar
  { ART_BAR, 3, {
    { H_BAR_MAMA,  126,  78, 76, 88, "Mama Byte" },
    { H_BAR_STREET,   6, 116, 78, 28, "> Gasse" },
    { H_BAR_SUBWAY, 236, 116, 78, 28, "> U-Bahn" },
  }},
  // SC_SUBWAY - Bahnsteig mit Agentin Chen
  { ART_SUBWAY, 3, {
    { H_SUB_CHEN,  118,  84, 70, 84, "Agentin Chen" },
    { H_SUB_STREET,   6, 116, 78, 28, "> Gasse" },
    { H_SUB_ROOF,  236, 116, 78, 28, "> Helix-Turm" },
  }},
  // SC_ROOF - Antennenwald auf dem Turm
  { ART_ROOF, 2, {
    { H_ROOF_ANTENNA, 128,  16, 66, 140, "Sendeantenne" },
    { H_ROOF_SUBWAY,    6, 116, 78, 28, "> U-Bahn" },
  }},
  // SC_ENDA - Morgen nach der Freilassung
  { ART_ENDA, 1, {
    { H_END_EXIT, 0, 0, SW, SH, "B lang: Menue" },
  }},
  // SC_ENDB - Neonregen nach dem Verkauf
  { ART_ENDB, 1, {
    { H_END_EXIT, 0, 0, SW, SH, "B lang: Menue" },
  }},
};

// ---------------------------------------------------------------- Spielstart

void NeonShardGame::init() {
  // Landscape: Das Abenteuer nutzt die volle Breite des Panels
  gfx.tft().setRotation(1);
  if (!fb_.created()) fb_.createSprite(SW, SH);
  if (!bg_) {
    bg_ = (uint16_t*)(psramFound() ? ps_malloc(SW * SH * 2)
                                   : malloc(SW * SH * 2));
  }
  phase_    = PH_TITLE;
  hasShard_ = false;
  metMama_  = false;
  tipMama_  = false;
  metChen_  = false;
  wantsMenu = false;
  decodeArt(ART_TITLE);  // Titelscreen liegt sofort an
}

void NeonShardGame::decodeArt(uint8_t art) {
  // Szenengrafik RLE-dekodieren (Generator: tools/gen_neonshard_art.py)
  if (!bg_) return;
  const RleArt* a = nullptr;
  switch (art) {
    case ART_ALLEY:  a = &kArtAlley;  break;
    case ART_BAR:    a = &kArtBar;    break;
    case ART_SUBWAY: a = &kArtSubway; break;
    case ART_ROOF:   a = &kArtRoof;   break;
    case ART_ENDA:   a = &kArtEndA;   break;
    case ART_ENDB:   a = &kArtEndB;   break;
    default:         a = &kArtTitle;  break;
  }
  uint32_t i = 0;
  for (uint16_t r = 0; r < a->runs; r++) {
    uint32_t n = a->data[r].count;
    while (n--) bg_[i++] = a->data[r].color;
  }
}

void NeonShardGame::quit() {
  fb_.deleteSprite();
  if (bg_) { free(bg_); bg_ = nullptr; }
}

void NeonShardGame::startStory() {
  phase_ = PH_SCENE;
  gotoScene(SC_ALLEY);
}

void NeonShardGame::gotoScene(uint8_t sc) {
  scene_ = sc;
  sel_   = 0;
  phase_ = (sc == SC_ENDA || sc == SC_ENDB) ? PH_END : PH_SCENE;
  decodeArt(kScenes[sc].art);
}

// ---------------------------------------------------------------- Eingabe

void NeonShardGame::update(const InputState& in, uint32_t dtMs) {
  (void)dtMs;
  switch (phase_) {
    case PH_TITLE:
      if (in.longPressed[(uint8_t)Key::B]) startStory();
      break;

    case PH_SCENE: {
      const SceneDef& s = kScenes[scene_];
      if (in.pressed[(uint8_t)Key::A]) {
        sel_ = (uint8_t)((sel_ + s.nHot - 1) % s.nHot);
      }
      if (in.pressed[(uint8_t)Key::B]) {
        sel_ = (sel_ + 1) % s.nHot;
      }
      if (in.longPressed[(uint8_t)Key::B]) interact(s.hot[sel_].id);
      break;
    }

    case PH_DIALOG:
      if (dlgIdx_ < dlgLineCount_ - 1) {
        // Zeilen vorwaerts blaettern (A/B kurz), B lang ebenso
        if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
            in.longPressed[(uint8_t)Key::B])
          dlgIdx_++;
      } else if (dlgChoiceCount_ > 0) {
        if (in.pressed[(uint8_t)Key::A])
          dlgChoice_ = (uint8_t)((dlgChoice_ + dlgChoiceCount_ - 1) % dlgChoiceCount_);
        if (in.pressed[(uint8_t)Key::B])
          dlgChoice_ = (dlgChoice_ + 1) % dlgChoiceCount_;
        if (in.longPressed[(uint8_t)Key::B])
          dialogEnd(dlgId_, dlgChoice_);
      } else {
        if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
            in.longPressed[(uint8_t)Key::B])
          dialogEnd(dlgId_, 255);
      }
      break;

    case PH_END:
      if (in.longPressed[(uint8_t)Key::B]) wantsMenu = true;
      break;
  }
}

void NeonShardGame::beginDialog(uint8_t dlgId, const char* const* lines,
                                uint8_t nLines, const char* const* choices,
                                uint8_t nChoices) {
  dlgId_         = dlgId;
  dlgLines_      = lines;
  dlgLineCount_  = nLines;
  dlgIdx_        = 0;
  dlgChoices_    = choices;
  dlgChoiceCount_ = nChoices;
  dlgChoice_     = 0;
  phase_         = PH_DIALOG;
}

// ---------------------------------------------------------------- Geschichte

void NeonShardGame::interact(uint8_t hs) {
  switch (hs) {
    case H_ALLEY_DRONE: {
      static const char* const L1[] = {
        "Vex: Ein zerschossener Courier-Droide. Noch warm.",
        "Vex: In seiner Ladebuchse blinkt ein Datensplitter. Der gehoert wohl Helix.",
        "Vex: Den behalte ich. Meine Schwester braucht eine Behandlung, keine Prinzipien.",
      };
      static const char* const L2[] = {
        "Vex: Nur noch Schrott und kalte Neonluft.",
      };
      if (hasShard_) beginDialog(DLG_DRONE2, L2, 1, nullptr, 0);
      else           beginDialog(DLG_DRONE,  L1, 3, nullptr, 0);
      break;
    }

    case H_ALLEY_RIKO: {
      static const char* const L[] = {
        "Riko: Hey Vex! Bloss nicht mit dem Ding rumlaufen.",
        "Riko: Das ist kein normaler Splitter. Da wohnt was drin.",
        "Riko: Mama Byte weiss mehr. Ihre Bar ist gleich links.",
        "Riko: Aber pass auf: Helix scherzt nicht.",
      };
      static const char* const L2[] = {
        "Riko: Und? Was ist auf dem Splitter?",
        "Vex: Eine Stimme. Sie nennt sich Ghost.",
        "Riko: ...Dann rannte mal. Bevor jemand anderes sie hoert.",
      };
      if (hasShard_) beginDialog(DLG_RIKO2, L2, 3, nullptr, 0);
      else           beginDialog(DLG_RIKO,  L,  4, nullptr, 0);
      break;
    }

    case H_ALLEY_BAR:
      gotoScene(SC_BAR);
      break;
    case H_ALLEY_SUBWAY:
    case H_BAR_SUBWAY:
      gotoScene(SC_SUBWAY);
      break;
    case H_BAR_STREET:
      gotoScene(SC_ALLEY);
      break;

    case H_BAR_MAMA: {
      static const char* const GATE[] = {
        "Mama Byte: Vex, Kind. Ohne Ware kein Gespraech. Du weisst, wie das laeuft.",
      };
      static const char* const L[] = {
        "Mama Byte: Hmm... ein Bewusstseins-Splitter. Eine FREIE KI, Vex.",
        "Ghost: <aus dem Splitter> Ich zahle nicht in Credits. Ich zahle in Freiheit.",
        "Mama Byte: 1000 Cred dafuer - oder du hoerst ihre Geschichte.",
        "Mama Byte: Deine Wahl, Courier.",
      };
      static const char* const C[] = { "Behalten - zuhoeren",
                                        "Verkaufen (+1000C)" };
      if (!hasShard_)      beginDialog(DLG_MAMA_GATE, GATE, 1, nullptr, 0);
      else if (!metMama_) { beginDialog(DLG_MAMA, L, 4, C, 2); }
      else {
        static const char* const AGAIN[] = {
          "Mama Byte: Die Zeit drueckt, Kind. Was wirds?",
        };
        beginDialog(DLG_MAMA, AGAIN, 1, C, 2);
      }
      break;
    }

    case H_SUB_CHEN: {
      static const char* const GATE[] = {
        "Chen: Sie verschwindet zwischen zwei Zuegen. Nichts zu holen hier.",
      };
      static const char* const L[] = {
        "Chen: Courier Vex. Helix weiss, was du da hast. Ein Leck, nichts weiter.",
        "Chen: 2000 Cred. Sofort, anonym, keine Fragen.",
        "Chen: Oder du findest heraus, was Helix mit Lecks macht.",
      };
      static const char* const C[] = { "Ablehnen - kein Leck",
                                        "Verkaufen (+2000C)" };
      if (!hasShard_)        beginDialog(DLG_CHEN_GATE, GATE, 1, nullptr, 0);
      else if (!metChen_)     beginDialog(DLG_CHEN, L, 3, C, 2);
      else {
        static const char* const AGAIN[] = {
          "Chen: Letzte Chance, Courier.",
        };
        beginDialog(DLG_CHEN, AGAIN, 1, C, 2);
      }
      break;
    }

    case H_SUB_ROOF: {
      static const char* const L[] = {
        "Vex: Ohne Grund fahre ich nicht zum Helix-Turm.",
      };
      if (tipMama_) gotoScene(SC_ROOF);
      else          beginDialog(DLG_NO_TOWER, L, 1, nullptr, 0);
      break;
    }
    case H_SUB_STREET:
      gotoScene(SC_ALLEY);
      break;

    case H_ROOF_ANTENNA: {
      static const char* const GATE[] = {
        "Vex: Ein toter Wald aus Metall. Ohne Splitter sinnlos.",
      };
      static const char* const L[] = {
        "Vex: Die alte Sendeantenne... Mama Bytes Tipp war gut.",
        "Ghost: <summt> Ich sende mich selbst. Ueberallhin. Niemand besitzt den Regen.",
        "Ghost: Eines verspreche ich, Vex: Deine Schwester wird nicht vergessen.",
        "Vex: Dann flieg, Ghost.",
      };
      if (hasShard_) beginDialog(DLG_ANTENNA, L, 4, nullptr, 0);
      else           beginDialog(DLG_ANTENNA_GATE, GATE, 1, nullptr, 0);
      break;
    }
    case H_ROOF_SUBWAY:
      gotoScene(SC_SUBWAY);
      break;

    case H_END_EXIT:
      wantsMenu = true;
      break;
  }
}

void NeonShardGame::dialogEnd(uint8_t dlgId, uint8_t choice) {
  switch (dlgId) {
    case DLG_DRONE:
      hasShard_ = true;
      break;
    case DLG_MAMA:
      metMama_ = true;
      if (choice == 1) { gotoScene(SC_ENDB); return; }
      tipMama_ = true;
      break;
    case DLG_CHEN:
      metChen_ = true;
      if (choice == 1) { gotoScene(SC_ENDB); return; }
      break;
    case DLG_ANTENNA:
      gotoScene(SC_ENDA);
      return;
    default:
      break;
  }
  phase_ = PH_SCENE;  // zurueck in die Szene
}

// ---------------------------------------------------------------- Zeichnen

void NeonShardGame::draw() {
  if (!fb_.created() || !bg_) {
    TFT_eSPI& tft = gfx.tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Framebuffer alloc fehlgeschlagen", 6, 8);
    return;
  }
  if (phase_ == PH_TITLE)      drawTitle(fb_);
  else if (phase_ == PH_END)   drawEnd(fb_);
  else {
    drawScene(fb_);
    if (phase_ == PH_DIALOG) drawDialog(fb_);
  }
  fb_.pushSprite(0, 0);
}

void NeonShardGame::drawTitle(TFT_eSprite& fb) {
  // Titelgrafik; der Schriftzug ist Teil des generierten Bildes
  fb.pushImage(0, 0, SW, SH, bg_);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, 0x0000);
  fb.setTextDatum(TL_DATUM);
  fb.drawString("B lang: Start", 6, 158);
}

void NeonShardGame::drawScene(TFT_eSprite& fb) {
  fb.pushImage(0, 0, SW, SH, bg_);
  const SceneDef& s = kScenes[scene_];

  // Hotspot-Markierungen: Eck-Klammern, Auswahl in Neon
  for (uint8_t i = 0; i < s.nHot; i++) {
    const Hotspot& h = s.hot[i];
    uint16_t col = (i == sel_) ? COL_ACCENT : 0x39E7;
    int16_t x1 = h.x + h.w - 1, y1 = h.y + h.h - 1;
    int16_t c = 6;
    fb.drawFastHLine(h.x, h.y, c, col);           fb.drawFastVLine(h.x, h.y, c, col);
    fb.drawFastHLine(x1 - c + 1, h.y, c, col);    fb.drawFastVLine(x1, h.y, c, col);
    fb.drawFastHLine(h.x, y1, c, col);            fb.drawFastVLine(h.x, y1 - c + 1, c, col);
    fb.drawFastHLine(x1 - c + 1, y1, c, col);      fb.drawFastVLine(x1, y1 - c + 1, c, col);
  }

  // Hinweiszeile unten: gewaehltes Ziel + Steuerung
  fb.fillRect(0, 148, SW, 22, COL_BAND);
  fb.drawFastHLine(0, 148, SW, 0x1082);
  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(2);
  fb.setTextColor(COL_WARM, COL_BAND);
  fb.drawString(s.hot[sel_].label, 6, 151);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BAND);
  fb.drawString("A/B: Ziel | B lang: Aktion", SW - 104, 155);
}

void NeonShardGame::drawDialog(TFT_eSprite& fb) {
  const char* line = dlgLines_[dlgIdx_];
  fb.fillRect(0, BAND_Y, SW, BAND_H, COL_BAND);
  fb.drawFastHLine(0, BAND_Y, SW, COL_ACCENT);

  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(2);   // Font 2 (16px): gut lesbar auf dem kleinen Panel

  // "Name: " praefix farbig auf eigener Zeile, Text darunter umgebrochen
  const char* colon = strstr(line, ": ");
  const char* text  = line;
  int16_t py = BAND_Y + 3;
  if (colon && colon - line <= 16) {
    int n = (int)(colon - line);
    char who[17];
    memcpy(who, line, n); who[n] = 0;
    fb.setTextColor(COL_ACCENT, COL_BAND);
    fb.drawString(who, 6, py);
    py += 17;
    text = colon + 2;
  }
  // Text in bis zu 3 Zeilen a 24 Zeichen (Font 2), am Leerzeichen umbrechen
  char buf[25];
  int len = strlen(text), off = 0;
  fb.setTextColor(0xFFFF, COL_BAND);
  for (int row = 0; row < 3 && off < len; row++) {
    int n = len - off;
    if (n > 24) {
      n = 24;
      while (n > 12 && text[off + n - 1] != ' ') n--;
    }
    memcpy(buf, text + off, n); buf[n] = 0;
    fb.drawString(buf, 6, py + row * 17);
    off += n;
  }

  // Zeilen-Fortschritt
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BAND);
  char pg[10];
  snprintf(pg, sizeof(pg), "%u/%u", dlgIdx_ + 1, dlgLineCount_);
  fb.drawString(pg, SW - 26, BAND_Y + 3);

  // Auswahlmoeglichkeiten (nur auf der letzten Zeile), Font 2
  if (dlgIdx_ == dlgLineCount_ - 1 && dlgChoiceCount_ > 0) {
    fb.setTextFont(2);
    int16_t cy = BAND_Y + 20;
    for (uint8_t i = 0; i < dlgChoiceCount_ && i < 3; i++) {
      bool on = (i == dlgChoice_);
      fb.setTextColor(on ? COL_WARM : 0x7BEF, COL_BAND);
      fb.drawString(on ? "> " : "  ", 6, cy + i * 17);
      fb.drawString(dlgChoices_[i], 28, cy + i * 17);
    }
  } else if (dlgIdx_ < dlgLineCount_ - 1) {
    fb.setTextColor(COL_DIM, COL_BAND);
    fb.drawString("B: weiter", SW - 60, SH - 10);
  }
}

void NeonShardGame::drawEnd(TFT_eSprite& fb) {
  fb.pushImage(0, 0, SW, SH, bg_);
  // Comic-Caption: kurze Zeilen in Grossschrift-Qualitaet (Font 2)
  static const char* const A[] = {
    "Der Splitter ist leer.",
    "Miras Rechnung: bezahlt.",
    "Anonym. Von ueberall her.",
    "GHOST: Ich bin noch da.",
  };
  static const char* const B[] = {
    "2000 Cred im Sack.",
    "Auf dem Splitter: GELÖSCHT.",
    "Der Regen schmeckt bitter.",
    "Riko sieht dich nicht mehr an.",
  };
  const char* const* ep = (scene_ == SC_ENDA) ? A : B;
  fb.fillRect(0, BAND_Y, SW, BAND_H, COL_BAND);
  fb.drawFastHLine(0, BAND_Y, SW, (scene_ == SC_ENDA) ? COL_WARM : 0xF81F);
  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(2);
  fb.setTextColor(0xFFFF, COL_BAND);
  for (uint8_t i = 0; i < 4; i++) fb.drawString(ep[i], 6, BAND_Y + 4 + i * 17);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BAND);
  fb.drawString("B lang: Menue", SW - 80, SH - 10);
}
