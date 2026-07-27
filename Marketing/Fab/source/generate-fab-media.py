from __future__ import annotations

import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageColor, ImageDraw, ImageFont


REPO_ROOT = Path(__file__).resolve().parents[3]
FAB_ROOT = REPO_ROOT / "Marketing" / "Fab"
FINAL_ROOT = FAB_ROOT / "final"
REVIEW_ROOT = FAB_ROOT / "review"
RAW_ROOT = REPO_ROOT / ".verification" / "fab-media" / "raw"
LAYOUT_PATH = FAB_ROOT / "source" / "media-layout.json"
SOURCE_HEAD_PATH = REPO_ROOT / ".verification" / "fab-media" / "source-head.txt"
VALIDATION_PATH = RAW_ROOT / "all-tokens-validation.json"
MAX_JPEG_BYTES = 2_800_000
MAX_TOTAL_BYTES = 20_000_000


MEDIA = [
    {
        "order": 1,
        "filename": "01-actor-metadata-overlay-hero.jpg",
        "role": "thumbnail",
        "layout": "B",
        "source": "hero-all.png",
        "crop": "viewport",
        "title": "ACTOR METADATA\nOVERLAY",
        "tagline": "See the data. Stay in the viewport.",
        "proof": "RULE-BASED EDITOR OVERLAYS",
    },
    {
        "order": 2,
        "filename": "02-before-after.jpg",
        "role": "gallery",
        "layout": "C",
        "sources": ["before.png", "after.png"],
        "crops": ["viewport", "viewport"],
        "title": "METADATA, WITHOUT\nTHE PANEL HUNT",
        "tagline": "The same level. The context is finally visible.",
        "proof": "BEFORE/AFTER",
    },
    {
        "order": 3,
        "filename": "03-rule-settings.jpg",
        "role": "gallery",
        "layout": "A",
        "source": "project-settings.png",
        "crop": "settings",
        "title": "RULES THAT MATCH\nYOUR WORKFLOW",
        "tagline": "Classes, tags, colors, distance and templates.",
        "proof": "FIRST MATCH WINS",
    },
    {
        "order": 4,
        "filename": "04-selected-mode.jpg",
        "role": "gallery",
        "layout": "B",
        "source": "selected.png",
        "crop": "scaledViewport",
        "title": "FOCUS ON WHAT\nYOU SELECT",
        "tagline": "Selected Actors mode keeps the viewport quiet.",
        "proof": "SELECTED / ALL / OFF",
    },
    {
        "order": 5,
        "filename": "05-distance-and-bounds.jpg",
        "role": "gallery",
        "layout": "A",
        "source": "distance-bounds.png",
        "crop": "viewport",
        "title": "SEE WHAT MATTERS.\nHIDE THE REST.",
        "tagline": "Distance filtering and optional bounds for dense levels.",
        "proof": "PER-RULE CONTROL",
    },
    {
        "order": 6,
        "filename": "06-template-tokens.jpg",
        "role": "gallery",
        "layout": "AToken",
        "source": "all-tokens.png",
        "crop": "scaledViewport",
        "title": "SHOW THE DATA\nYOU NEED",
        "tagline": "Labels, tags, layers and safe direct properties.",
        "proof": "FLEXIBLE TEMPLATES",
    },
]


def load_layout() -> dict:
    if not LAYOUT_PATH.is_file():
        raise FileNotFoundError(f"Missing layout JSON: {LAYOUT_PATH}")
    with LAYOUT_PATH.open(encoding="utf-8") as stream:
        return json.load(stream)


LAYOUT = load_layout()
CANVAS_SIZE = (LAYOUT["canvas"]["width"], LAYOUT["canvas"]["height"])
COLORS = LAYOUT["colors"]
FONTS = LAYOUT["fonts"]
SIZES = LAYOUT["fontSizes"]
CARD_RADIUS = LAYOUT["cards"]["radius"]
CARD_BORDER = LAYOUT["cards"]["borderWidth"]


