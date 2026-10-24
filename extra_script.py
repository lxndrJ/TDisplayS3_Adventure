# PlatformIO extra script:
# 1. Deaktiviert User_Setup.h in TFT_eSPI, damit ausschliesslich
#    include/tft_setup.h (Setup206 + USE_HSPI_PORT) greift.
# 2. Fuegt include/ in den globalen Include-Pfad ein, damit auch
#    TFT_eSPI.cpp (Library-Quellcode) tft_setup.h findet.
#    CPPPATH reicht nicht - Library-Builds ueberschreiben das.
#    Stattdessen patchen wir User_Setup_Select.h direkt so, dass
#    es unsere tft_setup.h per absoluten Pfad einbindet.
Import("env")

import os

project_dir = env.subst("$PROJECT_DIR")
libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
pioenv = env.subst("$PIOENV")

# --- 1. include/ global fuer Projekt-Quellcode ---
inc_dir = os.path.join(project_dir, "include")
env.Append(CPPPATH=[inc_dir])

# --- 2. Bootloader pruefen: muss bootloader_opi_80m.elf sein ---
# PlatformIO waehlt bei memory_type=qio_opi faelschlicherweise bootloader_qio_80m.elf
# (kein OPI-PSRAM). board_build.boot=opi in platformio.ini korrigiert das.
# Hier verifizieren wir, welcher Bootloader gebaut wird.
boot_mode = env.BoardConfig().get("build.boot", "")
if boot_mode != "opi":
    print("[extra_script] WARNING: board_build.boot ist nicht 'opi'! "
          "Bootloader unterstuetzt moeglicherweise kein OPI-PSRAM -> Crash!")

# --- 3. TFT_eSPI User_Setup_Select.h patchen ---
select_path = os.path.join(libdeps, pioenv, "TFT_eSPI", "User_Setup_Select.h")
if os.path.isfile(select_path):
    with open(select_path) as f:
        content = f.read()

    # User_Setup.h deaktivieren
    content = content.replace(
        "#include <User_Setup.h>           // Default setup is root library folder",
        "// #include <User_Setup.h>  // DISABLED by extra_script.py",
    )

    # Unsere tft_setup.h per absolutem Pfad einbinden (ersetzt alle Setups)
    tft_setup_abs = os.path.join(inc_dir, "tft_setup.h").replace("\\", "/")
    inject = (
        '#ifdef TFT_ESPI_USER_SETUP_PATH\n'
        '  #define USER_SETUP_LOADED\n'
        '  #include TFT_ESPI_USER_SETUP_PATH\n'
        '#endif\n'
    )
    # Am Anfang nach den Kommentaren einfuegen
    lines = content.split("\n")
    out = []
    inserted = False
    for line in lines:
        out.append(line)
        if not inserted and line.startswith("#ifdef TFT_ESPI_USER_SETUP_PATH"):
            inserted = True
    if not inserted:
        # Einfach vor #ifndef USER_SETUP_LOADED einfuegen
        content = content.replace(
            "#ifndef USER_SETUP_LOADED",
            f'#include "{tft_setup_abs}"\n\n#ifndef USER_SETUP_LOADED',
            1,
        )
    else:
        content = "\n".join(out)

    with open(select_path, "w") as f:
        f.write(content)
    print("[extra_script] TFT_eSPI User_Setup_Select.h gepatcht -> tft_setup.h (absoluter Pfad)")

print("[extra_script] Added to CPPPATH:", inc_dir)
print("[extra_script] Boot mode:", boot_mode, "(opi = korrekt fuer Octal PSRAM)")
