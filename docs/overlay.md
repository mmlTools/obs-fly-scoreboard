---
title: Overlay
nav_order: 6
---

{% include support-button.html %}

# Overlay

The Browser Source points at:

```
http://127.0.0.1:<port>/overlay/index.html
```

## Data contract: `overlay/plugin.json`
```json
{
  "version": 1,
  "server": { "port": 8089 },
  "time_label": "First Half",
  "timer": {
    "mode": "countdown",
    "running": false,
    "initial_ms": "0",
    "remaining_ms": "0",
    "last_tick_ms": "0"
  },
  "home": { "title": "", "subtitle": "", "logo": "", "score": 0, "rounds": 0 },
  "away": { "title": "", "subtitle": "", "logo": "", "score": 0, "rounds": 0 },
  "swap_sides": false,
  "show_scoreboard": true
}
```

Your HTML/JS overlay should **poll or watch** this file and re-render. The plugin **cache-busts** Browser Source URL updates.
