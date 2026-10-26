import sys, importlib.util
spec = importlib.util.spec_from_file_location("gen", "tools/gen_neonshard_art.py")
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)

RAMP = " .:-=+*#%@"  # dark -> light

def luminance(c):
    r, g, b = c
    return 0.3 * r + 0.6 * g + 0.1 * b

def preview(cv, name):
    print(f"== {name} ==")
    step_x, step_y = 4, 4   # 80x42 Zeichen
    for y in range(0, gen.H, step_y):
        line = ""
        for x in range(0, gen.W, step_x):
            lum = 0
            for dy in range(step_y):
                for dx in range(step_x):
                    if y + dy < gen.H and x + dx < gen.W:
                        lum += luminance(cv.px[y + dy][x + dx])
            lum /= step_x * step_y
            idx = min(len(RAMP) - 1, int(lum / 256 * len(RAMP)))
            line += RAMP[idx]
        print(line)
    print()

for name, fn in [("TITLE", gen.scene_title), ("ALLEY", gen.scene_alley),
                 ("BAR", gen.scene_bar), ("SUBWAY", gen.scene_subway),
                 ("ROOF", gen.scene_roof), ("END_A", gen.scene_end_a),
                 ("END_B", gen.scene_end_b)]:
    cv = fn()
    # bake in the glow like in main()
    if hasattr(cv, 'blur_glow'):
        cv.blur_glow(5)
        cv.bake()
    preview(cv, name)
