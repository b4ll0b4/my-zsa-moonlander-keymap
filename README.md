# Moonlander: Oryx + custom SOCD

Keep editing the layout in Oryx. GitHub Actions downloads a fresh export, adds
the SOCD module stored in this repository, and compiles against the matching
**official ZSA firmware**. There is no custom firmware fork, firmware submodule,
or merge between `main` and `oryx` to maintain.

The normal cycle is **Oryx → one GitHub build → Keymapp**. Oryx stores your layout;
this repository stores the custom SOCD behavior. You do not need to use Git or
install a local QMK build environment for ordinary layout changes.

## Everyday use

1. Edit your layout in [Oryx](https://configure.zsa.io/moonlander/layouts/vr90L/latest/0)
   and click **Compile this layout**.
2. Open [Actions → Fetch and build layout](https://github.com/b4ll0b4/my-zsa-moonlander-keymap/actions/workflows/fetch-and-build-layout.yml)
   and click **Run workflow**.
3. Select **main**, leave **Oryx revision** as **latest**, and start the workflow.
4. Wait for a successful run, then download its **moonlander-vr90L** artifact
   from the run's summary page and extract the ZIP.
5. Open [Keymapp](https://www.zsa.io/flash#flash-keymap), choose the `.bin` inside
   the extracted **firmware** folder, and follow its flashing instructions.

The Oryx layout must remain public. Flash the binary from the Action to include
SOCD. An ordinary binary downloaded directly from Oryx contains only Oryx features.

Clicking **Compile this layout** in Oryx saves the new revision but does not
start the GitHub build. Run the Action manually after your Oryx changes. A
successful Action produces a downloadable binary; flashing it remains manual.

The Action also runs when custom code changes are pushed to `main`, and on pull
requests affecting the build. It has read-only repository permissions and never
commits exports, updates a submodule, or pushes branch merges.

## What happens during a build

Every run automatically:

1. Downloads your requested Oryx revision (`latest` by default).
2. Inserts the three SOCD control keycodes into their reserved gaming-layer slots
   and enables the SOCD module stored in this repository.
3. Downloads the matching official ZSA firmware and its library dependencies.
4. Runs the preparation and SOCD behavior checks, then compiles the Moonlander `.bin`.
5. Uploads the firmware, raw Oryx export, prepared layout, custom module, and build
   metadata together as one artifact.

The generated layout is included in the artifact instead of being committed back
to this repository. This keeps Oryx's generated files separate from the custom
module, so ordinary Oryx edits do not require Git merges or conflict resolution.

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

Keep the three reserved control positions **blank/transparent in Oryx**. The
script inserts their keycodes automatically and stops with an error if a slot
has been assigned another function. Other keys, macros, tap dances, configuration,
and RGB code come from the fresh export. Custom code lives in the module and
does not need edits inside Oryx's generated functions.

Use Oryx for key bindings, tap dances, macros, and key colors. Change the module
for custom SOCD behavior or status indicators. Pushing changes to custom code or
build files on `main` starts a build automatically.

The build uses a temporary official firmware checkout under `.build/qmk/`.
ZSA's internal firmware library submodules are downloaded automatically during
the build; there are no firmware submodules to maintain in this repository.

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
