---
title: Development
nav_order: 8
---

{% include support-button.html %}

# Development

## Structure
- Dock UI (`FlyScoreDock`) – timer, scoreboard, teams, settings button
- State (`FlyState`) – persisted to `overlay/plugin.json`
- Web server – serves `overlay/` and health endpoint
- Browser Source manager – creates/updates the source in current scene

## Build (Windows example)
- Prereqs: OBS SDK, Qt, CMake, MSVC
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Contributing
PRs welcome: docs, overlay templates, localization, new render flags.
