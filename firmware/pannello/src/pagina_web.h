// Pagina web del pannello (servita da server_web.cpp). Un solo file: HTML, CSS e JavaScript.
#pragma once

static const char PAGINA_WEB[] = R"PAGINA(<!doctype html>
<html lang="it">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>loopant · pannello</title>
<style>
:root{--bg:#0E1217;--surface:#171D24;--surface2:#212932;--border:#2D3742;--text:#E8EDF2;--text2:#A3AFBB;
--accent:#FFB547;--onaccent:#1A1206;--accentbg:#3A2C10;--ok:#46C08A;--okbg:#12301F;--danger:#D13438;--dangerbg:#3B1517;--dangertext:#FF9A9C}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:16px/1.4 -apple-system,system-ui,"Segoe UI",Roboto,sans-serif}
header{position:sticky;top:0;z-index:2;display:flex;flex-wrap:wrap;align-items:center;gap:12px;padding:12px 16px;background:var(--surface);border-bottom:1px solid var(--border)}
header h1{margin:0;font-size:20px;font-weight:800;letter-spacing:.04em}
header .loc{color:var(--text2);font-size:14px}
.spazio{flex:1}
.chip{padding:6px 12px;border-radius:6px;font-size:13px;font-weight:700;letter-spacing:.04em;background:var(--surface2);color:var(--text2)}
.chip.ok{background:var(--okbg);color:var(--ok)}.chip.mov{background:var(--accentbg);color:var(--accent)}.chip.err{background:var(--dangerbg);color:var(--dangertext)}
button{font:inherit;font-weight:700;border-radius:10px;border:1px solid var(--border);background:var(--surface2);color:var(--text);min-height:46px;padding:0 16px;cursor:pointer}
button:active{filter:brightness(1.3)}
button.stop{background:var(--danger);border:0;color:#fff;font-size:18px;letter-spacing:.06em;padding:0 22px}
button.primario{background:var(--accent);color:var(--onaccent);border:0}
main{max-width:1100px;margin:0 auto;padding:16px;display:grid;gap:16px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:16px}
.titoletto{font-size:13px;font-weight:700;color:var(--text2);letter-spacing:.06em;margin:0 0 10px}
.freq{font-size:44px;font-weight:700;color:var(--accent);font-variant-numeric:tabular-nums}
.freq small{font-size:20px;color:var(--text2);font-weight:600}
.riga{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-top:12px}
input{font:inherit;font-size:24px;font-weight:700;width:170px;min-height:52px;padding:0 12px;border-radius:10px;border:2px solid var(--border);background:var(--bg);color:var(--text)}
input:focus{outline:none;border-color:var(--accent)}
.info{color:var(--text2);font-size:14px;margin-top:10px}
.info b{color:var(--text)}
#schermo{display:block;width:100%;max-width:1024px;aspect-ratio:1024/600;border-radius:8px;border:1px solid var(--border);background:#000;cursor:crosshair;touch-action:manipulation}
label{color:var(--text2);font-size:14px;display:flex;gap:6px;align-items:center}
.avviso{color:var(--text2);font-size:13px}
</style>
</head>
<body>
<header>
  <h1 id="call">loopant</h1><span class="loc" id="loc"></span>
  <span class="spazio"></span>
  <span class="chip" id="stato">…</span>
  <button class="stop" onclick="comando('stop')">■ STOP</button>
</header>
<main>
  <section class="card">
    <p class="titoletto">ANTENNA ACCORDATA SU</p>
    <div class="freq"><span id="accordata">—</span> <small>kHz</small></div>
    <div class="info">Posizione <b id="pos">—</b> · <span id="err"></span></div>
    <div class="riga">
      <input id="khz" inputmode="decimal" placeholder="kHz" aria-label="Frequenza in kHz">
      <button class="primario" onclick="tune()">TUNE</button>
      <button onclick="banda(40)">40 m</button><button onclick="banda(20)">20 m</button>
      <button onclick="passo(-5)">−5</button><button onclick="passo(-1)">−1</button>
      <button onclick="passo(1)">+1</button><button onclick="passo(5)">+5</button>
      <button onclick="comando('zero')">ZERO</button>
    </div>
    <p class="avviso">In TX non accordare con RF ad alta potenza: circa 5 W per la ricerca del ROS minimo.</p>
  </section>
  <section class="card">
    <p class="titoletto">SCHERMO DEL PANNELLO · tocca l'immagine per toccare lo schermo</p>
    <img id="schermo" alt="Schermo del pannello" src="/schermo.bmp?scala=2">
    <div class="riga">
      <label><input type="checkbox" id="auto" checked style="width:auto;min-height:0"> Aggiorna automaticamente</label>
      <button onclick="ricarica(true)">Aggiorna ora (piena risoluzione)</button>
      <span class="avviso" id="rete"></span>
    </div>
  </section>
</main>
<script>
const $ = s => document.querySelector(s);
let ultimo = null;
const migliaia = n => String(n).replace(/\B(?=(\d{3})+(?!\d))/g, ' ');
function kHz(hz){ const d = Math.round(hz / 100); return migliaia(Math.floor(d / 10)) + ',' + (d % 10); }
function valore(){ return parseFloat($('#khz').value.replace(/\s/g, '').replace(',', '.')); }
function scrivi(k){ $('#khz').value = k.toFixed(1).replace('.', ','); }

async function aggiornaStato(){
  try {
    const s = await (await fetch('/stato', {cache: 'no-store'})).json();
    ultimo = s;
    $('#call').textContent = s.nominativo || 'loopant';
    $('#loc').textContent = s.locator || '';
    const c = $('#stato');
    c.className = 'chip';
    if (!s.collegato) { c.textContent = 'ANTENNA NON COLLEGATA'; c.classList.add('err'); }
    else if (s.stato === 'ERRORE') { c.textContent = 'ERRORE: ' + s.errore; c.classList.add('err'); }
    else { c.textContent = s.stato; c.classList.add(s.stato === 'FERMO' ? 'ok' : 'mov'); }
    $('#accordata').textContent = s.collegato && s.freq_hz ? kHz(s.freq_hz) : '—';
    $('#pos').textContent = s.collegato ? migliaia(s.posizione) : '—';
    $('#err').textContent = s.collegato ? (s.azzerata ? 'azzerata' : 'non azzerata') : '';
    $('#rete').textContent = s.rete || '';
    if (!$('#khz').value && s.ultima40) scrivi(s.ultima40 / 1000);
  } catch (e) {
    const c = $('#stato'); c.className = 'chip err'; c.textContent = 'PANNELLO NON RAGGIUNGIBILE';
  }
}

async function comando(azione, extra){
  try {
    const r = await fetch('/comando', {method: 'POST', body: new URLSearchParams(Object.assign({azione}, extra || {}))});
    const j = await r.json();
    if (!j.ok) alert(j.errore || 'Comando non eseguito');
  } catch (e) { alert('Pannello non raggiungibile'); }
  aggiornaStato();
}
function tune(){ const k = valore(); if (isNaN(k)) return; comando('goto', {khz: k.toFixed(1)}); }
function banda(b){ if (ultimo) scrivi((b === 40 ? ultimo.ultima40 : ultimo.ultima20) / 1000); }
function passo(d){ const k = valore(); if (!isNaN(k)) scrivi(k + d); }

const img = $('#schermo');
let timer = null;
// Aggiornamento continuo a metà risoluzione; "Aggiorna ora" a risoluzione piena
function ricarica(piena){ clearTimeout(timer); img.src = '/schermo.bmp?' + (piena ? '' : 'scala=2&') + 't=' + Date.now(); }
img.onload = img.onerror = () => { clearTimeout(timer); if ($('#auto').checked) timer = setTimeout(() => ricarica(false), 700); };
$('#auto').onchange = () => { if ($('#auto').checked) ricarica(false); };
img.onclick = async e => {
  const r = img.getBoundingClientRect();
  const x = Math.round((e.clientX - r.left) * 1024 / r.width), y = Math.round((e.clientY - r.top) * 600 / r.height);
  try { await fetch('/tocco', {method: 'POST', body: new URLSearchParams({x, y})}); } catch (err) {}
  setTimeout(() => ricarica(false), 300);
};
setInterval(aggiornaStato, 1000);
aggiornaStato();
</script>
</body>
</html>
)PAGINA";
