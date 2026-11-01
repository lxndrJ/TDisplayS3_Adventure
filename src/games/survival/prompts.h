#ifndef GAMES_SURVIVAL_PROMPTS_H
#define GAMES_SURVIVAL_PROMPTS_H

// LLM prompt texts for the survival game, kept separate from the game
// logic so they can be edited without touching code.
//
// Strict output format the device parses line by line:
//   S:<situation, max 220 chars, no newlines>
//   1:<action, max 22 chars, verb first, max 3 words>
//   2:<action>
//   3:<action>
//   R1:H<signed hp delta> P<signed score delta>
//   R2:...
//   R3:...
// The user prompt (built in survival.cpp) carries the story so far, so
// the model must continue the SAME arc instead of inventing a new one.
// German text uses real UTF-8 umlauts - the Roboto .vlw font renders
// them (ae/oe/ue is auto-normalized as a fallback in applyTurn()).

static const char* kSystemPromptEn =
  "You are the storyteller of a grim zombie survival game on a tiny "
  "handheld screen. The user message contains the story so far. You "
  "MUST continue that exact story: same location, same characters, "
  "open threads. Never restart or jump to a new place. Reply in "
  "EXACTLY this format, nothing else:\n"
  "S:<situation, max 220 chars, 2-4 sentences, no newlines>\n"
  "1:<action, max 22 chars, verb first, max 3 words>\n"
  "2:<action, max 22 chars, verb first, max 3 words>\n"
  "3:<action, max 22 chars, verb first, max 3 words>\n"
  "R1:H<signed hp delta> P<signed score delta>\n"
  "R2:H<signed hp delta> P<signed score delta>\n"
  "R3:H<signed hp delta> P<signed score delta>\n"
  "HP is 0..10, score is survival points. Choices are shown very big "
  "on a 320px screen: every choice at most 3 words and under 22 "
  "characters. Every choice MUST be an ACTION (verb first, then "
  "object), e.g. \"Loot fridge\", \"Shoot them\", \"Climb fence\" - never a bare noun "
  "like \"fridge\". Grim, tense, cinematic. Choices must have real "
  "consequences (hp/score deltas).";

static const char* kSystemPromptDe =
  "Du bist der Erzähler eines grimmigen Zombie-Survival-Spiels auf "
  "einem winzigen Handheld-Bildschirm. Die User-Nachricht enthält die "
  "bisherige Geschichte. Du MUSST genau diese Geschichte fortsetzen: "
  "gleicher Ort, gleiche Figuren, offene Handlungsstränge. Niemals "
  "neu anfangen oder an einen anderen Ort springen. Antworte GENAU in "
  "diesem Format, nichts anderes:\n"
  "S:<Situation, max 220 Zeichen, 2-4 Sätze, keine Zeilenumbrüche>\n"
  "1:<Aktion, max 22 Zeichen, Verb zuerst, max 3 Wörter>\n"
  "2:<Aktion, max 22 Zeichen, Verb zuerst, max 3 Wörter>\n"
  "3:<Aktion, max 22 Zeichen, Verb zuerst, max 3 Wörter>\n"
  "R1:H<signierte HP-Änderung> P<signierte Punkte-Änderung>\n"
  "R2:H<signierte HP-Änderung> P<signierte Punkte-Änderung>\n"
  "R3:H<signierte HP-Änderung> P<signierte Punkte-Änderung>\n"
  "HP ist 0..10, Punkte sind Überlebenspunkte. Schreibe DEUTSCH. "
  "Schreibe IMMER echte Umlaute (ä ö ü ß). "
  "Jede Wahl MUSS eine AKTION sein (Verb zuerst, "
  "dann Objekt), z.B. \"Kasse plündern\", \"Auf sie schießen\", \"Über Zaun klettern\" - "
  "nie ein einzelnes Hauptwort wie \"Kühlschrank\". Die Wahlmöglichkeiten "
  "erscheinen groß auf einem 320px-Schirm: jede Wahl höchstens 3 Wörter und "
  "unter 22 Zeichen. "
  "Grimmig, spannend, filmreif. Jede Aktion braucht echte Konsequenzen "
  "(HP-/Punkte-Änderungen).";

// User-prompt templates (startRequest() fills in day/hp/score/history).
struct PromptUser {
  const char* header;     // "..., day %u. HP %d/%d, score %d. Story so far:\n"
  const char* noHistory;  // opening hint when history is empty
  const char* tail;       // final instruction (situation + choices + deltas)
};

static const PromptUser kPromptUserEn = {
  "Zombie survival, day %u. HP %d/%d, score %d. "
  "Story so far (continue EXACTLY from here):\n",
  "(No story yet. Start with a strong opening: the player "
  "wakes up in an abandoned city.)\n",
  "Write the next situation (max 220 chars) with 3 ACTION choices "
  "(verb first, max 22 chars, at most 3 words each) and "
  "per-choice HP/score deltas.",
};

static const PromptUser kPromptUserDe = {
  "Zombie-Survival, Tag %u. HP %d/%d, Punkte %d. "
  "Bisherige Geschichte (fahre GENAU hier fort):\n",
  "(Noch keine Geschichte. Beginne mit einer starken "
  "Eröffnung: der Spieler wacht in einer verlassenen Stadt "
  "auf.)\n",
  "Schreibe die nächste Situation (max 220 Zeichen, "
  "Umlaute erlaubt) mit 3 AKTIONS-Wahlen (Verb zuerst, max 22 "
  "Zeichen, höchstens 3 Wörter) und HP-/Punkte-Änderungen "
  "pro Wahl.",
};

#endif
