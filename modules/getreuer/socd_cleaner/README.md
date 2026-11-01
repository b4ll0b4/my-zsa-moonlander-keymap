# Moonlander adaptation of SOCD Cleaner

Based on Pascal Getreuer's [SOCD Cleaner](https://getreuer.info/posts/keyboards/socd-cleaner/),
vendored from the module already used by this layout. Original copyright and
Apache 2.0 notices are retained; the license is in `../LICENSE.txt`.

Local changes:

- Filtering is off at boot and restricted to the highest active gaming layer.
- Leaving gaming disables it; entering gaming never enables it automatically.
- `SOCD_LAST` and `SOCD_UP` select a mode without enabling it; `SOCDTOG` toggles.
- The direction pairs live in the module instead of the generated keymap.
- All physical direction holds are tracked while filtering is disabled, too.
  Reports are reconciled on mode, toggle, and layer changes.
- The Moonlander's left indicators show layers 1/2/3; right indicators 1/2 show
  the active SOCD mode. Right indicators are off when filtering is disabled.

The module requires community module API 1.1 and the Moonlander status-LED API.
`SOCD_GAMING_LAYER` is passed from `layout.json` by the preparation script.
