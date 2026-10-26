#pragma once
// Páginas embutidas: login e configuração. HTML/CSS/JS sem dependências externas
// (a placa pode estar só no AP, sem internet).

#include <Arduino.h>

#define QS_STYLE R"CSS(
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
<html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Quadros Sequenciais</title><style>)HTML" QS_STYLE R"HTML(
.login{max-width:360px;margin:40px auto;text-align:center}.login .logo{display:inline-flex;margin-bottom:12px}
.login .logo img{height:120px}</style></head><body>
<div class="login card"><div class="logo"><img src="/logo.svg" alt="Nicolau dos Brinquedos"></div>
<h1>Quadros Sequenciais</h1>
<form method="post" action="/login" style="text-align:left">
<label for="p">Senha</label><input id="p" type="password" name="pass" autocomplete="current-password" autofocus required>
<div id="e" class="msg err hide">Senha incorreta.</div>
<div style="margin-top:12px"><button class="primary" style="width:100%">Entrar</button></div></form></div>
<script>if(location.search.indexOf('erro=1')>=0)document.getElementById('e').classList.remove('hide')</script>
</body></html>)HTML";

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Quadros Sequenciais</title><style>)HTML" QS_STYLE R"HTML(</style></head><body>
<header><div class="logo"><img src="/logo.svg" alt="Nicolau dos Brinquedos"></div>
<h1>Quadros Sequenciais</h1><span id="pill" class="pill">…</span></header>
<main>
<div class="tabs"><button id="tSeq" class="act" onclick="tab('seq')">Sequência</button>
<button id="tNet" onclick="tab('net')">Rede</button></div>

<section id="pSeq">
<div class="card">
<div class="row"><button class="primary" onclick="cmd('start')">▶ Iniciar</button>
<button class="stop" onclick="cmd('stop')">■ Parar</button>
<span class="muted" id="prog"></span></div>
<div class="lamps" id="lamps" style="margin-top:12px"></div>
<div id="relErr" class="msg err hide">Os relés não respondem (TCA9554).</div>
</div>

<div class="card">
<h2>Sequência</h2>
<div class="muted">Toque num quadro para acrescentar um passo. Repetições são permitidas.</div>
<div class="chans" id="chans" style="margin-top:10px"></div>
<div class="seq" id="seq"></div>
<div class="row"><button onclick="back()">⌫ Apagar último</button><button onclick="clr()">Limpar</button>
<span class="muted" id="cnt"></span></div>
<label for="seqTxt">Ou digite (canais 1 a 8 separados por vírgula)</label>
<input id="seqTxt" type="text" inputmode="numeric" oninput="fromText()" placeholder="4,4,6,7,8,1,2,5,5,6">
</div>

<div class="card">
<h2>Modo de acendimento</h2>
<label class="inl"><input type="radio" name="mode" value="PULSO" onchange="modeUi();dirty()"> Pulso</label>
<label class="inl"><input type="radio" name="mode" value="TROCA" onchange="modeUi();dirty()"> Troca</label>
<div class="muted" id="modeHelp"></div>
<div class="grid2">
<div><label for="on">Tempo aceso por passo (s)</label><input id="on" type="number" min="0.1" step="0.1" oninput="dirty()"></div>
<div id="gapBox"><label for="gap">Tempo entre acionamentos (s)</label><input id="gap" type="number" min="0.1" step="0.1" oninput="dirty()"></div>
<div><label for="cyc">Tempo entre ciclos (s)</label><input id="cyc" type="number" min="0" step="0.1" oninput="dirty()"></div>
<div id="blinkBox"><label for="blink">Apagada na repetição (s)</label><input id="blink" type="number" min="0.1" step="0.1" oninput="dirty()"></div>
</div>
</div>

<div class="card">
<h2>Repetição</h2>
<label class="inl"><input type="radio" name="rep" value="ONCE" onchange="repUi();dirty()"> 1 vez</label>
<label class="inl"><input type="radio" name="rep" value="N_TIMES" onchange="repUi();dirty()"> N vezes</label>
<label class="inl"><input type="radio" name="rep" value="LOOP" onchange="repUi();dirty()"> Sempre</label>
<div id="cntBox" style="max-width:160px"><label for="rc">Número de ciclos</label><input id="rc" type="number" min="1" step="1" oninput="dirty()"></div>
<label class="inl" style="margin-top:12px"><input type="checkbox" id="auto" onchange="dirty()"> Começar sozinho ao ligar a placa</label>
<details style="margin-top:12px"><summary>Nomes dos quadros</summary><div class="grid2" id="names"></div></details>
</div>

<div class="card"><div class="row"><button class="primary" onclick="save()">Salvar</button>
<span class="muted" id="dirtyMsg"></span></div><div id="saveMsg" class="msg hide"></div>
<div class="muted" style="margin-top:6px">Salvar para a sequência em andamento. Depois, use Iniciar.</div></div>

<div class="card"><h2>Teste manual</h2>
<div class="muted">Liga e desliga cada quadro. Só funciona com a sequência parada.</div>
<div class="chans" id="manual" style="margin-top:10px"></div></div>
</section>

<section id="pNet" class="hide">
<div class="card"><h2>Estado da rede</h2><div class="kv" id="netState"></div></div>

<div class="card"><h2>Configuração de rede</h2>
<label for="host">Nome na rede (hostname)</label><input id="host" type="text">
<div class="muted" id="hostHelp"></div>

<h2 style="margin-top:16px">Ethernet</h2>
<label class="inl"><input type="checkbox" id="ethEn"> Habilitada</label>
<label class="inl"><input type="radio" name="ethMode" value="dhcp" onchange="ipUi('eth')"> DHCP</label>
<label class="inl"><input type="radio" name="ethMode" value="static" onchange="ipUi('eth')"> IP fixo</label>
<div class="grid2" id="ethStatic"></div>

<h2 style="margin-top:16px">WiFi</h2>
<label class="inl"><input type="checkbox" id="wifiEn"> Habilitado</label>
<div class="row" style="margin-top:6px"><button id="scanBtn" onclick="scan()">Procurar redes</button><span class="muted" id="scanMsg"></span></div>
<div id="scanList"></div>
<div class="grid2"><div><label for="ssid">Rede (SSID)</label><input id="ssid" type="text" autocomplete="off"></div>
<div><label for="wpass">Senha do WiFi</label><input id="wpass" type="password" autocomplete="new-password"></div></div>
<label class="inl"><input type="radio" name="wifiMode" value="dhcp" onchange="ipUi('wifi')"> DHCP</label>
<label class="inl"><input type="radio" name="wifiMode" value="static" onchange="ipUi('wifi')"> IP fixo</label>
<div class="grid2" id="wifiStatic"></div>

<h2 style="margin-top:16px">Home Assistant (MQTT, opcional)</h2>
<div class="grid2"><div><label for="mb">Broker (vazio = desligado)</label><input id="mb" type="text"></div>
<div><label for="mp">Porta</label><input id="mp" type="number" min="1" max="65535"></div>
<div><label for="mu">Usuário</label><input id="mu" type="text" autocomplete="off"></div>
<div><label for="mw">Senha</label><input id="mw" type="password" autocomplete="new-password"></div></div>
<div class="row" style="margin-top:12px"><button class="primary" onclick="saveNet()">Salvar e reiniciar</button></div>
<div id="netMsg" class="msg hide"></div>
<div class="muted" style="margin-top:6px">A placa reinicia para aplicar a rede. A sequência salva continua.</div>
</div>

<div class="card"><h2>Senha da página</h2>
<div class="grid2"><div><label for="pc">Senha atual</label><input id="pc" type="password" autocomplete="current-password"></div><div></div>
<div><label for="pn">Nova senha</label><input id="pn" type="password" autocomplete="new-password"></div>
<div><label for="pf">Repita a nova senha</label><input id="pf" type="password" autocomplete="new-password"></div></div>
<div class="row" style="margin-top:12px"><button onclick="savePass()">Trocar senha</button></div>
<div id="passMsg" class="msg hide"></div>
<div class="muted" style="margin-top:6px">Esqueceu? Segure o botão BOOT da placa por 10 s para voltar à senha padrão. Esta troca não afeta a senha do WiFi do AP.</div>
</div>

<div class="card"><form method="post" action="/logout"><button>Sair</button></form></div>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
let cfg=null,seq=[],isDirty=false,st=null;
const STN={PARADO:'Parado',PASSO:'Acendendo',APAGADA_REPETICAO:'Apagada (repetição)',INTERVALO:'Intervalo',PAUSA_CICLO:'Pausa entre ciclos'};
function tab(t){$('pSeq').classList.toggle('hide',t!='seq');$('pNet').classList.toggle('hide',t!='net');
$('tSeq').classList.toggle('act',t=='seq');$('tNet').classList.toggle('act',t=='net');if(t=='net')loadNet()}
async function api(path,body){const o=body===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)};
const r=await fetch(path,o);if(r.status==401){location.reload();throw 0}
let j={};try{j=await r.json()}catch(e){}if(!r.ok)throw new Error(j.error||('Erro '+r.status));return j}
function msg(id,ok,t){const e=$(id);e.textContent=t;e.className='msg '+(ok?'ok':'err')}
const s2ms=v=>Math.round(parseFloat(String(v).replace(',','.'))*1000);
const ms2s=v=>(v/1000).toString();
function name(i){return cfg&&cfg.channel_names[i-1]||('Quadro '+i)}
function buildChans(){let h='',m='';for(let i=1;i<=8;i++){h+=`<button onclick="add(${i})"><b>CH${i}</b><small>${esc(name(i))}</small></button>`;
m+=`<button id="m${i}" onclick="man(${i})"><b>CH${i}</b><small>${esc(name(i))}</small></button>`}$('chans').innerHTML=h;$('manual').innerHTML=m}
function esc(s){return String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function renderSeq(fromTxt){const cur=st&&st.state!='PARADO'?st.step-1:-1;
$('seq').innerHTML=seq.length?seq.map((c,i)=>`<span class="chip${i==cur&&!isDirty?' cur':''}" title="${esc(name(c))}">${c}</span>`).join(''):'<span class="muted">Sequência vazia</span>';
$('cnt').textContent=seq.length+(seq.length==1?' passo':' passos');if(!fromTxt)$('seqTxt').value=seq.join(',')}
function add(c){seq.push(c);dirty();renderSeq()}
function back(){seq.pop();dirty();renderSeq()}
function clr(){seq=[];dirty();renderSeq()}
function fromText(){const t=$('seqTxt').value.split(/[\s,;]+/).filter(x=>x.length);const ok=t.every(x=>/^[1-8]$/.test(x));
$('seqTxt').style.borderColor=ok?'':'var(--err)';if(ok){seq=t.map(Number);dirty();renderSeq(true)}}
function radio(n){const e=document.querySelector(`input[name=${n}]:checked`);return e?e.value:null}
function setRadio(n,v){const e=document.querySelector(`input[name=${n}][value="${v}"]`);if(e)e.checked=true}
function modeUi(){const p=radio('mode')=='PULSO';$('gapBox').classList.toggle('hide',!p);$('blinkBox').classList.toggle('hide',p);
$('modeHelp').textContent=p?'Pulso: há um tempo desligado entre os quadros.':'Troca: o próximo quadro acende quando o anterior apaga. Um quadro repetido em seguida dá uma apagada curta.'}
function repUi(){$('cntBox').classList.toggle('hide',radio('rep')!='N_TIMES')}
function dirty(){isDirty=true;$('dirtyMsg').textContent='Alterações não salvas';$('saveMsg').classList.add('hide')}
function fill(){seq=cfg.sequence.slice();setRadio('mode',cfg.mode);setRadio('rep',cfg.repeat);
$('on').value=ms2s(cfg.on_ms);$('gap').value=ms2s(cfg.gap_ms);$('cyc').value=ms2s(cfg.cycle_pause_ms);$('blink').value=ms2s(cfg.blink_off_ms);
$('rc').value=cfg.repeat_count;$('auto').checked=cfg.autostart;
$('names').innerHTML=cfg.channel_names.map((n,i)=>`<div><label for="n${i}">CH${i+1}</label><input id="n${i}" type="text" value="${esc(n)}" oninput="dirty()"></div>`).join('');
modeUi();repUi();buildChans();renderSeq();isDirty=false;$('dirtyMsg').textContent=''}
async function load(){cfg=await api('/api/config');fill()}
async function save(){const b={sequence:seq,mode:radio('mode'),repeat:radio('rep'),on_ms:s2ms($('on').value),gap_ms:s2ms($('gap').value),
cycle_pause_ms:s2ms($('cyc').value),blink_off_ms:s2ms($('blink').value),repeat_count:parseInt($('rc').value)||0,autostart:$('auto').checked,
channel_names:[...Array(8).keys()].map(i=>$('n'+i).value)};
for(const k of['on_ms','gap_ms','cycle_pause_ms','blink_off_ms'])if(isNaN(b[k])){msg('saveMsg',false,'Preencha todos os tempos.');return}
try{await api('/api/config',b);cfg=await api('/api/config');fill();msg('saveMsg',true,'Configuração salva.')}catch(e){if(e)msg('saveMsg',false,e.message)}}
async function cmd(c){try{await api('/api/'+c,{});poll()}catch(e){}}
async function man(i){if(!st||st.state!='PARADO')return;try{await api('/api/manual',{mask:st.manual^(1<<(i-1))});poll()}catch(e){if(e)alert(e.message)}}
function lamps(mask){let h='';for(let i=1;i<=8;i++)h+=`<div class="lamp${mask>>(i-1)&1?' on':''}" title="${esc(name(i))}">${i}</div>`;$('lamps').innerHTML=h}
async function poll(){try{st=await api('/api/state')}catch(e){return}
const run=st.state!='PARADO';$('pill').textContent=STN[st.state]||st.state;
$('prog').textContent=run?`Passo ${st.step}/${st.total} · CH${st.channel} · ciclo ${st.cycle}${st.cycles?'/'+st.cycles:''}`:'';
lamps(st.relays);$('relErr').classList.toggle('hide',st.relays_ok);
for(let i=1;i<=8;i++){const b=$('m'+i);if(b){b.disabled=run;b.classList.toggle('primary',!!(st.manual>>(i-1)&1))}}
renderSeq(document.activeElement==$('seqTxt'));netState()}
function netState(){if(!st)return;const n=st.net;$('netState').innerHTML=
`<span>Ethernet</span><span>${n.eth_ip?n.eth_ip:(n.eth_link?'com cabo, sem IP':'sem cabo')}</span>`+
`<span>WiFi</span><span>${n.wifi_ip||'desconectado'}</span>`+
`<span>AP</span><span>${n.ap?('ligado: '+esc(n.ap_ssid)):'desligado'}</span>`+
`<span>Home Assistant</span><span>${n.mqtt?'conectado':'desconectado'}</span>`+
`<span>Endereço</span><span>http://${esc(n.hostname)}.local</span>`}
function ipFields(p){return['ip','gateway','netmask','dns'].map(k=>`<div><label for="${p}_${k}">${{ip:'IP',gateway:'Gateway',netmask:'Máscara',dns:'DNS (opcional)'}[k]}</label><input id="${p}_${k}" type="text" inputmode="decimal"></div>`).join('')}
function ipUi(p){$(p+'Static').classList.toggle('hide',radio(p+'Mode')!='static')}
async function loadNet(){const n=await api('/api/network');$('host').value=n.hostname;
$('hostHelp').textContent='A página fica em http://'+n.hostname+'.local';
$('ethStatic').innerHTML=ipFields('eth');$('wifiStatic').innerHTML=ipFields('wifi');
for(const p of['eth','wifi']){const o=n[p];$(p+'En').checked=o.enabled;setRadio(p+'Mode',o.dhcp?'dhcp':'static');
for(const k of['ip','gateway','netmask','dns'])$(p+'_'+k).value=o[k]||'';ipUi(p)}
$('ssid').value=n.wifi.ssid;$('wpass').value='';$('wpass').placeholder=n.wifi.pass_set?'•••••• (mantida)':'';
$('mb').value=n.mqtt.broker;$('mp').value=n.mqtt.port;$('mu').value=n.mqtt.user;$('mw').value='';$('mw').placeholder=n.mqtt.pass_set?'•••••• (mantida)':''}
async function saveNet(){const ifc=p=>{const o={enabled:$(p+'En').checked,dhcp:radio(p+'Mode')!='static'};for(const k of['ip','gateway','netmask','dns'])o[k]=$(p+'_'+k).value.trim();return o};
const b={hostname:$('host').value.trim(),eth:ifc('eth'),wifi:ifc('wifi'),mqtt:{broker:$('mb').value.trim(),port:parseInt($('mp').value)||1883,user:$('mu').value}};
b.wifi.ssid=$('ssid').value;if($('wpass').value)b.wifi.pass=$('wpass').value;if($('mw').value)b.mqtt.pass=$('mw').value;
try{await api('/api/network',b);msg('netMsg',true,'Salvo. A placa está reiniciando; reconecte em alguns segundos.')}catch(e){if(e)msg('netMsg',false,e.message)}}
function bars(r){return r>=-55?'▂▄▆█':r>=-67?'▂▄▆':r>=-78?'▂▄':'▂'}
async function scan(){$('scanBtn').disabled=true;$('scanMsg').textContent='Procurando…';$('scanList').innerHTML='';
try{for(let i=0;i<20;i++){const r=await api('/api/wifi/scan');if(!r.scanning){
const l=r.networks.sort((a,b)=>b.rssi-a.rssi);$('scanMsg').textContent=l.length?l.length+' redes encontradas':'Nenhuma rede encontrada';
$('scanList').innerHTML=l.map(n=>`<button class="net" style="display:flex;width:100%;justify-content:space-between;margin-top:6px" data-s="${esc(n.ssid)}" onclick="pick(this.dataset.s)"><span>${esc(n.ssid)}${n.secure?' 🔒':''}</span><span class="muted">${bars(n.rssi)} ${n.rssi} dBm</span></button>`).join('');
break}await new Promise(r=>setTimeout(r,1000))}}catch(e){if(e)$('scanMsg').textContent=e.message}
$('scanBtn').disabled=false}
function pick(s){$('ssid').value=s;$('wifiEn').checked=true;$('scanList').innerHTML='';$('scanMsg').textContent='Rede escolhida: '+s;$('wpass').focus()}
async function savePass(){try{await api('/api/password',{current:$('pc').value,new:$('pn').value,confirm:$('pf').value});
$('pc').value=$('pn').value=$('pf').value='';msg('passMsg',true,'Senha alterada.')}catch(e){if(e)msg('passMsg',false,e.message)}}
load().then(poll);setInterval(poll,1000);
</script></body></html>)HTML";
