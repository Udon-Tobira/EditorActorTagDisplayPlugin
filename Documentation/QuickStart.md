# Quick Start

This guide takes about five minutes in a new Unreal Editor project.

For a public sample and validation project, see the [Actor Metadata Overlay Sample](https://github.com/metyatech/ActorMetadataOverlaySample). Its overview map is `/Game/ActorMetadataOverlayDemo/Maps/ActorMetadataOverlayOverview`.

1. Copy the `ActorMetadataOverlay` folder into the project's `Plugins` folder.
2. Enable **Actor Metadata Overlay** from **Edit > Plugins**.
3. Restart the editor.
4. Open **Project Settings > Plugins > Actor Metadata Overlay**.
5. Confirm that **Default Actor Metadata** is present and enabled.
6. Select an actor in the level and add an Actor Tag in its Details panel.
7. Open the level viewport's **Show** menu and choose **Actor Metadata Overlay > Selected Actors**.
8. Confirm that the actor label, class, and tags appear above the selected actor.
9. Change the viewport mode to **All Matching Actors** to see every matching actor.
10. To inspect a different metadata set, edit the rule template with the tokens in [Settings Reference](SettingsReference.md).

The overlay appears in the normal Level Editor Viewport. Turn **Game View** on to hide it; turning Game View off shows it again. It is not drawn in PIE or SIE game screens, Static Mesh Editor previews, or other Asset Preview Viewports. To verify a non-empty **Data Layers** token, use a World Partition map.

If nothing appears, check the following:

- The plugin is enabled and the editor was restarted.
- The rule is enabled and its actor class is valid.
- The selected actor matches the required and excluded Actor Tags.
- The viewport mode is not **Off**.
- The actor is not hidden and is within the global and per-rule distance limits.
- The rule's `bSelectedOnly` option is not restricting an All Matching Actors view.
