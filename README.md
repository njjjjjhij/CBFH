# W Menu - CBFH (Geode source project)

Target metadata: Geode v5.11.2, Geometry Dash 2.2081 (Android and Windows).

## Important: source ZIP is not a `.geode` mod

Do **not** rename this ZIP file to `.geode`. A `.geode` file is a compiled package produced by the Geode build tools; changing a ZIP's extension does not compile it and causes Geode to report an invalid/old mod.

## Build an Android package with GitHub Actions

1. Upload the **contents of this project folder** to the root of a GitHub repository (so `mod.json` and `.github/` are at the repository root).
2. Open the repository's **Actions** tab.
3. Run **Build W Menu** (or push a commit to trigger it).
4. When the workflow succeeds, download the `WMenu-Android64-geode` artifact from that run.
5. Inside the downloaded artifact is the compiled `.geode` file. Install that file in Geode; do not rename the source ZIP.

The workflow uses the Geode SDK version specified in `mod.json` (`v5.11.2`) and targets Android64.

## Current feature status

- W button hook for the main menu and pause menu.
- Popup with five toggle slots.
- CBFH setting with a bounded 1–20 frame counter.
- Four additional utility toggle slots.

The CBFH switch and frame counter are currently UI/configuration scaffolding only. Automatic frame-perfect input is not implemented and cannot guarantee jumps. A successful compile and runtime test is still required before calling this a working mod.
