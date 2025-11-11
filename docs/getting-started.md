---
title: Getting Started
nav_order: 2
---

{% include support-button.html %}

# Getting Started

## 1) Install
1. Build the plugin (or download a release) and place the binary into your OBS plugins path.  
   - Windows: `obs-studio\obs-plugins\64bit\fly-score.dll`
2. Start OBS.

## 2) First Run
- The plugin ensures `BASE/overlay/` exists and seeds defaults.
- A **Browser Source** named (e.g.) `Fly Scoreboard Overlay` is created/updated in the current scene pointing to  
  `http://127.0.0.1:<port>/overlay/index.html`.

## 3) Controls
Open the **Fly Score** dock:
- **Timer**: `countdown / countup`, start/pause, reset, `mm:ss`
- **Scoreboard**: scores + rounds, **Swap** and **Show/Hide**
- **Teams**: titles, subtitles, **logo upload**
- **Settings**: server port, health, hotkeys help, open overlay folder
- **Refresh Overlay**: cache-bust fetch
- **Clear + Reset**: wipe teams/icons and restore defaults

> If the Browser Source doesn’t appear, check you have the OBS Browser plugin enabled.
