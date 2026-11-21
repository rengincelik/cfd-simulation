// Lid-driven cavity: browser front end for the C++ solver (cavity.wasm).
// The solver, the Ghia comparison and the colormaps run in C++ (web/cavity_wasm.cpp);
// this file only drives it and draws the result.

const $ = (id) => document.getElementById(id);

const canvas = $('view');
const ctx = canvas.getContext('2d');
const banner = $('banner');
const runBtn = $('run');
const viewBtns = [...document.querySelectorAll('[data-view]')];

const FRAME_BUDGET_MS = 12;                       // time spent stepping per animation frame
const SPEEDS = { max: Infinity, normal: 5, slow: 1 };

let M;                // Emscripten module
let api;              // wrapped C functions
let nx = 0;
let ny = 0;
let off;              // nx x ny offscreen canvas holding the coloured field
let img;              // its ImageData
let view = 0;         // 0 velocity, 1 temperature, 2 pressure
let running = true;
let diverged = false;
let dirty = true;
let lastReadout = 0;

// Same defaults as cavity.cfg; the page overrides Re and grid.
const cfgText = () =>
  [
    `nx ${$('grid').value}`,
    `ny ${$('grid').value}`,
    'lx 1.0',
    'ly 1.0',
    `reynolds ${$('re').value}`,
    'lid_vel 1.0',
    'prandtl 0.71',
    'dt 0.001',
    'p_iter 50',
  ].join('\n');

const f64 = (ptr, n) => new Float64Array(M.HEAPF64.buffer, ptr, n);

function say(message) {
  banner.hidden = !message;
  banner.textContent = message ?? '';
}

// ── simulation control ───────────────────────────────────────────────────────

function start() {
  diverged = false;
  say(null);
  if (!api.init(cfgText())) {
    running = false;
    say('The solver rejected this configuration.');
    syncRunButton();
    return;
  }
  nx = api.nx();
  ny = api.ny();
  off = document.createElement('canvas');
  off.width = nx;
  off.height = ny;
  img = new ImageData(nx, ny);
  dirty = true;
  readout(true);
}

function reset() {
  if (!api.reset()) {
    start();
    return;
  }
  diverged = false;
  say(null);
  dirty = true;
  readout(true);
}

function setRunning(value) {
  running = value && !diverged;
  syncRunButton();
}

function syncRunButton() {
  runBtn.textContent = running ? 'Pause' : 'Run';
}

function setView(next) {
  view = next;
  viewBtns.forEach((b) => b.setAttribute('aria-pressed', String(Number(b.dataset.view) === view)));
  dirty = true;
}

function advance() {
  const cap = SPEEDS[$('speed').value];
  const t0 = performance.now();
  let n = 0;
  while (n < cap) {
    api.step(1);
    n += 1;
    if (performance.now() - t0 > FRAME_BUDGET_MS) break;
  }
}

// ── drawing ──────────────────────────────────────────────────────────────────

function fitCanvas() {
  const rect = canvas.getBoundingClientRect();
  const size = Math.max(1, Math.round(rect.width * (window.devicePixelRatio || 1)));
  if (canvas.width !== size) {
    canvas.width = size;
    canvas.height = size;
    dirty = true;
  }
}

function draw() {
  if (!nx) return;
  const ptr = api.render(view);
  img.data.set(new Uint8ClampedArray(M.HEAPU8.buffer, ptr, nx * ny * 4));
  off.getContext('2d').putImageData(img, 0, 0);

  const size = canvas.width;
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(off, 0, 0, size, size);

  if (view === 0) drawArrows(size);
}

// Velocity arrows, laid out like the SDL version (every `step` cells).
function drawArrows(size) {
  const step = Math.max(2, Math.round(nx / 16));
  const u = f64(api.field(0), nx * ny);
  const v = f64(api.field(1), nx * ny);
  const vmax = Math.max(f64(api.stats(), 8)[3], 1e-9);
  const cell = size / nx;
  const scale = (cell * step * 0.4) / vmax;

  ctx.strokeStyle = 'rgba(255, 255, 255, 0.7)';
  ctx.lineWidth = Math.max(1, size / 480);
  ctx.beginPath();
  for (let j = Math.floor(step / 2); j < ny; j += step) {
    for (let i = Math.floor(step / 2); i < nx; i += step) {
      const k = j * nx + i;
      if (Math.hypot(u[k], v[k]) < 1e-6) continue;
      const cx = (i + 0.5) * cell;
      const cy = (ny - 1 - j + 0.5) * cell;
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + u[k] * scale, cy - v[k] * scale);
    }
  }
  ctx.stroke();
}

// ── numbers ──────────────────────────────────────────────────────────────────

