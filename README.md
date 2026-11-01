# Moonlander: Oryx + custom SOCD

Keep editing the layout in Oryx. GitHub Actions downloads a fresh export, adds
the SOCD module stored in this repository, and compiles against the matching
**official ZSA firmware**. There is no custom firmware fork, firmware submodule,
or merge between `main` and `oryx` to maintain.

## Everyday use

1. Edit your layout in [Oryx](https://configure.zsa.io/moonlander/layouts/vr90L/latest/0)
   and click **Compile this layout**.
2. In this repository's **Actions → Fetch and build layout**, click **Run workflow**.
   Leave the revision as `latest`.
3. Download the `moonlander-vr90L` artifact, extract it, and flash the `.bin`
   inside its `firmware` folder using [Keymapp](https://www.zsa.io/flash#flash-keymap).

The Oryx layout must remain public. Flash the binary from the Action to include
SOCD. An ordinary binary downloaded directly from Oryx contains only Oryx features.

The Action also runs when custom code changes are pushed to `main`, and on pull
requests affecting the build. It has read-only repository permissions and never
commits exports, updates a submodule, or pushes branch merges.

## SOCD controls and indicators

**SOCD is off at startup, including in the gaming layer.** Entering gaming does
not enable it. Leaving gaming disables it, so returning to gaming starts off
again. The selected resolution is remembered until power-off; its enabled state
is never saved. Selecting a resolution does not enable SOCD.

The three existing controls stay on the last three keys of the right-hand bottom
row in gaming (layer 1), from left to right:

| Control | Function |
| --- | --- |
| `SOCD_LAST` | Select Last Input Priority for both direction pairs. |
| `SOCD_UP` | Select Up Priority vertically, Neutral horizontally. |
| `SOCDTOG` (Print Screen position) | Enable/disable the selected mode. |

Direction pairs remain **Space/D** (up/down) and **S/F** (left/right). SOCD
processing and its controls are restricted to gaming, even if a control keycode
is accidentally assigned to another layer. Changing modes, toggling, or leaving
gaming while directions are held reconciles the keyboard report immediately.

The six small status indicators are independent of the key backlighting:

| Indicators | Meaning |
| --- | --- |
| Left 1 / 2 / 3 | Gaming / left mouse / right mouse layer, as before. |
| Right 1 (blue) | Unused, off. |
| Right 2 (green) | SOCD enabled with Last Input Priority. |
| Right 3 (red) | SOCD enabled with Up Priority + horizontal Neutral. |
| All right indicators off | SOCD disabled. |

Numbering follows the board's `STATUS_LED_1` through `STATUS_LED_6` API, from the
outer left indicator to the outer right indicator. The module takes ownership of
these indicators with `MOONLANDER_USER_LEDS`; Oryx still controls key colors.
The status indicators have fixed colors and support on/off control. Last Input
Priority uses green; Up Priority uses red.

## Where to make changes

| Location | Purpose |
| --- | --- |
| `layout.json` | Layout ID, board revision, gaming layer, and the three reserved control slots. |
| `modules/getreuer/socd_cleaner/` | SOCD behavior, direction pairs, modes, and status LEDs. |
| `scripts/prepare_layout.py` | Download the Oryx export and insert the three controls. |
| `.github/workflows/fetch-and-build-layout.yml` | Download official firmware, compile, and upload the result. |
| `vr90L/` | Historical merged source, retained for reference; not used by the new build. |

Keep the three reserved control positions **blank/transparent in Oryx**. The
script inserts their keycodes automatically and stops with an error if a slot
has been assigned another function. Other keys, macros, tap dances, configuration,
and RGB code come from the fresh export. Custom code lives in the module and
does not need edits inside Oryx's generated functions.

The old `oryx` branch and the old firmware fork can remain on GitHub as archives.
Existing local files in `qmk_firmware/` are retained and ignored; the build uses a
temporary official checkout under `.build/qmk/`. ZSA's internal firmware library
submodules are downloaded automatically during the build.

## Checking or rebuilding

The artifact includes the raw Oryx export, prepared sources, Oryx revision,
firmware branch, and exact ZSA commit used. To rebuild a layout revision, enter
its ID instead of `latest` when running the Action. The ZSA branch may advance;
the commit file records which version each build actually used.

With Python 3 and curl, source preparation can also run locally:

```powershell
python scripts/prepare_layout.py
# Or use an already downloaded, unmodified Oryx source folder:
python scripts/prepare_layout.py --source path/to/oryx_source
python -m unittest discover -s tests -v
```

The Action tests the C module's report behavior before compiling firmware.
Hardware validation is still required after flashing: confirm startup and gaming
entry leave SOCD off, enable each mode, check both direction pairs and indicators,
then leave/re-enter gaming and confirm it is off again.

Based on [Pascal Getreuer's SOCD Cleaner](https://getreuer.info/posts/keyboards/socd-cleaner/)
and [Pierre Poulain's Oryx integration](https://blog.zsa.io/oryx-custom-qmk-features/).
The adapted module retains its Apache 2.0 license; see `modules/getreuer/LICENSE.txt`.
