# Qm Icon Atlases

This directory is for generated Qm icon atlas files:

- `qm_icons_1x.png` / `qm_icons_1x.json`
- `qm_icons_2x.png` / `qm_icons_2x.json`
- `qm_icons_4x.png` / `qm_icons_4x.json`

The JSON manifest stores icon IDs and pixel rects. `CQmIconManager` converts
those rects to UV coordinates at runtime and renders from PNG only.
