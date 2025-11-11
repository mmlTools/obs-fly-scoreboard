---
title: Usage
nav_order: 3
---

{% include support-button.html %}

# Usage

## Timer
- **Countdown**: type `mm:ss`, press ▶, time decreases to 0.
- **Countup**: type `mm:ss` (optional offset), press ▶, time increases.
- **Reset** respects mode:
  - Countdown → resets to typed (or initial) value
  - Countup   → resets to `0` (or typed offset)

## Scoreboard
- Change **Home/Guests** scores and wins with the spinners.
- Toggle **Swap Home ↔ Guests** to invert sides in the overlay.
- Toggle **Show Scoreboard** to temporarily hide the overlay element.

## Teams & Logos
- Set **Title** and **Subtitle** for each team.
- Click **Browse** to pick a logo:
  - The file is **copied** to `overlay/` with a short **SHA-256 hash** in the filename (avoids cache issues).
- You can also paste a **data: URI** to embed SVG/PNG directly.

## Apply / Refresh
- **Apply** saves to `overlay/plugin.json`.
- **Refresh** cache-busts the Browser Source URL to force reload.

> Edits won’t get wiped while typing: the UI won’t overwrite focused fields.
