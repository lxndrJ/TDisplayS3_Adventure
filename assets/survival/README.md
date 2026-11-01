# Survival fonts (Roboto, Apache License 2.0)

`Roboto20.vlw` / `Roboto28.vlw` are anti-aliased TFT_eSPI smooth fonts
generated from Roboto-Regular.ttf (Google, Apache 2.0) with
`tools/gen_roboto_vlw.py`. Charset: U+0020-007E + U+00A0-00FF
(Basic Latin + Latin-1 Supplement -> real umlauts).

Flash them to the FAT "storage" partition once:

    pio run -t uploadfs

(PlatformIO copies `data/` there; the copies of the .vlw files live in
`data/.) Without the files the game falls back to built-in Font 4/2.
