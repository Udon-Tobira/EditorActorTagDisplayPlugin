# Actor Metadata Overlay

## Overview

Actor Metadata Overlay is an Unreal Editor-only viewport tool for keeping useful actor metadata visible while laying out and debugging a level. Configurable rules display actor labels, classes, Actor Tags, Gameplay Tags, folders, data layers, and selected public properties directly over actors.

## Features

- Event-driven metadata cache with direct per-viewport canvas drawing.
- First-match rule evaluation with actor class and tag filters.
- Selected Actors, All Matching Actors, and Off display modes.
- Fixed metadata tokens and safe direct-property tokens.
- Per-rule colors, world offsets, distance limits, selected-only filters, and optional bounding boxes.
- Separate project rules and per-user editor display preferences.

## Supported Versions

- Unreal Engine 5.6
- Unreal Engine 5.7
- Unreal Engine 5.8
- Win64
- Editor only

## Installation

### Fab

Download the ZIP for your Unreal Engine version from Fab and place the single `ActorMetadataOverlay` plugin folder in the project's `Plugins` directory. Enable the plugin from **Edit > Plugins**, then restart the editor when prompted.

### Project Plugins folder

Copy the `ActorMetadataOverlay` plugin folder into `<YourProject>/Plugins/`. Open the project, enable **Actor Metadata Overlay**, and restart the editor.

## Public sample project

The free [Actor Metadata Overlay Sample](https://github.com/metyatech/ActorMetadataOverlaySample) contains a UE 5.6 source-of-truth demo map, fixture actors, Gameplay Tags, Data Layers, and automation coverage for validating the plugin without redistributing the paid plugin package.

## Quick Start

1. Open **Project Settings > Plugins > Actor Metadata Overlay**.
2. Review the default rule or add a rule for a specific actor class.
3. Add the actor tags and template fields required by your workflow.
4. In the level viewport, open **Viewport Show > Actor Metadata Overlay > Selected Actors**.
5. Select an actor to see its metadata overlaid in the viewport.

## Display Modes

- **Off** hides the overlay.
- **Selected Actors** displays cached matching actors that are selected.
- **All Matching Actors** displays every cached actor that matches a rule, subject to rule and global distance limits.

The overlay is drawn in normal Level Editor Viewports, including Perspective, Top, Front, Side, and split Level Editor layouts. It remains visible when **Game View** is off. It is hidden when **Game View** is on, during PIE or SIE game screens, and in Static Mesh Editor or other Asset Preview Viewports.

## Rule Evaluation

Rules are evaluated from top to bottom. The first enabled rule whose class and tag filters match an actor is used. An actor never receives multiple overlays. A blank or unresolved actor class does not match.

## Template Tokens

Fixed tokens are `{ActorLabel}`, `{ActorName}`, `{ActorClass}`, `{ActorTags}`, `{GameplayTags}`, `{Folder}`, and `{DataLayers}`. Property values use `{Property:PropertyName}` and are limited to direct, public, editable or Blueprint-visible properties. See [SettingsReference.md](Documentation/SettingsReference.md) for formatting and safety limits.

## Project Settings

Project rules, the default template, outline color, and the maximum property value length are configured at **Project Settings > Plugins > Actor Metadata Overlay**.

## Editor Preferences

Per-user display mode, global distance, text scale, outline visibility, and bounding-box visibility are configured at **Editor Preferences > Plugins > Actor Metadata Overlay**.

## Performance

The plugin does not scan every actor every frame. It rebuilds an event-driven cache when the level or rules change, refreshes individual actors on actor events, and traverses only matching cached actors during viewport drawing.

Canonical documentation: <https://metyatech.github.io/unreal-plugin-docs/actor-metadata-overlay/>.

## Limitations

- The plugin does not edit Actor Tags.
- Gameplay Tags are shown only for actors implementing `IGameplayTagAssetInterface`.
- Property tokens support direct public properties only; they do not evaluate functions or nested paths.
- The plugin is Editor only and supports Win64.

## Troubleshooting

- Confirm the plugin is enabled and restart the editor after installation.
- Confirm the rule is enabled, its actor class is resolved, and any required or excluded tags are correct.
- Confirm the viewport mode is **Selected Actors** or **All Matching Actors** rather than **Off**.
- Check global and per-rule distance limits, actor visibility, and the [settings reference](Documentation/SettingsReference.md).

## Support

Open an issue at <https://github.com/metyatech/ActorMetadataOverlayPlugin/issues>.

## License

Products obtained from Fab are subject to the Fab Standard License. This public repository does not grant a separate redistribution license for the Fab product.

Central plugin documentation: <https://metyatech.github.io/unreal-plugin-docs/actor-metadata-overlay/>.
