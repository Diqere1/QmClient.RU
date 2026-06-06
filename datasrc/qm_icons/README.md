# Qm Icon Sources

Place Tabler SVG source files in `datasrc/qm_icons/tabler/`.

SVG files are build-time inputs only. Runtime code must load the generated
`data/qmclient/icons/qm_icons_*x.png` atlas and matching JSON manifest through
`CQmIconManager`; do not parse SVG in the client.

The atlas builder prefers `resvg`, `inkscape`, or `magick` when available. If
none is installed, it can render the simple line/circle/polyline SVG subset used
by the bundled Qm icon sources with Pillow, keeping the fallback build-time only.

Generate atlases with:

```sh
python scripts/qm_build_icon_atlas.py --source datasrc/qm_icons/tabler --output data/qmclient/icons --sizes 1 2 4
```
