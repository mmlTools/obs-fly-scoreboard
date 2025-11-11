const els = {
  // root
  board: document.getElementById("scoreboard"),

  // left (home DOM bucket)
  homeTitle: document.getElementById("homeTitle"),
  homeSub: document.getElementById("homeSub"),
  homeLogo: document.getElementById("homeLogo"),
  homeScore: document.getElementById("homeScore"),

  // right (away DOM bucket)
  awayTitle: document.getElementById("awayTitle"),
  awaySub: document.getElementById("awaySub"),
  awayLogo: document.getElementById("awayLogo"),
  awayScore: document.getElementById("awayScore"),

  // center
  time_label: document.getElementById("time_label"),
  timer: document.getElementById("timer"),
  aggScore: document.getElementById("aggScore"),
};

let last = null;

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

function setLogo(img, src) {
  if (!img) return;
  if (!src) {
    img.removeAttribute("src");
    img.alt = "";
    return;
  }
  img.src = src; // relative path served next to index.html
}

function render(st) {
  const {
    home,
    away,
    time_label,
    timer,
    swap_sides = false,
    show_scoreboard = true,
  } = st;

  // Map data to DOM buckets based on swap_sides
  const left = swap_sides ? away : home;
  const right = swap_sides ? home : away;

  // Left (home DOM bucket)
  els.homeTitle.textContent = left.title || "Home";
  els.homeSub.textContent = left.subtitle || "";
  els.homeScore.textContent = Number(left.score ?? 0);
  setLogo(els.homeLogo, left.logo);

  // Right (away DOM bucket)
  els.awayTitle.textContent = right.title || "Guests";
  els.awaySub.textContent = right.subtitle || "";
  els.awayScore.textContent = Number(right.score ?? 0);
  setLogo(els.awayLogo, right.logo);

  // Center label + time + aggregate score (match wins)
  els.time_label.textContent = time_label || "generic";
  els.timer.textContent = mmss(timer?.remaining_ms ?? 0);

  const lRounds = Number(left.rounds ?? 0);
  const rRounds = Number(right.rounds ?? 0);
  els.aggScore.textContent = `${lRounds} : ${rRounds}`;

  // Animate show/hide only when the flag changes
  if (!last || last.show_scoreboard !== show_scoreboard) {
    if (show_scoreboard) {
      els.board.classList.remove("is-hidden");
      els.board.setAttribute("aria-hidden", "false");
    } else {
      els.board.classList.add("is-hidden");
      els.board.setAttribute("aria-hidden", "true");
    }
  }

  last = st;
}

async function loop() {
  try {
    const st = await fetchState();
    render(st);
  } catch (e) {
    // silent retry
  } finally {
    setTimeout(loop, 1000);
  }
}

loop();
