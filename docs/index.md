---
layout: default
title: Fly Scoreboard
nav_order: 1
---

{% include support-button.html %}

<span class="hero-badge">OBS Plugin</span>

# Fly Scoreboard
A lightweight, hotkey-friendly scoreboard overlay for OBS with a built-in web server and an auto-managed Browser Source.

- **Hotkeys everywhere** — bump scores/rounds, toggle swap/show, control the timer.
- **Live overlay** — JSON-driven `overlay/plugin.json`, cache-busted reloads.
- **Nice UX** — safe text editing (no flicker), hashed logo uploads to avoid stale cache.
- **One-click reset** — clear teams, delete icons, reset `plugin.json`.

[Get Started →](getting-started.html){: .btn .btn-primary }
[Support on Ko-fi](https://ko-fi.com/mmltech){: .btn }

---

## Features
- Dock UI with **Timer**, **Scoreboard**, **Team** controls
- Browser Source auto-create/update (`overlay/index.html`)
- **Hashed** icon uploads (`home-<hash>.png`, `guest-<hash>.png`)
- **Clear + Reset** button: wipes teams/icons, restores defaults
- **Settings Dialog** with server health, port, hotkey help, open overlay folder

> Want to help this project grow?  
> {% include support-button.html %}
