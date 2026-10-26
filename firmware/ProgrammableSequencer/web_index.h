#pragma once
// Embedded pages: login and settings. HTML/CSS/JS with no external dependencies
// (the board may be on its own AP, with no internet).

#include <Arduino.h>

#define SEQ_STYLE R"CSS(
:root{--bg:#f4f4f2;--card:#fff;--fg:#1b1b1b;--muted:#666;--line:#dcdcd8;--accent:#1f6feb;--accent-fg:#fff;
--ok:#1a7f37;--warn:#b35900;--err:#c62828;--chip:#eef2f8;--on:#f5b301}
@media (prefers-color-scheme:dark){:root{--bg:#141414;--card:#1e1e1e;--fg:#ececec;--muted:#a0a0a0;--line:#333;
--accent:#4c8dff;--accent-fg:#fff;--ok:#4caf50;--warn:#ffa726;--err:#ef5350;--chip:#263040;--on:#ffcc33}}
*{box-sizing:border-box}html,body{margin:0;background:var(--bg);color:var(--fg);
font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
header{display:flex;align-items:center;gap:12px;padding:12px 16px;background:var(--card);border-bottom:1px solid var(--line)}
.logo{background:#fff;border-radius:6px;padding:4px;display:flex;align-items:center}
.logo img{height:52px;width:auto;display:block}
h1{font-size:1.15rem;margin:0}h2{font-size:1rem;margin:0 0 10px}
main{max-width:760px;margin:0 auto;padding:12px 16px 40px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px;margin:12px 0}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
button{font:inherit;border:1px solid var(--line);background:var(--card);color:var(--fg);border-radius:8px;
padding:10px 14px;cursor:pointer;min-height:44px}
button.primary{background:var(--accent);border-color:var(--accent);color:var(--accent-fg);font-weight:600}
button.stop{background:var(--err);border-color:var(--err);color:#fff;font-weight:600}
button:disabled{opacity:.45;cursor:not-allowed}
input[type=text],input[type=password],input[type=number]{font:inherit;width:100%;padding:9px 10px;border-radius:8px;
border:1px solid var(--line);background:var(--bg);color:var(--fg)}
label{display:block;margin:8px 0 4px;color:var(--muted);font-size:.9rem}
.inl{display:inline-flex;align-items:center;gap:6px;margin:4px 14px 4px 0;color:var(--fg);font-size:1rem}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:0 12px}
@media (max-width:520px){.grid2{grid-template-columns:1fr}}
.tabs{display:flex;gap:6px;margin-top:12px}.tabs button{flex:1}
.tabs button.act{background:var(--accent);color:var(--accent-fg);border-color:var(--accent)}
.chans{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.chans button{display:flex;flex-direction:column;align-items:center;padding:8px 4px}
.chans small{color:var(--muted);font-size:.72rem;max-width:100%;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.seq{display:flex;flex-wrap:wrap;gap:6px;min-height:44px;padding:8px;border:1px dashed var(--line);border-radius:8px;margin:10px 0}
.chip{background:var(--chip);border-radius:16px;padding:4px 10px;font-weight:600;font-variant-numeric:tabular-nums}
.chip.cur{background:var(--on);color:#000}
.lamps{display:flex;gap:6px;flex-wrap:wrap}
.lamp{width:38px;height:38px;border-radius:50%;border:2px solid var(--line);display:flex;align-items:center;
justify-content:center;font-size:.8rem;font-weight:700;color:var(--muted)}
.lamp.on{background:var(--on);border-color:var(--on);color:#000}
.pill{margin-left:auto;padding:4px 10px;border-radius:12px;font-size:.85rem;font-weight:600;background:var(--chip)}
.msg{margin-top:8px;font-weight:600}.msg.ok{color:var(--ok)}.msg.err{color:var(--err)}
.muted{color:var(--muted);font-size:.9rem}.hide{display:none}
.kv{display:grid;grid-template-columns:max-content 1fr;gap:4px 12px;font-size:.95rem}
details summary{cursor:pointer;color:var(--muted)}
)CSS"

static const char LOGIN_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Programmable Sequencer</title><style>)HTML" SEQ_STYLE R"HTML(
.login{max-width:360px;margin:40px auto;text-align:center}.login .logo{display:inline-flex;margin-bottom:12px}
.login .logo img{height:120px}</style></head><body>
<div class="login card"><div class="logo"><img src="/logo.svg" alt="Nicolau dos Brinquedos"></div>
<h1>Programmable Sequencer</h1>
<form method="post" action="/login" style="text-align:left">
<label for="p">Password</label><input id="p" type="password" name="pass" autocomplete="current-password" autofocus required>
<div id="e" class="msg err hide">Wrong password.</div>
<div style="margin-top:12px"><button class="primary" style="width:100%">Sign in</button></div></form></div>
<script>if(location.search.indexOf('error=1')>=0)document.getElementById('e').classList.remove('hide')</script>
</body></html>)HTML";

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Programmable Sequencer</title><style>)HTML" SEQ_STYLE R"HTML(</style></head><body>
<header><div class="logo"><img src="/logo.svg" alt="Nicolau dos Brinquedos"></div>
<h1>Programmable Sequencer</h1><span id="pill" class="pill">…</span></header>
<main>
<div class="tabs"><button id="tSeq" class="act" onclick="tab('seq')">Sequence</button>
<button id="tNet" onclick="tab('net')">Network</button></div>

<section id="pSeq">
<div class="card">
<div class="row"><button class="primary" onclick="cmd('start')">▶ Start</button>
<button class="stop" onclick="cmd('stop')">■ Stop</button>
<span class="muted" id="prog"></span></div>
<div class="lamps" id="lamps" style="margin-top:12px"></div>
<div id="relErr" class="msg err hide">The relays are not answering (TCA9554).</div>
</div>

<div class="card">
<h2>Sequence</h2>
<div class="muted">Tap a channel to add a step. Repeats are allowed.</div>
<div class="chans" id="chans" style="margin-top:10px"></div>
<div class="seq" id="seq"></div>
<div class="row"><button onclick="back()">⌫ Delete last</button><button onclick="clr()">Clear</button>
<span class="muted" id="cnt"></span></div>
<label for="seqTxt">Or type it (channels 1 to 8 separated by commas)</label>
<input id="seqTxt" type="text" inputmode="numeric" oninput="fromText()" placeholder="4,4,6,7,8,1,2,5,5,6">
</div>

<div class="card">
<h2>Light mode</h2>
<label class="inl"><input type="radio" name="mode" value="PULSE" onchange="modeUi();dirty()"> Pulse</label>
<label class="inl"><input type="radio" name="mode" value="SWAP" onchange="modeUi();dirty()"> Swap</label>
<div class="muted" id="modeHelp"></div>
<div class="grid2">
<div><label for="on">On time per step (s)</label><input id="on" type="number" min="0.1" step="0.1" oninput="dirty()"></div>
<div id="gapBox"><label for="gap">Time between steps (s)</label><input id="gap" type="number" min="0.1" step="0.1" oninput="dirty()"></div>
<div><label for="cyc">Time between cycles (s)</label><input id="cyc" type="number" min="0" step="0.1" oninput="dirty()"></div>
<div id="blinkBox"><label for="blink">Off time on a repeat (s)</label><input id="blink" type="number" min="0.1" step="0.1" oninput="dirty()"></div>
</div>
</div>

<div class="card">
<h2>Repeat</h2>
<label class="inl"><input type="radio" name="rep" value="ONCE" onchange="repUi();dirty()"> Once</label>
<label class="inl"><input type="radio" name="rep" value="N_TIMES" onchange="repUi();dirty()"> N times</label>
<label class="inl"><input type="radio" name="rep" value="LOOP" onchange="repUi();dirty()"> Forever</label>
<div id="cntBox" style="max-width:160px"><label for="rc">Number of cycles</label><input id="rc" type="number" min="1" step="1" oninput="dirty()"></div>
<label class="inl" style="margin-top:12px"><input type="checkbox" id="auto" onchange="dirty()"> Start by itself when the board powers up</label>
<details style="margin-top:12px"><summary>Channel names</summary><div class="grid2" id="names"></div></details>
</div>

<div class="card"><div class="row"><button class="primary" onclick="save()">Save</button>
<span class="muted" id="dirtyMsg"></span></div><div id="saveMsg" class="msg hide"></div>
<div class="muted" style="margin-top:6px">Saving stops a running sequence. Then press Start.</div></div>

<div class="card"><h2>Manual test</h2>
<div class="muted">Switches each channel on and off. Only works while the sequence is stopped.</div>
<div class="chans" id="manual" style="margin-top:10px"></div></div>
</section>

<section id="pNet" class="hide">
<div class="card"><h2>Network status</h2><div class="kv" id="netState"></div></div>

<div class="card"><h2>Network settings</h2>
<label for="host">Network name (hostname)</label><input id="host" type="text">
<div class="muted" id="hostHelp"></div>

<h2 style="margin-top:16px">Ethernet</h2>
<label class="inl"><input type="checkbox" id="ethEn"> Enabled</label>
<label class="inl"><input type="radio" name="ethMode" value="dhcp" onchange="ipUi('eth')"> DHCP</label>
<label class="inl"><input type="radio" name="ethMode" value="static" onchange="ipUi('eth')"> Static IP</label>
<div class="grid2" id="ethStatic"></div>

<h2 style="margin-top:16px">WiFi</h2>
<label class="inl"><input type="checkbox" id="wifiEn"> Enabled</label>
<div class="row" style="margin-top:6px"><button id="scanBtn" onclick="scan()">Scan networks</button><span class="muted" id="scanMsg"></span></div>
<div id="scanList"></div>
<div class="grid2"><div><label for="ssid">Network (SSID)</label><input id="ssid" type="text" autocomplete="off"></div>
<div><label for="wpass">WiFi password</label><input id="wpass" type="password" autocomplete="new-password"></div></div>
<label class="inl"><input type="radio" name="wifiMode" value="dhcp" onchange="ipUi('wifi')"> DHCP</label>
<label class="inl"><input type="radio" name="wifiMode" value="static" onchange="ipUi('wifi')"> Static IP</label>
<div class="grid2" id="wifiStatic"></div>

<h2 style="margin-top:16px">Home Assistant (MQTT, optional)</h2>
<div class="grid2"><div><label for="mb">Broker (empty = off)</label><input id="mb" type="text"></div>
<div><label for="mp">Port</label><input id="mp" type="number" min="1" max="65535"></div>
<div><label for="mu">User</label><input id="mu" type="text" autocomplete="off"></div>
<div><label for="mw">Password</label><input id="mw" type="password" autocomplete="new-password"></div></div>
<div class="row" style="margin-top:12px"><button class="primary" onclick="saveNet()">Save and restart</button></div>
<div id="netMsg" class="msg hide"></div>
<div class="muted" style="margin-top:6px">The board restarts to apply the network settings. The saved sequence is kept.</div>
</div>

<div class="card"><h2>Page password</h2>
<div class="grid2"><div><label for="pc">Current password</label><input id="pc" type="password" autocomplete="current-password"></div><div></div>
<div><label for="pn">New password</label><input id="pn" type="password" autocomplete="new-password"></div>
<div><label for="pf">Repeat the new password</label><input id="pf" type="password" autocomplete="new-password"></div></div>
<div class="row" style="margin-top:12px"><button onclick="savePass()">Change password</button></div>
<div id="passMsg" class="msg hide"></div>
<div class="muted" style="margin-top:6px">Forgot it? Hold the board's BOOT button for 10 s to go back to the default password. This change does not affect the AP WiFi password.</div>
</div>

<div class="card"><form method="post" action="/logout"><button>Sign out</button></form></div>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
let cfg=null,seq=[],isDirty=false,st=null;
const STN={STOPPED:'Stopped',STEP:'Lit',REPEAT_OFF:'Off (repeat)',GAP:'Gap',CYCLE_PAUSE:'Cycle pause'};
function tab(t){$('pSeq').classList.toggle('hide',t!='seq');$('pNet').classList.toggle('hide',t!='net');
$('tSeq').classList.toggle('act',t=='seq');$('tNet').classList.toggle('act',t=='net');if(t=='net')loadNet()}
async function api(path,body){const o=body===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)};
const r=await fetch(path,o);if(r.status==401){location.reload();throw 0}
let j={};try{j=await r.json()}catch(e){}if(!r.ok)throw new Error(j.error||('Error '+r.status));return j}
function msg(id,ok,t){const e=$(id);e.textContent=t;e.className='msg '+(ok?'ok':'err')}
const s2ms=v=>Math.round(parseFloat(String(v).replace(',','.'))*1000);
const ms2s=v=>(v/1000).toString();
function name(i){return cfg&&cfg.channel_names[i-1]||('Channel '+i)}
function buildChans(){let h='',m='';for(let i=1;i<=8;i++){h+=`<button onclick="add(${i})"><b>CH${i}</b><small>${esc(name(i))}</small></button>`;
m+=`<button id="m${i}" onclick="man(${i})"><b>CH${i}</b><small>${esc(name(i))}</small></button>`}$('chans').innerHTML=h;$('manual').innerHTML=m}
function esc(s){return String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function renderSeq(fromTxt){const cur=st&&st.state!='STOPPED'?st.step-1:-1;
$('seq').innerHTML=seq.length?seq.map((c,i)=>`<span class="chip${i==cur&&!isDirty?' cur':''}" title="${esc(name(c))}">${c}</span>`).join(''):'<span class="muted">Empty sequence</span>';
$('cnt').textContent=seq.length+(seq.length==1?' step':' steps');if(!fromTxt)$('seqTxt').value=seq.join(',')}
function add(c){seq.push(c);dirty();renderSeq()}
function back(){seq.pop();dirty();renderSeq()}
function clr(){seq=[];dirty();renderSeq()}
function fromText(){const t=$('seqTxt').value.split(/[\s,;]+/).filter(x=>x.length);const ok=t.every(x=>/^[1-8]$/.test(x));
$('seqTxt').style.borderColor=ok?'':'var(--err)';if(ok){seq=t.map(Number);dirty();renderSeq(true)}}
function radio(n){const e=document.querySelector(`input[name=${n}]:checked`);return e?e.value:null}
function setRadio(n,v){const e=document.querySelector(`input[name=${n}][value="${v}"]`);if(e)e.checked=true}
function modeUi(){const p=radio('mode')=='PULSE';$('gapBox').classList.toggle('hide',!p);$('blinkBox').classList.toggle('hide',p);
$('modeHelp').textContent=p?'Pulse: there is an off time between the channels.':'Swap: the next channel turns on as the previous one turns off. A channel repeated right after gets a short off.'}
function repUi(){$('cntBox').classList.toggle('hide',radio('rep')!='N_TIMES')}
function dirty(){isDirty=true;$('dirtyMsg').textContent='Unsaved changes';$('saveMsg').classList.add('hide')}
function fill(){seq=cfg.sequence.slice();setRadio('mode',cfg.mode);setRadio('rep',cfg.repeat);
$('on').value=ms2s(cfg.on_ms);$('gap').value=ms2s(cfg.gap_ms);$('cyc').value=ms2s(cfg.cycle_pause_ms);$('blink').value=ms2s(cfg.blink_off_ms);
$('rc').value=cfg.repeat_count;$('auto').checked=cfg.autostart;
$('names').innerHTML=cfg.channel_names.map((n,i)=>`<div><label for="n${i}">CH${i+1}</label><input id="n${i}" type="text" value="${esc(n)}" oninput="dirty()"></div>`).join('');
modeUi();repUi();buildChans();renderSeq();isDirty=false;$('dirtyMsg').textContent=''}
async function load(){cfg=await api('/api/config');fill()}
async function save(){const b={sequence:seq,mode:radio('mode'),repeat:radio('rep'),on_ms:s2ms($('on').value),gap_ms:s2ms($('gap').value),
cycle_pause_ms:s2ms($('cyc').value),blink_off_ms:s2ms($('blink').value),repeat_count:parseInt($('rc').value)||0,autostart:$('auto').checked,
channel_names:[...Array(8).keys()].map(i=>$('n'+i).value)};
for(const k of['on_ms','gap_ms','cycle_pause_ms','blink_off_ms'])if(isNaN(b[k])){msg('saveMsg',false,'Fill in all the times.');return}
try{await api('/api/config',b);cfg=await api('/api/config');fill();msg('saveMsg',true,'Settings saved.')}catch(e){if(e)msg('saveMsg',false,e.message)}}
async function cmd(c){try{await api('/api/'+c,{});poll()}catch(e){}}
async function man(i){if(!st||st.state!='STOPPED')return;try{await api('/api/manual',{mask:st.manual^(1<<(i-1))});poll()}catch(e){if(e)alert(e.message)}}
function lamps(mask){let h='';for(let i=1;i<=8;i++)h+=`<div class="lamp${mask>>(i-1)&1?' on':''}" title="${esc(name(i))}">${i}</div>`;$('lamps').innerHTML=h}
async function poll(){try{st=await api('/api/state')}catch(e){return}
const run=st.state!='STOPPED';$('pill').textContent=STN[st.state]||st.state;
$('prog').textContent=run?`Step ${st.step}/${st.total} · CH${st.channel} · cycle ${st.cycle}${st.cycles?'/'+st.cycles:''}`:'';
lamps(st.relays);$('relErr').classList.toggle('hide',st.relays_ok);
for(let i=1;i<=8;i++){const b=$('m'+i);if(b){b.disabled=run;b.classList.toggle('primary',!!(st.manual>>(i-1)&1))}}
renderSeq(document.activeElement==$('seqTxt'));netState()}
function netState(){if(!st)return;const n=st.net;$('netState').innerHTML=
`<span>Ethernet</span><span>${n.eth_ip?n.eth_ip:(n.eth_link?'cable, no IP':'no cable')}</span>`+
`<span>WiFi</span><span>${n.wifi_ip||'disconnected'}</span>`+
`<span>AP</span><span>${n.ap?('on: '+esc(n.ap_ssid)):'off'}</span>`+
`<span>Home Assistant</span><span>${n.mqtt?'connected':'disconnected'}</span>`+
`<span>Address</span><span>http://${esc(n.hostname)}.local</span>`}
function ipFields(p){return['ip','gateway','netmask','dns'].map(k=>`<div><label for="${p}_${k}">${{ip:'IP',gateway:'Gateway',netmask:'Netmask',dns:'DNS (optional)'}[k]}</label><input id="${p}_${k}" type="text" inputmode="decimal"></div>`).join('')}
function ipUi(p){$(p+'Static').classList.toggle('hide',radio(p+'Mode')!='static')}
async function loadNet(){const n=await api('/api/network');$('host').value=n.hostname;
$('hostHelp').textContent='The page is at http://'+n.hostname+'.local';
$('ethStatic').innerHTML=ipFields('eth');$('wifiStatic').innerHTML=ipFields('wifi');
for(const p of['eth','wifi']){const o=n[p];$(p+'En').checked=o.enabled;setRadio(p+'Mode',o.dhcp?'dhcp':'static');
for(const k of['ip','gateway','netmask','dns'])$(p+'_'+k).value=o[k]||'';ipUi(p)}
$('ssid').value=n.wifi.ssid;$('wpass').value='';$('wpass').placeholder=n.wifi.pass_set?'•••••• (kept)':'';
$('mb').value=n.mqtt.broker;$('mp').value=n.mqtt.port;$('mu').value=n.mqtt.user;$('mw').value='';$('mw').placeholder=n.mqtt.pass_set?'•••••• (kept)':''}
async function saveNet(){const ifc=p=>{const o={enabled:$(p+'En').checked,dhcp:radio(p+'Mode')!='static'};for(const k of['ip','gateway','netmask','dns'])o[k]=$(p+'_'+k).value.trim();return o};
const b={hostname:$('host').value.trim(),eth:ifc('eth'),wifi:ifc('wifi'),mqtt:{broker:$('mb').value.trim(),port:parseInt($('mp').value)||1883,user:$('mu').value}};
b.wifi.ssid=$('ssid').value;if($('wpass').value)b.wifi.pass=$('wpass').value;if($('mw').value)b.mqtt.pass=$('mw').value;
try{await api('/api/network',b);msg('netMsg',true,'Saved. The board is restarting; reconnect in a few seconds.')}catch(e){if(e)msg('netMsg',false,e.message)}}
function bars(r){return r>=-55?'▂▄▆█':r>=-67?'▂▄▆':r>=-78?'▂▄':'▂'}
async function scan(){$('scanBtn').disabled=true;$('scanMsg').textContent='Scanning…';$('scanList').innerHTML='';
try{for(let i=0;i<20;i++){const r=await api('/api/wifi/scan');if(!r.scanning){
const l=r.networks.sort((a,b)=>b.rssi-a.rssi);$('scanMsg').textContent=l.length?l.length+' networks found':'No network found';
$('scanList').innerHTML=l.map(n=>`<button class="net" style="display:flex;width:100%;justify-content:space-between;margin-top:6px" data-s="${esc(n.ssid)}" onclick="pick(this.dataset.s)"><span>${esc(n.ssid)}${n.secure?' 🔒':''}</span><span class="muted">${bars(n.rssi)} ${n.rssi} dBm</span></button>`).join('');
break}await new Promise(r=>setTimeout(r,1000))}}catch(e){if(e)$('scanMsg').textContent=e.message}
$('scanBtn').disabled=false}
function pick(s){$('ssid').value=s;$('wifiEn').checked=true;$('scanList').innerHTML='';$('scanMsg').textContent='Network chosen: '+s;$('wpass').focus()}
async function savePass(){try{await api('/api/password',{current:$('pc').value,new:$('pn').value,confirm:$('pf').value});
$('pc').value=$('pn').value=$('pf').value='';msg('passMsg',true,'Password changed.')}catch(e){if(e)msg('passMsg',false,e.message)}}
load().then(poll);setInterval(poll,1000);
</script></body></html>)HTML";
