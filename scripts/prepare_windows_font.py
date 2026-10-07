"""Build an OFL-licensed desktop font from pinned Noto CJK source and product text.

Only source/include and the shipped data are read; research and generated build trees
are not inputs. CI regenerates this subset whenever product text changes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request

import fontTools
from fontTools import subset
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
VERSION = "4.59.0"
SOURCE_BASE = "https://raw.githubusercontent.com/notofonts/noto-cjk/Sans2.004/"
FONT_PATH = "Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"
FONT_SHA = "2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b"
LICENSE_SHA = "6a73f9541c2de74158c0e7cf6b0a58ef774f5a780bf191f2d7ec9cc53efe2bf2"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def source(local, relative, expected):
    if local:
        data = Path(local).read_bytes()
    else:
        request = urllib.request.Request(SOURCE_BASE + relative, headers={"User-Agent": "Ark-Village-font-build"})
        with urllib.request.urlopen(request, timeout=30) as response:
            data = response.read()
    if digest(data) != expected:
        raise ValueError(f"Pinned Noto source hash mismatch: {relative}")
    return data


def product_characters(inventory_path):
    # The shared build and the runtime atlas use this exact Node authority. Refresh
    # its one common output even when CI prepares the font before CMake configures.
    inventory_path = inventory_path.resolve()
    subprocess.run([
        "node", str(ROOT / "scripts" / "compile_desktop_glyphs.mjs"),
        "--root", str(ROOT), "--json", str(inventory_path),
        "--header", str(inventory_path.with_name("desktop_glyphs.hpp")),
    ], check=True)
    inventory_bytes = inventory_path.read_bytes()
    inventory = json.loads(inventory_bytes)
    points = inventory["codepoints"]
    if (inventory.get("schema") != 1 or
            any(type(code) is not int or code < 32 or code == 127 or code > 0x10FFFF or
                0xD800 <= code <= 0xDFFF for code in points) or
            points != sorted(set(points)) or not set(range(32, 127)).issubset(points)):
        raise ValueError("Invalid product Unicode inventory")
    return set(points), inventory["inputs"], digest(inventory_bytes)


def rename(font):
    # A subset is a derivative, so use a distinct family/PostScript name under OFL.
    family = "Ark Village CJK Subset"
    names = {1: family, 2: "Regular", 3: "ArkVillageCJKSubset-2.004", 4: family,
             6: "ArkVillageCJKSubset-Regular", 16: family, 17: "Regular"}
    for record in font["name"].names:
        if record.nameID in names:
            record.string = names[record.nameID].encode(record.getEncoding())
    cff = font["CFF "].cff
    cff.fontNames = [names[6]]
    top = cff.topDictIndex[0]
    top.FullName, top.FamilyName = family, family
    for index, item in enumerate(top.FDArray):
        item.FontName = f"ArkVillageCJKSubset-FD{index}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--source-font", type=Path, help="Optional already-downloaded pinned OTF")
    parser.add_argument("--source-license", type=Path, help="Optional already-downloaded pinned OFL")
    parser.add_argument("--glyph-inventory", type=Path,
                        default=ROOT / "build" / "shared-libraries" / "desktop-generated" / "desktop_glyphs.json",
                        help="Common glyph inventory output; Node regenerates it and its private header")
    args = parser.parse_args()
    if fontTools.__version__ != VERSION:
        raise RuntimeError(f"Use fonttools=={VERSION}; found {fontTools.__version__}")
    font_data = source(args.source_font, FONT_PATH, FONT_SHA)
    license_data = source(args.source_license, "LICENSE", LICENSE_SHA)
    points, inputs, inventory_sha = product_characters(args.glyph_inventory)
    from io import BytesIO
    font = TTFont(BytesIO(font_data), recalcTimestamp=False)
    missing = points - set(font.getBestCmap())
    if missing:
        raise ValueError("Noto lacks requested glyphs: " + " ".join(f"U+{code:04X}" for code in sorted(missing)))
    options = subset.Options()
    options.recalc_timestamp = False
    options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14, 16, 17]
    options.name_languages = [0x409]
    options.layout_features = []  # raylib renders individual cmap glyphs, without shaping.
    worker = subset.Subsetter(options=options)
    worker.populate(unicodes=sorted(points))
    worker.subset(font)
    rename(font)
    output = BytesIO()
    font.save(output)
    packed = output.getvalue()
    with TTFont(BytesIO(packed)) as verified:
        if points - set(verified.getBestCmap()):
            raise ValueError("Subset lost required product glyphs")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "default.otf").write_bytes(packed)
    (args.output_dir / "OFL.txt").write_bytes(license_data)
    manifest = {
        "source": "Noto Sans CJK SC Regular 2.004", "license": "SIL Open Font License 1.1",
        "font_url": SOURCE_BASE + FONT_PATH, "font_sha256": FONT_SHA,
        "license_url": SOURCE_BASE + "LICENSE", "license_sha256": LICENSE_SHA,
        "tool": f"fonttools=={VERSION}", "family": "Ark Village CJK Subset",
        "output_sha256": digest(packed), "source_bytes": len(font_data), "output_bytes": len(packed),
        "codepoints": [f"U+{code:04X}" for code in sorted(points)], "inputs": inputs,
        "glyph_inventory_sha256": inventory_sha,
        "glyph_generator": "scripts/compile_desktop_glyphs.mjs",
        "glyph_generator_sha256": digest((ROOT / "scripts" / "compile_desktop_glyphs.mjs").read_bytes()),
    }
    (args.output_dir / "SOURCES.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {len(points)} product glyphs; font {len(font_data)} -> {len(packed)} bytes; sha256={digest(packed)}")


if __name__ == "__main__":
    main()
