# Actor Metadata Overlay Fab media

This directory contains the six Fab listing images for Actor Metadata Overlay v2.0.0. The images show real Unreal Engine 5.8 Level Editor captures composed into a consistent 1920x1080 Fab presentation. The temporary demonstration scene is not included with the plugin or the upload set.

## Product story

Actor Metadata Overlay keeps actor labels, tags, gameplay tags, Data Layers, direct properties, rule matches, distance filtering, and optional bounds visible in the viewport. The six images communicate the primary workflow in upload order:

1. `final/01-actor-metadata-overlay-hero.jpg` — `ACTOR METADATA OVERLAY`; hero thumbnail proving rule-based editor overlays in the viewport.
2. `final/02-before-after.jpg` — `METADATA, WITHOUT THE PANEL HUNT`; the same camera before and after the overlay is enabled.
3. `final/03-rule-settings.jpg` — `RULES THAT MATCH YOUR WORKFLOW`; real Project Settings proof with `FIRST MATCH WINS` and the important rule controls.
4. `final/04-selected-mode.jpg` — `FOCUS ON WHAT YOU SELECT`; one selected actor with the `SELECTED / ALL / OFF` display-mode proof.
5. `final/05-distance-and-bounds.jpg` — `SEE WHAT MATTERS. HIDE THE REST.`; near/far visibility and per-rule zone bounds.
6. `final/06-template-tokens.jpg` — `SHOW THE DATA YOU NEED`; a real overlay containing actor, class, tag, gameplay-tag, Data Layer, state, and priority data beside the exact template token card.

Every image includes the `metyatech` brand, the disclaimer `Demo scene not included.`, a dark 28 px rounded presentation card, and no engine, marketplace, price, badge, arrow, or fabricated product UI imagery.

## Generate and validate

From the repository root, after recreating the temporary UE capture host and raw captures:

```powershell
python Marketing/Fab/source/generate-fab-media.py
```

The JSON-driven generator reads `source/media-layout.json`, validates the seven real raw captures at least 2560x1440, writes exactly six 1920x1080 JPEGs to `final/`, writes the three review images to `review/`, and regenerates `media-manifest.json`. JPEG output starts at quality 92 with optimization and 4:4:4 subsampling; quality is lowered by 2 only when necessary, down to 84, to keep each final image under 2.8 MB. The six-image total must remain under 20 MB.

Review outputs are:

- `review/contact-sheet.jpg` — all six finals with filenames shown below each image.
- `review/thumbnail-preview-320x180.jpg` — the exact hero thumbnail result at 320x180.
- `review/gallery-preview-320x180.jpg` — the six gallery results at 320x180 each.

## Upload order and packaging

Upload the six files under `final/` in the numbered order above. Use `final/01-actor-metadata-overlay-hero.jpg` as the Fab thumbnail. Do not upload `source/`, `review/`, raw captures, the temporary host project, or build artifacts.

The generated upload archive is `.verification/fab-media/artifacts/ActorMetadataOverlay-Fab-media-upload-<UTC timestamp>.zip`; its root contains exactly the six JPEG files and no folder entry. The source archive is `.verification/fab-media/artifacts/ActorMetadataOverlay-Fab-media-source-<UTC timestamp>.zip`; it contains the raw capture archive, generator, layout, manifest, finals, review evidence, source provenance, and capture notes, while excluding the host project and build/runtime caches.

The raw source archive records the actual UE 5.8 Open World capture project, capture date, plugin source HEAD, and capture paths. No AI-generated imagery or fabricated Unreal UI is used. The outer labels, cards, gradient, and token example are deterministic presentation layers around the real captures.
