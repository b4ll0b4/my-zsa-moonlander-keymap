"""Download a fresh Oryx export and add the local SOCD module, without Git merges."""

import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
from zipfile import ZipFile
from io import BytesIO

ROOT = Path(__file__).resolve().parents[1]


def request(url, data=None):
    # curl uses the operating system's HTTPS trust configuration (including
    # Windows' certificate-chain handling), like the original build workflow.
    command = ["curl", "--fail", "--silent", "--show-error", "--location",
               "--connect-timeout", "15", "--max-time", "60", url]
    if data is not None:
        command += ["--header", "Content-Type: application/json", "--data-binary", "@-"]
        data = json.dumps(data).encode()
    result = subprocess.run(command, input=data, capture_output=True, timeout=70)
    if result.returncode:
        raise RuntimeError(f"HTTPS request failed: {result.stderr.decode(errors='replace').strip()}")
    return result.stdout


def download(config, revision, destination):
    response = json.loads(request("https://oryx.zsa.io/graphql", {
        "query": "query getLayout($hashId: String!, $revisionId: String!, $geometry: String) {layout(hashId: $hashId, geometry: $geometry, revisionId: $revisionId) {revision {hashId qmkVersion title}}}",
        "variables": {"hashId": config["layout_id"], "geometry": config["keyboard"], "revisionId": revision},
    }))
    if response.get("errors"):
        raise ValueError(f"Oryx returned errors: {response['errors']}")
    metadata = response["data"]["layout"]["revision"]
    if not metadata or not re.fullmatch(r"[A-Za-z0-9]+", metadata["hashId"]):
        raise ValueError("Oryx did not return a valid public layout revision")
    version = float(metadata["qmkVersion"])
    if not version.is_integer() or version < 25:
        raise ValueError("This setup requires Oryx firmware 25 or newer (community module API 1.1)")
    metadata["firmware_ref"] = f"firmware{int(version)}"
    metadata["layout_id"] = config["layout_id"]
    metadata["keyboard"] = config["keyboard"]
    destination.mkdir(parents=True, exist_ok=True)
    with ZipFile(BytesIO(request(f"https://oryx.zsa.io/source/{metadata['hashId']}"))) as archive:
        names = set()
        for entry in archive.infolist():
            parts = Path(entry.filename).parts
            if entry.is_dir() or not any(part.endswith("_source") for part in parts[:-1]):
                continue
            name = parts[-1]
            if name in names:
                raise ValueError(f"Duplicate source filename: {name}")
            names.add(name)
            (destination / name).write_bytes(archive.read(entry))
    return metadata


def patch_keymap(source, config):
    """Edit only three arguments of the gaming layout; preserve all other source bytes."""
    # Refuse already-customized snapshots: fresh Oryx exports are the input.
    if re.search(r"\b(?:SOCD_LAST|SOCD_UP|socd_opposing_pairs)\b", source):
        raise ValueError("Expected an unmodified Oryx keymap, not the old merged custom keymap")
    layer = config["gaming_layer"]
    matches = list(re.finditer(rf"\[\s*{layer}\s*\]\s*=\s*LAYOUT_moonlander\s*\(", source))
    if len(matches) != 1:
        raise ValueError(f"Expected exactly one Moonlander gaming layer [{layer}]")
    start = matches[0].end()
    depth, argument_start, spans = 1, start, []
    for pos in range(start, len(source)):
        char = source[pos]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                spans.append((argument_start, pos))
                break
        elif char == "," and depth == 1:
            spans.append((argument_start, pos))
            argument_start = pos + 1
    if depth != 0 or len(spans) != 72:
        raise ValueError(f"Expected 72 Moonlander keys, found {len(spans)}")
    replacements = []
    for slot, keycode in config["socd_keys"].items():
        left, right = spans[int(slot)]
        current = source[left:right]
        # These are intentionally reserved blank keys in Oryx. Fail instead of
        # silently replacing a new binding if they are reassigned in the GUI.
        if current.strip() not in ("KC_TRANSPARENT", "KC_TRNS", "_______", "KC_NO", "XXXXXXX"):
            raise ValueError(f"SOCD slot {slot} is no longer blank: {current.strip()}")
        leading = len(current) - len(current.lstrip())
        trailing = len(current.rstrip())
        replacements.append((left + leading, left + trailing, keycode))
    for left, right, keycode in sorted(replacements, reverse=True):
        source = source[:left] + keycode + source[right:]
    return source


def prepare(source, destination, config):
    if source.resolve() == destination.resolve():
        raise ValueError("Keep the original Oryx source separate from the prepared layout")
    keymap = patch_keymap((source / "keymap.c").read_bytes().decode(), config)
    manifest = json.loads((source / "keymap.json").read_text())
    manifest["modules"] = list(dict.fromkeys([*manifest.get("modules", []), "getreuer/socd_cleaner"]))
    destination.mkdir(parents=True, exist_ok=True)
    for file in source.iterdir():
        if file.is_file():
            shutil.copy2(file, destination / file.name)
    (destination / "keymap.c").write_bytes(keymap.encode())
    (destination / "keymap.json").write_text(json.dumps(manifest, indent=2) + "\n")
    # Share the selected layer with the module, without editing Oryx's config.h.
    with (destination / "rules.mk").open("a") as rules:
        rules.write(f"\nOPT_DEFS += -DSOCD_GAMING_LAYER={config['gaming_layer']}\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, help="Use a previously downloaded raw Oryx source folder")
    parser.add_argument("--revision", default="latest")
    parser.add_argument("--output", type=Path, default=ROOT / ".build/layout")
    args = parser.parse_args()
    config = json.loads((ROOT / "layout.json").read_text())
    metadata = None
    source = args.source
    if source is None:
        source = args.output.parent / "oryx-source"
        metadata = download(config, args.revision, source)
    prepare(source, args.output, config)
    if metadata:
        (args.output.parent / "build-info.json").write_text(json.dumps(metadata, indent=2) + "\n")
        print(metadata["firmware_ref"])
    else:
        print(f"Prepared {args.output}")


if __name__ == "__main__":
    main()
