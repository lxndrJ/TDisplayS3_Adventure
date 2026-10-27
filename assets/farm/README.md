# SPROUT - Custom Sprite Graphics

Drop PNGs with these names here, then run:

    python3 tools/gen_farm_art.py

If a file exists it replaces the procedurally generated sprite; if it is
missing, the procedural sprite is kept.

Style reference: **Super Retro World - Farming** by Gif
(https://store.godotengine.org/asset/gif/super-retro-world/), free,
custom commercial license. The pack ships 16x16 pixel-art top-down
spritesheets for Godot; sprites here are drawn procedurally in that
look, and real sprites from the pack can be dropped in as overrides.

| File          | Sprite    | Size | Replaces                          |
|---------------|-----------|------|-----------------------------------|
| Soil.png      | Soil      | 16x16| fallow plot                       |
| TilledSoil.png| TilledSoil| 16x16| dug plot                          |
| Sprout.png    | Sprout    | 16x16| growth stage 1                    |
| Crop1.png     | Crop1     | 16x16| growth stage 2                    |
| Crop2.png     | Crop2     | 16x16| growth stage 3 (almost ripe)      |
| Crop3.png     | Crop3     | 16x16| ripe crop (harvest)               |
| Withered.png  | Withered  | 16x16| dried-out crop                    |
| Cursor.png    | Cursor    | 16x16| (reserved: selection cursor)      |
| Farmer.png    | Farmer    | 16x16| player character (front view)     |
| WaterCan.png  | WaterCan  | 16x16| watering can HUD icon             |
| Coin.png      | Coin      | 16x16| gold HUD icon                     |
| Plot.png      | Plot      | 16x16| decorative grass frame            |
| House.png     | House     | 32x32| farmhouse decoration              |

Notes:
- Any input size; scaled nearest-neighbour to the target size.
- RGB, RGBA, grayscale and palette, 8 bit, not interlaced.
- Alpha is ignored: sprites are drawn opaque (full 16x16 tiles).
- Keep the Super Retro World palette (earthy greens/browns, saturated
  crop colors) for a consistent look.
