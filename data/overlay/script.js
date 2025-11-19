// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
function mmss(ms) {
  ms = Math.max(0, ms | 0);
  const m = Math.floor(ms / 60000);
  const s = Math.floor((ms % 60000) / 1000);
  return `${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
}

async function fetchState() {
  const res = await fetch("plugin.json", { cache: "no-store" });
  if (!res.ok) throw new Error("fetch failed");
  return await res.json();
}

/**
 * Compute the live timer value from JSON only.
 * Uses: mode, running, remaining_ms, last_tick_ms.
 */
function liveTimerMs(timer) {
  if (!timer) return 0;

  const mode = timer.mode || "countdown";
  const running = !!timer.running;
  const baseRemaining = Number(timer.remaining_ms ?? 0);
  const lastTick = Number(timer.last_tick_ms ?? 0);

  if (!running || !lastTick) {
    return baseRemaining;
  }

  const now = Date.now();
  const delta = now - lastTick;

  if (mode === "countup") {
    return baseRemaining + delta;
  }

  // countdown
  const v = baseRemaining - delta;
  return v < 0 ? 0 : v;
}

// -----------------------------------------------------------------------------
// Template engine
//
// Supports:
//   {{home.logo}}
//   {{away.title}}
//   {{custom_fields[0].label}}
//   {{timers[0].label}}
//   {{timers[0].mmss}}
//   {{show_rounds ? 'fs-show' : 'fs-hide'}}
// -----------------------------------------------------------------------------
const templateBindings = [];

function collectTemplateBindings() {
  // Text nodes
  const walker = document.createTreeWalker(
    document.body,
    NodeFilter.SHOW_TEXT,
    null
  );

  let node;
  while ((node = walker.nextNode())) {
    const text = node.nodeValue;
    if (text && text.includes("{{")) {
      const binding = createBinding(node, "text", null, text);
      if (binding) templateBindings.push(binding);
    }
  }

  // Attributes
  const all = document.querySelectorAll("*");
  all.forEach((el) => {
    for (const attr of el.attributes) {
      const val = attr.value;
      if (val && val.includes("{{")) {
        const binding = createBinding(el, "attr", attr.name, val);
        if (binding) templateBindings.push(binding);
      }
    }
  });
}

function createBinding(targetNode, kind, attrName, templateString) {
  const parts = [];
  const regex = /{{\s*([^}]+?)\s*}}/g;
  let lastIndex = 0;
  let match;

  while ((match = regex.exec(templateString)) !== null) {
    if (match.index > lastIndex) {
      parts.push({
        type: "literal",
        value: templateString.slice(lastIndex, match.index),
      });
    }
    parts.push({
      type: "expr",
      value: match[1].trim(),
    });
    lastIndex = regex.lastIndex;
  }

  if (!parts.length) return null;

  if (lastIndex < templateString.length) {
    parts.push({
      type: "literal",
      value: templateString.slice(lastIndex),
    });
  }

  return { targetNode, kind, attrName, parts };
}

function resolvePath(obj, path) {
  if (!obj || !path) return undefined;

  // tokens: foo, bar, [0] from "foo.bar[0].baz"
  const re = /([^[.\]]+)|\[(\d+)\]/g;
  let cur = obj;
  let m;

  while ((m = re.exec(path)) !== null) {
    if (m[1]) {
      // dot notation
      cur = cur ? cur[m[1]] : undefined;
    } else if (m[2]) {
      // [index]
      const idx = Number(m[2]);
      if (!Array.isArray(cur)) return undefined;
      cur = cur[idx];
    }
    if (cur == null) return cur;
  }

  return cur;
}

function stripQuotes(s) {
  if (!s) return s;
  s = s.trim();
  if (
    (s.startsWith('"') && s.endsWith('"')) ||
    (s.startsWith("'") && s.endsWith("'"))
  ) {
    return s.slice(1, -1);
  }
  return s;
}

function evaluateExpression(expr, data) {
  if (!expr) return "";

  expr = expr.trim();

  // Ternary:
  //   condition ? 'trueVal' : 'falseVal'
  //   condition ?? 'trueVal' : 'falseVal' (we treat ? and ?? the same here)
  const ternaryMatch = expr.match(
    /^(.+?)\s*(\?\??)\s*(['"].*?['"])\s*:\s*(['"].*?['"])$/
  );

  if (ternaryMatch) {
    const condPath = ternaryMatch[1].trim();
    const trueLit  = stripQuotes(ternaryMatch[3]);
    const falseLit = stripQuotes(ternaryMatch[4]);

    const condVal = resolvePath(data, condPath);
    return condVal ? trueLit : falseLit;
  }

  // Simple path
  const v = resolvePath(data, expr);
  return v == null ? "" : v;
}

function applyTemplateBindings(data) {
  if (!data) return;

  for (const b of templateBindings) {
    let str = "";

    for (const part of b.parts) {
      if (part.type === "literal") {
        str += part.value;
      } else {
        const v = evaluateExpression(part.value, data);
        str += v == null ? "" : String(v);
      }
    }

    if (b.kind === "text") {
      b.targetNode.nodeValue = str;
    } else if (b.kind === "attr") {
      b.targetNode.setAttribute(b.attrName, str);
    }
  }
}

// -----------------------------------------------------------------------------
// Rendering
// -----------------------------------------------------------------------------
let currentJsonState = null;

function renderFrame() {
  if (!currentJsonState) return;

  // Clone to avoid mutating original JSON
  const st = JSON.parse(JSON.stringify(currentJsonState));

  // Normalize timers array
  if (!Array.isArray(st.timers)) {
    st.timers = [];
  }

  // Compute live ms + mmss per timer
  for (let i = 0; i < st.timers.length; i++) {
    const t = st.timers[i] || {};
    const ms = liveTimerMs(t);
    t.live_ms = ms;
    t.mmss = mmss(ms);
    st.timers[i] = t;
  }

  // Apply templates
  applyTemplateBindings(st);

  // Optionally handle show/hide scoreboard via class
  const board = document.getElementById("scoreboard");
  if (board) {
    if (st.show_scoreboard) {
      board.classList.remove("is-hidden");
      board.setAttribute("aria-hidden", "false");
    } else {
      board.classList.add("is-hidden");
      board.setAttribute("aria-hidden", "true");
    }
  }
}

// -----------------------------------------------------------------------------
// Poll loop
// -----------------------------------------------------------------------------
async function pollLoop() {
  try {
    const st = await fetchState();
    currentJsonState = st;
  } catch (e) {
    // ignore errors; keep last state
  } finally {
    setTimeout(pollLoop, 1000); // poll every second
  }
}

function animationLoop() {
  renderFrame();
  requestAnimationFrame(animationLoop);
}

// -----------------------------------------------------------------------------
// Boot
// -----------------------------------------------------------------------------
collectTemplateBindings();
pollLoop();
animationLoop();