const num = (x, digits) => (Number.isFinite(x) ? x.toFixed(digits) : '—');

function readout(force = false) {
  const now = performance.now();
  if (!force && now - lastReadout < 100) return;
  lastReadout = now;

  const s = f64(api.stats(), 8);
  $('o-step').textContent = String(s[0]);
  $('o-time').textContent = num(s[1], 3);
  $('o-vmax').textContent = num(s[3], 3);
  $('o-dmax').textContent = Number.isFinite(s[2]) ? s[2].toExponential(2) : '—';
  $('o-p').textContent = `${num(s[4], 2)} … ${num(s[5], 2)}`;

  const g = f64(api.ghia(), 5);
  $('ghia-title').textContent = `vs. Ghia et al. (1982), Re ${s[6]}`;
  $('g-u').textContent = g[0] ? `${num(g[1], 4)} / ${num(g[3], 4)}` : 'no reference';
  $('g-v').textContent = g[0] ? `${num(g[2], 4)} / ${num(g[4], 4)}` : 'no reference';

  if (!s[7] && !diverged) {
    diverged = true;
    running = false;
    syncRunButton();
    say('The solution diverged (non-finite values). Press Reset, or lower Re / grid size.');
  }
}

// ── CSV ──────────────────────────────────────────────────────────────────────

function exportCsv() {
  if (!nx) return;
  const n = nx * ny;
  const [u, v, p, t] = [0, 1, 2, 3].map((w) => f64(api.field(w), n));
  const dx = api.lx() / (nx - 1);
  const dy = api.ly() / (ny - 1);
  const e = (x) => x.toExponential(8);

  const rows = ['i,j,x,y,u,v,p,T'];
  for (let j = 0; j < ny; j += 1) {
    for (let i = 0; i < nx; i += 1) {
      const k = j * nx + i;
      rows.push([i, j, e(i * dx), e(j * dy), e(u[k]), e(v[k]), e(p[k]), e(t[k])].join(','));
    }
  }

  const step = String(f64(api.stats(), 8)[0]).padStart(6, '0');
  const a = document.createElement('a');
  a.href = URL.createObjectURL(new Blob([`${rows.join('\n')}\n`], { type: 'text/csv' }));
  a.download = `cavity_Re${$('re').value}_${nx}x${ny}_step${step}.csv`;
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

// ── wiring ───────────────────────────────────────────────────────────────────

function frame() {
  requestAnimationFrame(frame);
  if (!api) return;
  if (running && !diverged) {
    advance();
    readout();
    dirty = true;
  }
  if (dirty) {
    dirty = false;
    draw();
  }
}

function bind() {
  runBtn.addEventListener('click', () => setRunning(!running));
  $('reset').addEventListener('click', reset);
  $('csv').addEventListener('click', exportCsv);
  $('re').addEventListener('change', start);
  $('grid').addEventListener('change', start);
  viewBtns.forEach((b) => b.addEventListener('click', () => setView(Number(b.dataset.view))));

  window.addEventListener('keydown', (e) => {
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    if (e.target.closest?.('select, input')) return;
    const key = e.key.toLowerCase();
    if (key === ' ' && !e.target.closest?.('button')) {
      e.preventDefault();
      setRunning(!running);
    } else if (key === 'r') {
      reset();
    } else if (key === '1' || key === '2' || key === '3') {
      setView(Number(key) - 1);
    }
  });

  new ResizeObserver(fitCanvas).observe(canvas);
  fitCanvas();
}

async function boot() {
  bind();
  say('Loading solver…');
  try {
    // Imported here, not at the top, so a missing build shows a message
    // instead of leaving a silent, dead page.
    const { default: createCfdModule } = await import('./cavity.js');
    M = await createCfdModule();
  } catch (err) {
    console.error(err);
    say('Could not load the WebAssembly solver (docs/cavity.js and cavity.wasm built?).');
    return;
  }
  api = {
    init: M.cwrap('cfd_init', 'number', ['string']),
    reset: M.cwrap('cfd_reset', 'number', []),
    step: M.cwrap('cfd_step', 'number', ['number']),
    nx: M.cwrap('cfd_nx', 'number', []),
    ny: M.cwrap('cfd_ny', 'number', []),
    lx: M.cwrap('cfd_lx', 'number', []),
    ly: M.cwrap('cfd_ly', 'number', []),
    field: M.cwrap('cfd_field', 'number', ['number']),
    render: M.cwrap('cfd_render_rgba', 'number', ['number']),
    // Both return a pointer to a small static table of doubles (read with f64).
    stats: M.cwrap('cfd_stats', 'number', []),
    ghia: M.cwrap('cfd_ghia', 'number', []),
  };
  start();
  window.focus();
}

requestAnimationFrame(frame);
boot();
