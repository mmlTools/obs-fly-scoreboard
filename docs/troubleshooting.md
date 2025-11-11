---
title: Troubleshooting
nav_order: 7
---

{% include support-button.html %}

# Troubleshooting

### The Browser Source doesn’t show the overlay
- Verify **OBS Browser** is installed/enabled.
- Check the **Settings** dialog: server is running and port is correct.
- Click **Refresh Overlay** to force a reload.

### I change titles but they revert
- The dock avoids overwriting focused fields, but ensure you press **Enter** or click outside to commit.
- Confirm `overlay/plugin.json` updates (timestamps).

### Logos won’t update
- Logos are copied to `overlay/` with a **hash** in the name. If you manually replace files, click **Refresh Overlay**.
- Use **Clear + Reset** to wipe previous logo variants.

### Timer broken / wrong mode
- When switching **countup/countdown**, press **Apply** or start again. Reset respects the active mode.