def color(name: str) -> str:
    return COLORS[name]


def font(name: str, size_name: str) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(FONTS[name], size=SIZES[size_name])


def draw_gradient(image: Image.Image, box: tuple[int, int, int, int]) -> None:
    """Draw the Layout B dark readability gradient as a separate operation."""
    x, y, width, height = box
    overlay = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    pixels = overlay.load()
    background = ImageColor.getrgb(color("background"))
    for px in range(width):
        strength = max(0.0, 1.0 - (px / max(1, width)) ** 0.72)
        alpha = round(214 * strength)
        for py in range(height):
            pixels[px, py] = (*background, alpha)
    image.paste(overlay, (x, y), overlay)


def rounded_border_shadow(
    image: Image.Image,
    box: tuple[int, int, int, int],
    border_color: str,
) -> None:
    """Draw the shared rounded card shadow, fill, and border."""
    x, y, width, height = box
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (x + 10, y + 12, x + width + 10, y + height + 12),
        radius=CARD_RADIUS,
        fill=color("cardShadow"),
    )
    draw.rounded_rectangle(
        (x, y, x + width, y + height),
        radius=CARD_RADIUS,
        fill=color("card"),
        outline=border_color,
        width=CARD_BORDER,
    )


def fit_cover(source: Image.Image, size: tuple[int, int]) -> Image.Image:
    target_width, target_height = size
    scale = max(target_width / source.width, target_height / source.height)
    resized = source.resize(
        (round(source.width * scale), round(source.height * scale)),
        Image.Resampling.LANCZOS,
    )
    left = max(0, (resized.width - target_width) // 2)
    top = max(0, (resized.height - target_height) // 2)
    return resized.crop((left, top, left + target_width, top + target_height))


def screenshot_card(
    image: Image.Image,
    screenshot: Image.Image,
    box: tuple[int, int, int, int],
    border_color: str,
) -> None:
    """Place a real capture inside a rounded screenshot card."""
    rounded_border_shadow(image, box, border_color)
    x, y, width, height = box
    clipped = fit_cover(screenshot, (width - CARD_BORDER * 2, height - CARD_BORDER * 2))
    mask = Image.new("L", clipped.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, clipped.width - 1, clipped.height - 1),
        radius=max(0, CARD_RADIUS - CARD_BORDER),
        fill=255,
    )
    image.paste(clipped, (x + CARD_BORDER, y + CARD_BORDER), mask)


def base_canvas() -> Image.Image:
    image = Image.new("RGB", CANVAS_SIZE, color("background"))
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, CANVAS_SIZE[0], CANVAS_SIZE[1] // 2), fill=color("background"))
    draw.rectangle(
        (0, CANVAS_SIZE[1] // 2, CANVAS_SIZE[0], CANVAS_SIZE[1]),
        fill=color("background2"),
    )
    return image


def text_width(draw: ImageDraw.ImageDraw, value: str, used_font: ImageFont.FreeTypeFont) -> int:
    return draw.textbbox((0, 0), value, font=used_font)[2]


def wrap_text(
    draw: ImageDraw.ImageDraw,
    value: str,
    used_font: ImageFont.FreeTypeFont,
    max_width: int,
    max_lines: int,
) -> list[str]:
    """Wrap text within the JSON-defined safe box while preserving explicit lines."""
    result: list[str] = []
    for paragraph in value.splitlines() or [""]:
        words = paragraph.split()
        if not words:
            result.append("")
            continue
        current = words[0]
        for word in words[1:]:
            candidate = f"{current} {word}"
            if text_width(draw, candidate, used_font) <= max_width:
                current = candidate
            else:
                result.append(current)
                current = word
        result.append(current)
    if len(result) > max_lines:
        raise ValueError(f"Text wrapped to {len(result)} lines, limit is {max_lines}: {value!r}")
    return result


def draw_title(
    draw: ImageDraw.ImageDraw,
    title: str,
    box: tuple[int, int, int, int],
    title_size_name: str,
) -> int:
    x, y, width, _ = box
    used_font = font("bold", title_size_name)
    lines = wrap_text(draw, title, used_font, width, 2)
    line_height = SIZES[title_size_name] + 7
    for index, line in enumerate(lines):
        draw.text((x, y + index * line_height), line, font=used_font, fill=color("text"))
    return y + len(lines) * line_height


def draw_tagline(
    draw: ImageDraw.ImageDraw,
    tagline: str,
    box: tuple[int, int, int, int],
    start_y: int,
) -> int:
    x, _, width, _ = box
    used_font = font("regular", "tagline")
    lines = wrap_text(draw, tagline, used_font, width, 2)
    line_height = SIZES["tagline"] + 8
    for index, line in enumerate(lines):
        draw.text(
            (x, start_y + index * line_height),
            line,
            font=used_font,
            fill=color("secondary"),
        )
    return start_y + len(lines) * line_height


def draw_proof(
    draw: ImageDraw.ImageDraw,
    proof: str,
    x: int,
    y: int,
    max_width: int,
    border_color: str,
) -> None:
    used_font = font("bold", "proof")
    text_padding = 22
    width = min(max_width, text_width(draw, proof, used_font) + text_padding * 2)
    height = SIZES["proof"] + 28
    draw.rounded_rectangle(
        (x, y, x + width, y + height),
        radius=18,
        fill=color("background2"),
        outline=border_color,
        width=CARD_BORDER,
    )
    draw.text((x + text_padding, y + 12), proof, font=used_font, fill=border_color)


def draw_brand_disclaimer(draw: ImageDraw.ImageDraw) -> None:
    margin = LAYOUT["canvas"]["safeMargin"]
    draw.text(
        (margin, CANVAS_SIZE[1] - margin - SIZES["disclaimer"]),
        "Demo scene not included.",
        font=font("regular", "disclaimer"),
        fill=color("secondary"),
    )
    brand = "metyatech"
    brand_width = text_width(draw, brand, font("bold", "brand"))
    draw.text(
        (CANVAS_SIZE[0] - margin - brand_width, CANVAS_SIZE[1] - margin - SIZES["brand"]),
        brand,
        font=font("bold", "brand"),
        fill=color("text"),
    )


def draw_code_card(
    image: Image.Image,
    box: tuple[int, int, int, int],
    lines: Iterable[str],
) -> None:
    rounded_border_shadow(image, box, color("accent"))
    draw = ImageDraw.Draw(image)
    x, y, width, _ = box
    draw.text((x + 30, y + 30), "TEMPLATE TOKENS", font=font("bold", "proof"), fill=color("accent"))
    line_y = y + 94
    line_height = SIZES["code"] + 25
    for line in lines:
        draw.text((x + 30, line_y), line, font=font("mono", "code"), fill=color("text"))
        line_y += line_height


def load_source(name: str, crop_name: str) -> Image.Image:
    path = RAW_ROOT / name
    if not path.is_file():
        raise FileNotFoundError(f"Missing real UE capture: {path}")
    with Image.open(path) as source:
        minimum_width, minimum_height = 2560, 1440
        if source.width < minimum_width or source.height < minimum_height:
            raise ValueError(f"Raw capture is below 2560x1440: {path} -> {source.size}")
        source_image = source.convert("RGB")
    crop = tuple(LAYOUT["sources"][crop_name]["crop"])
    if crop_name == "settings":
        return source_image.crop(crop)
    return source_image.crop(crop)


def layout_a(spec: dict, screenshot: Image.Image) -> Image.Image:
    image = base_canvas()
    draw = ImageDraw.Draw(image)
    layout = LAYOUT["layouts"]["A"]
    text_box = tuple(layout["text"])
    text_end = draw_title(draw, spec["title"], text_box, "galleryTitle")
    text_end = draw_tagline(draw, spec["tagline"], text_box, text_end + 28)
    draw_proof(draw, spec["proof"], text_box[0], layout["proofY"], text_box[2], color("accentGreen"))
    screenshot_card(image, screenshot, tuple(layout["screenshot"]), color("accent"))
    draw_brand_disclaimer(draw)
    return image


def layout_b(spec: dict, screenshot: Image.Image) -> Image.Image:
    image = base_canvas()
    screenshot_box = tuple(LAYOUT["layouts"]["B"]["screenshot"])
    screenshot_card(image, screenshot, screenshot_box, color("accent"))
    draw_gradient(image, tuple(LAYOUT["layouts"]["B"]["gradient"]))
    draw = ImageDraw.Draw(image)
    text_box = tuple(LAYOUT["layouts"]["B"]["text"])
    text_end = draw_title(draw, spec["title"], text_box, "heroTitle" if spec["order"] == 1 else "galleryTitle")
    draw_tagline(draw, spec["tagline"], text_box, text_end + 30)
    draw_proof(draw, spec["proof"], text_box[0], LAYOUT["layouts"]["B"]["proofY"], text_box[2], color("accent"))
    draw_brand_disclaimer(draw)
    return image


def layout_c(spec: dict, before: Image.Image, after: Image.Image) -> Image.Image:
    image = base_canvas()
    draw = ImageDraw.Draw(image)
    text_box = tuple(LAYOUT["layouts"]["C"]["text"])
    text_end = draw_title(draw, spec["title"], text_box, "galleryTitle")
    draw_tagline(draw, spec["tagline"], text_box, text_end + 16)
    before_box = tuple(LAYOUT["layouts"]["C"]["before"])
    after_box = tuple(LAYOUT["layouts"]["C"]["after"])
    screenshot_card(image, before, before_box, color("secondary"))
    screenshot_card(image, after, after_box, color("accent"))
    divider_x = LAYOUT["layouts"]["C"]["dividerX"]
    draw.line((divider_x, before_box[1], divider_x, before_box[1] + before_box[3]), fill=color("text"), width=CARD_BORDER)
    label_font = font("bold", "beforeAfter")
    draw.text((before_box[0] + 24, before_box[1] + 22), "BEFORE", font=label_font, fill=color("text"))
    draw.text((after_box[0] + 24, after_box[1] + 22), "AFTER", font=label_font, fill=color("accent"))
    draw_brand_disclaimer(draw)
    return image


def layout_a_tokens(spec: dict, screenshot: Image.Image) -> Image.Image:
    image = base_canvas()
    draw = ImageDraw.Draw(image)
    layout = LAYOUT["layouts"]["AToken"]
    text_box = tuple(layout["text"])
    text_end = draw_title(draw, spec["title"], text_box, "galleryTitle")
    draw_tagline(draw, spec["tagline"], text_box, text_end + 16)
    screenshot_card(image, screenshot, tuple(layout["screenshot"]), color("accent"))
    draw_code_card(
        image,
        tuple(layout["code"]),
        [
            "{ActorLabel}",
            "Class: {ActorClass}",
            "Tags: {ActorTags}",
            "Gameplay: {GameplayTags}",
            "Layer: {DataLayers}",
            "Priority: {Property:Priority}",
        ],
    )
    draw_proof(draw, spec["proof"], text_box[0], layout["proofY"], text_box[2], color("accent"))
    draw_brand_disclaimer(draw)
    return image


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def save_jpeg(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    quality = 92
    while True:
        image.convert("RGB").save(
            path,
            format="JPEG",
            quality=quality,
            optimize=True,
            subsampling=0,
        )
        if path.stat().st_size <= MAX_JPEG_BYTES:
            return
        if quality <= 84:
            raise RuntimeError(f"JPEG remains above 2.8 MB at quality 84: {path}")
        quality -= 2


def make_review_outputs(finals: list[Path]) -> None:
    preview_size = (LAYOUT["previews"]["width"], LAYOUT["previews"]["height"])
    REVIEW_ROOT.mkdir(parents=True, exist_ok=True)
    previews: list[tuple[Path, Image.Image]] = []
    for final in finals:
        with Image.open(final) as source:
            previews.append((final, source.convert("RGB").resize(preview_size, Image.Resampling.LANCZOS)))

    save_jpeg(previews[0][1], REVIEW_ROOT / "thumbnail-preview-320x180.jpg")
    gallery = Image.new("RGB", (preview_size[0] * 3, preview_size[1] * 2), color("background"))
    for index, (_, preview) in enumerate(previews):
        gallery.paste(preview, ((index % 3) * preview_size[0], (index // 3) * preview_size[1]))
    save_jpeg(gallery, REVIEW_ROOT / "gallery-preview-320x180.jpg")

    cell_width = CANVAS_SIZE[0] // LAYOUT["previews"]["contactColumns"]
    cell_height = LAYOUT["previews"]["contactCellHeight"]
    contact = Image.new(
        "RGB",
        (cell_width * LAYOUT["previews"]["contactColumns"], cell_height * LAYOUT["previews"]["contactRows"]),
        color("background"),
    )
    contact_draw = ImageDraw.Draw(contact)
    label_font = font("regular", "disclaimer")
    for index, (final, preview) in enumerate(previews):
        cell_x = (index % 3) * cell_width
        cell_y = (index // 3) * cell_height
        thumb = fit_cover(preview, (cell_width - 32, cell_height - 48))
        contact.paste(thumb, (cell_x + 16, cell_y + 8))
        contact_draw.text(
            (cell_x + 16, cell_y + cell_height - 32),
            final.name,
            font=label_font,
            fill=color("text"),
        )
    save_jpeg(contact, REVIEW_ROOT / "contact-sheet.jpg")


def parse_source_head() -> tuple[str, str]:
    if not SOURCE_HEAD_PATH.is_file():
        raise FileNotFoundError(f"Missing capture provenance: {SOURCE_HEAD_PATH}")
    values: dict[str, str] = {}
    for line in SOURCE_HEAD_PATH.read_text(encoding="utf-8").splitlines():
        if ": " in line:
            key, value = line.split(": ", 1)
            values[key] = value
    return values["Plugin HEAD"], ".verification/fab-media/host/ActorMetadataOverlaySmoke.uproject"


def validate_all_tokens_capture() -> None:
    if not VALIDATION_PATH.is_file():
        raise FileNotFoundError(f"Missing host validation JSON: {VALIDATION_PATH}")
    with VALIDATION_PATH.open(encoding="utf-8") as stream:
        validation = json.load(stream)
    if validation.get("implementsGameplayTagAssetInterface") is not True:
        raise ValueError("Host validation does not prove IGameplayTagAssetInterface")
    if sorted(validation.get("ownedGameplayTags", [])) != ["Loot.Rare", "World.Interactable"]:
        raise ValueError("Host validation Gameplay Tags do not match the required pair")
    if sorted(validation.get("actorTags", [])) != ["Inspectable", "Loot"]:
        raise ValueError("Host validation actor tags do not match the required pair")
    if sorted(validation.get("dataLayers", [])) != ["Gameplay", "Night"]:
        raise ValueError("Host validation Data Layers do not match the required pair")
    if validation.get("state") != "Ready":
        raise ValueError("Host validation state is not Ready")
    if validation.get("priority") != 80:
        raise ValueError("Host validation priority is not 80")
    if validation.get("expectedGameplayLine") != "Gameplay: Loot.Rare, World.Interactable":
        raise ValueError("Host validation Gameplay line is not exact")
    if validation.get("visualCheck") is not True:
        raise ValueError("Host validation visualCheck must be true after visual inspection")
    if validation.get("rawCapture") != "all-tokens.png":
        raise ValueError("Host validation rawCapture must be all-tokens.png")


def write_manifest(finals: list[Path], source_head: str, capture_project: str) -> None:
    metadata = {item["filename"]: item for item in MEDIA}
    entries = []
    for final in finals:
        item = metadata[final.name]
        with Image.open(final) as opened:
            entry = {
                "order": item["order"],
                "filename": final.name,
                "role": item["role"],
                "title": item["title"].replace("\n", " "),
                "tagline": item["tagline"],
                "proof": item["proof"],
                "width": opened.width,
                "height": opened.height,
                "format": "JPEG",
                "mode": opened.mode,
                "sizeBytes": final.stat().st_size,
                "sha256": sha256(final),
                "sourceCaptures": [
                    f".verification/fab-media/raw/{name}"
                    for name in item.get("sources", [item.get("source")])
                    if name
                ],
            }
            if item["order"] == 6:
                entry["validationEvidence"] = ".verification/fab-media/raw/all-tokens-validation.json"
            entries.append(entry)
    manifest = {
        "productName": "Actor Metadata Overlay",
        "productVersion": "2.0.0",
        "designSystemVersion": LAYOUT["designSystemVersion"],
        "thumbnail": "final/01-actor-metadata-overlay-hero.jpg",
        "canvasWidth": CANVAS_SIZE[0],
        "canvasHeight": CANVAS_SIZE[1],
        "accentColor": color("accent"),
        "backgroundColor": color("background"),
        "generatedAtUtc": datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "sourcePluginHead": source_head,
        "captureProject": capture_project,
        "captureEngine": "Unreal Engine 5.8 Level Editor viewport",
        "files": entries,
    }
    (FAB_ROOT / "media-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n",
        encoding="utf-8",
    )


def main() -> None:
    source_head, capture_project = parse_source_head()
    validate_all_tokens_capture()
    required_sources = sorted(
        {
            name
            for item in MEDIA
            for name in item.get("sources", [item.get("source")])
            if name
        }
    )
    for source in required_sources:
        source_path = RAW_ROOT / source
        if not source_path.is_file():
            raise FileNotFoundError(f"Missing required raw capture: {source_path}")
        with Image.open(source_path) as opened:
            if opened.width < 2560 or opened.height < 1440:
                raise ValueError(f"Raw capture below 2560x1440: {source_path} -> {opened.size}")

    FINAL_ROOT.mkdir(parents=True, exist_ok=True)
    expected_final_names = {item["filename"] for item in MEDIA}
    unexpected_existing = [
        path for path in FINAL_ROOT.iterdir()
        if path.is_file() and path.name not in expected_final_names
    ]
    if unexpected_existing:
        raise RuntimeError(f"final/ contains unexpected files before generation: {unexpected_existing}")

    for spec in MEDIA:
        if spec["layout"] == "C":
            before = load_source(spec["sources"][0], spec["crops"][0])
            after = load_source(spec["sources"][1], spec["crops"][1])
            rendered = layout_c(spec, before, after)
        else:
            screenshot = load_source(spec["source"], spec["crop"])
            if spec["layout"] == "A":
                rendered = layout_a(spec, screenshot)
            elif spec["layout"] == "B":
                rendered = layout_b(spec, screenshot)
            elif spec["layout"] == "AToken":
                rendered = layout_a_tokens(spec, screenshot)
            else:
                raise ValueError(f"Unknown layout: {spec['layout']}")
        save_jpeg(rendered, FINAL_ROOT / spec["filename"])

    finals = [FINAL_ROOT / item["filename"] for item in MEDIA]
    actual_final_names = sorted(path.name for path in FINAL_ROOT.glob("*.jpg"))
    expected_final_names = sorted(expected_final_names)
    if actual_final_names != expected_final_names:
        raise RuntimeError(f"final/ contains unexpected files: {actual_final_names}")
    total_bytes = sum(path.stat().st_size for path in finals)
    if total_bytes >= MAX_TOTAL_BYTES:
        raise RuntimeError(f"Final image total is >= 20 MB: {total_bytes}")
    make_review_outputs(finals)
    write_manifest(finals, source_head, capture_project)
    print(json.dumps({"finals": [path.name for path in finals], "totalBytes": total_bytes}, indent=2))


if __name__ == "__main__":
    main()
