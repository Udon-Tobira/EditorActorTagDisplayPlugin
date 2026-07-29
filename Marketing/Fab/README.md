# Actor Metadata Overlay Fab media

This directory contains the six Fab listing images for Actor Metadata Overlay v2.0.0. The imagery is captured from the Unreal Engine 5.8 Level Editor using the acquired Deep Water Station demo map as screenshot background. The background environment asset and its demo scene are not included with the plugin, Upload ZIP, or Source ZIP.

## Product story

Actor Metadata Overlay keeps actor labels, tags, Gameplay Tags, Data Layers, direct properties, rule matches, distance filtering, and optional bounds visible in the viewport. The six images communicate the primary workflow in upload order:

1. `final/01-actor-metadata-overlay-hero.jpg` — `ACTOR METADATA OVERLAY`; a thumbnail-first full-bleed Deep Water Station viewport showing one prominent real Actor Metadata Overlay. The Hero is generated from one real UE capture and shows only the product name and the real overlay.
2. `final/02-before-after.jpg` — `METADATA, WITHOUT THE PANEL HUNT`; the same map, camera, viewport, lighting, selection, and Game View-off state before and after changing only Display Mode from `Off` to `All Matching Actors`.
3. `final/03-rule-settings.jpg` — `RULES THAT MATCH YOUR WORKFLOW`; the real **Project Settings > Plugins > Actor Metadata Overlay** page with rule fields such as Actor Class, Required/Excluded Actor Tags, Display Template, Display Color, Max Draw Distance, and Draw Bounding Box.
4. `final/04-selected-mode.jpg` — `FOCUS ON WHAT YOU SELECT`; one selected actor with the exact two-line tagline `Selected Actors mode` / `keeps the viewport quiet.`.
5. `final/05-distance-and-bounds.jpg` — `SEE WHAT MATTERS. HIDE THE REST.`; near/far visibility and a per-rule zone bound in the finished demo scene.
6. `final/06-template-tokens.jpg` — `SHOW THE DATA YOU NEED`; a real overlay containing actor, class, actor tags, Gameplay Tags, Data Layers, state, and priority beside the matching template token card.

The Hero does not contain `Demo scene not included.`. Gallery images 02–06 include that disclosure because the Deep Water Station demo scene is not included with the product. No marketplace badges, prices, arrows, fabricated Unreal UI, or AI-generated imagery are used.

## Generate and validate

From the repository root, after creating the temporary UE capture host and raw captures:

```powershell
python Marketing/Fab/source/generate-fab-media.py
```

The JSON-driven generator reads `source/media-layout.json`, validates the seven actual raw UE captures in `.verification/fab-media-v4/raw/` at least 2560x1440, requires the Preview check, the Before/After validation, and the Gameplay Tags validation, then writes exactly six 1920x1080 RGB JPEGs to `final/` and the three review images to `review/`. The Hero uses the `HeroFullBleed` layout, the fixed `heroFullBleedC` crop, and one real `hero-all.png` capture; it never draws overlay content onto a background. To regenerate only the approved Hero while preserving gallery finals 02–06, use:

```powershell
python Marketing/Fab/source/generate-fab-media.py --hero-only
```

Generating `final/06-template-tokens.jpg` requires `.verification/fab-media-v4/raw/all-tokens-validation.json` and `.verification/fab-media-v4/raw/all-tokens-overlay-proof.png`. The generator fails unless the record proves the required Gameplay Tag interface, Gameplay Tags, actor tags, Data Layers, state, priority, exact Gameplay line, raw capture name, and completed visual check.

## Upload order and packaging

Upload the six files under `final/` in the numbered order above. Use `final/01-actor-metadata-overlay-hero.jpg` as the Fab thumbnail. Do not upload `source/`, `review/`, raw captures, the temporary host project, or build artifacts.

The upload archive contains only the six final JPEGs at its root. The source archive contains the raw captures, validation JSON and diff evidence, generator, layout, manifest, finals, review images, and provenance while excluding the Deep Water Station source assets, Host Project, Binaries, Intermediate, Saved, DerivedDataCache, credentials, and Git data. The User Review archive additionally includes the Before/After captures, diff, and Gameplay Tags proof for visual inspection.

The Hero's dark gradient, product title, accent line, and full-bleed crop are deterministic presentation layers around one real Unreal Engine capture. Gallery cards and the token example are also deterministic presentation layers around real Unreal Engine captures. `Demo scene not included.` is a disclosure on gallery images 02–06, not a product feature claim.
