---
title: FAQ
nav_order: 9
---

{% include support-button.html %}

# FAQ

**Why not write directly to a Browser Source local file path?**  
Running an HTTP server gives us health checks, cache-busting via query strings, and easy cross-platform paths.

**Where are files stored?**  
`BASE/overlay/` next to `plugin.json` (module config path’s parent).

**Can I style the overlay?**  
Yes your `overlay/index.html` is yours. Read `plugin.json` and render as you like.
