# Fab Submission

## Listing

- Title: `Actor Metadata Overlay`
- Category: `Tools & Plugins`
- Subcategory: `Engine Tools`
- Suggested tags: Editor, Tool, Plugin, Actor, Metadata, Debugging, Level, Workflow

## Suggested pricing

- Personal: `$9.99`
- Professional: `$19.99`

These are suggested prices only. Pricing must be entered manually in the publisher portal; the code does not set or automate pricing.

## Short description

> Display actor labels, classes, tags, gameplay tags, folders, data layers, and selected property values directly in the Unreal Editor viewport using configurable rules.

## Long description

Level designers and technical artists often move back and forth between the Details panel, the Outliner, and the viewport just to confirm an actor's metadata. Actor Metadata Overlay puts the information needed for placement and debugging directly above the actor in the Unreal Editor viewport.

Rules choose actors by class and Actor Tags, then render a configurable template with labels, classes, Actor Tags, Gameplay Tags, folders, data layers, and selected property values. The overlay supports Selected Actors, All Matching Actors, and Off modes, per-rule templates and colors, distance limits, optional bounding boxes, and an event-driven cache that avoids a full actor scan every frame.

The product is Editor only and supports Win64. It is a viewport metadata display tool, not an Actor Tags editing tool: it does not add, remove, or edit Actor Tags, and it does not add Outliner columns or a search interface.

For a free public demonstration and validation project, link to the [Actor Metadata Overlay Sample](https://github.com/metyatech/ActorMetadataOverlaySample). The sample provides the UE 5.6 overview map and fixture plugin while keeping the paid plugin package separate.

## Media checklist

Prepare at least these six screenshots:

1. Before / After viewport comparison.
2. Project rule settings.
3. Selected Actors mode.
4. All Matching Actors with distance filtering.
5. A property and Gameplay Tags template.
6. Bounding box option enabled.

Every image should be at least 1920×1080, have readable UI, contain no misleading product or engine branding outside the demonstrated workflow, and avoid duplicating the same screen.

Capture listing images in a normal Level Editor Viewport. Do not use Preview Editor or PIE images as evidence of the product feature.

## Engine packages

Build and register one ZIP for each supported engine: Unreal Engine 5.6, 5.7, and 5.8. Each ZIP must contain only the `EditorActorTagDisplay` plugin folder for that engine and Win64.

## MarketplaceURL manual step

After creating the Fab Draft Listing, copy the official product URL supplied by the Publisher Portal into `MarketplaceURL` in `EditorActorTagDisplay.uplugin`. Rebuild all three engine packages after this change. Do not use a placeholder URL or invent a Listing ID.
