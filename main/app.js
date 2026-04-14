var mode='tcp', ws=null, ledState=false;
var wsFc=0, wsLastT=Date.now(), wsFps=0;
var canvas=document.getElementById('ws-canvas'), ctx=canvas?canvas.getContext('2d'):null;
var streamUrl='http://'+location.hostname+':8081/stream/tcp';
/* Defer stream load to avoid overloading ESP32 connections on page load */
setTimeout(function(){document.getElementById('tcp-view').src=streamUrl;},500);

function switchMode(m) {
    mode = m;
    document.querySelectorAll('.tab').forEach(function(t,i){t.className='tab'+(i===(m==='tcp'?0:1)?' active':'')});
    document.getElementById('tcp-view').style.display = m==='tcp'?'block':'none';
    canvas.style.display = m==='ws'?'block':'none';
    document.getElementById('hud-ws').style.display = m==='ws'?'inline':'none';
    document.querySelectorAll('.ws-only').forEach(function(el){el.style.display=m==='ws'?'inline-block':'none'});
    if (m==='ws' && !ws) connectWs();
    if (m==='tcp' && ws) { ws.close(); ws=null; }
}

function connectWs() {
    ws = new WebSocket('ws://'+location.host+'/ws/stream');
    ws.binaryType = 'arraybuffer';
    ws.onopen = function(){ document.getElementById('hud-ws').textContent='WS: connected'; };
    ws.onmessage = function(e) {
        if (typeof e.data==='string') {
            try { var m=JSON.parse(e.data); if(m.heap_free) document.getElementById('si-heap').textContent=Math.round(m.heap_free/1024)+'KB'; } catch(x){}
            return;
        }
        var blob=new Blob([e.data],{type:'image/jpeg'}), url=URL.createObjectURL(blob), img=new Image();
        img.onload=function(){ if(canvas.width!==img.width||canvas.height!==img.height){canvas.width=img.width;canvas.height=img.height;} ctx.drawImage(img,0,0); URL.revokeObjectURL(url); wsFc++; };
        img.src=url;
    };
    ws.onerror = function(){ document.getElementById('hud-ws').textContent='WS: error'; };
    ws.onclose = function(){ document.getElementById('hud-ws').textContent='WS: disconnected'; ws=null; if(mode==='ws') setTimeout(connectWs,2000); };
}
function wsSend(o){ if(ws&&ws.readyState===1) ws.send(JSON.stringify(o)); }

function fmtUptime(s) {
    var d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);
    return (d>0?d+'d ':'')+(h>0?h+'h ':'')+m+'m';
}

function updateStatus() {
    fetch('/api/status').then(function(r){return r.json()}).then(function(d){
        var fps = mode==='ws' ? wsFps : d.fps;
        document.getElementById('hud-fps').textContent='FPS: '+fps.toFixed(1);
        document.getElementById('hud-temp').innerHTML='TEMP: '+d.temperature.toFixed(1)+'&deg;C';
        ledState=d.led_state;
        var btn=document.getElementById('led-btn');
        btn.className='btn '+(ledState?'on':'off');
        btn.innerHTML=ledState?'&#x1F7E2; LED ON':'&#x1F534; LED OFF';
        document.getElementById('si-uptime').textContent=fmtUptime(d.uptime);
        document.getElementById('si-heap').textContent=Math.round(d.heap_free/1024)+'KB';
        document.getElementById('si-hmin').textContent=Math.round(d.heap_min/1024)+'KB';
        document.getElementById('si-rssi').textContent=d.rssi+'dBm';
        document.getElementById('si-wifi').textContent=d.wifi_connected?'Connected':'Disconnected';
        document.getElementById('si-temp').textContent=d.temperature.toFixed(1)+'\u00B0C';
        if(d.version) document.getElementById('fw-ver').textContent=' v'+d.version;
        document.getElementById('status').textContent='Connected | '+new Date().toLocaleTimeString();
    }).catch(function(){ document.getElementById('status').textContent='Connection lost...'; });
}

function updateWsFps() {
    var now=Date.now(), dt=now-wsLastT;
    if(dt>=1000){ wsFps=wsFc*1000/dt; wsFc=0; wsLastT=now; }
}

function toggleLed() {
    fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({state:'toggle'})})
    .then(function(r){return r.json()}).then(function(){updateStatus()}).catch(function(){});
}

function takeSnapshot() {
    fetch('/api/snapshot').then(function(r){return r.blob()}).then(function(b){
        var a=document.createElement('a');
        a.href=URL.createObjectURL(b);
        a.download='esp32cam_'+new Date().toISOString().replace(/[:.]/g,'-')+'.jpg';
        a.click(); URL.revokeObjectURL(a.href);
    }).catch(function(){alert('Snapshot failed');});
}

function startOta() {
    var url=document.getElementById('ota-url').value.trim();
    if(!url){alert('Enter firmware URL');return;}
    if(!confirm('Start OTA update? Device will reboot.'))return;
    document.getElementById('ota-btn').disabled=true;
    fetch('/api/ota',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:url})})
    .then(function(r){return r.json()}).then(function(d){
        if(d.status==='started') pollOta();
        else{document.getElementById('ota-status').textContent='Error: '+(d.error||'unknown');document.getElementById('ota-btn').disabled=false;}
    }).catch(function(){document.getElementById('ota-status').textContent='Request failed';document.getElementById('ota-btn').disabled=false;});
}
function pollOta() {
    fetch('/api/ota/status').then(function(r){return r.json()}).then(function(d){
        document.getElementById('ota-bar').style.width=d.progress+'%';
        document.getElementById('ota-status').textContent=d.status+' ('+d.progress+'%)';
        if(d.in_progress) setTimeout(pollOta,500);
        else document.getElementById('ota-btn').disabled=false;
    }).catch(function(){document.getElementById('ota-status').textContent='Device rebooting...';});
}

setInterval(updateStatus,1000);
setInterval(updateWsFps,500);
updateStatus();

function setCam(key, val) {
    var body = {};
    body[key] = (typeof val === 'boolean' || val === true || val === false) ? val : parseInt(val);
    if (key === 'hmirror' || key === 'vflip') body[key] = !!val;
    var el = document.getElementById('v-'+key);
    if (el) el.textContent = val;
    fetch('/api/camera',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)})
    .then(function(r){return r.json()}).then(function(d){ loadCamSettings(d); }).catch(function(){});
}
function loadCamSettings(d) {
    ['brightness','contrast','saturation','sharpness'].forEach(function(k){
        var el = document.getElementById('cam-'+k);
        var vl = document.getElementById('v-'+k);
        if (el && d[k] !== undefined) { el.value = d[k]; if(vl) vl.textContent = d[k]; }
    });
    if (d.hmirror !== undefined) document.getElementById('cam-hmirror').checked = d.hmirror;
    if (d.vflip !== undefined) document.getElementById('cam-vflip').checked = d.vflip;
}
fetch('/api/camera').then(function(r){return r.json()}).then(loadCamSettings).catch(function(){});

function fmtBytes(b){if(b<1024)return b+'B';if(b<1048576)return(b/1024).toFixed(1)+'KB';if(b<1073741824)return(b/1048576).toFixed(1)+'MB';return(b/1073741824).toFixed(2)+'GB';}

function sdRefresh(){
    fetch('/api/sd/status').then(function(r){return r.json()}).then(function(d){
        if(!d.mounted){document.getElementById('sd-no-card').style.display='block';document.getElementById('sd-content').style.display='none';document.getElementById('sd-badge').textContent='(no card)';return;}
        document.getElementById('sd-no-card').style.display='none';document.getElementById('sd-content').style.display='block';
        document.getElementById('sd-badge').textContent='(mounted)';
        document.getElementById('sd-name').textContent=d.name||'SD';
        document.getElementById('sd-total').textContent=fmtBytes(d.total_bytes);
        document.getElementById('sd-free').textContent=fmtBytes(d.free_bytes);
    }).catch(function(){document.getElementById('sd-badge').textContent='(error)';});
    fetch('/api/sd/list').then(function(r){return r.json()}).then(function(d){
        var c=document.getElementById('sd-files');
        if(!d.files||d.files.length===0){c.innerHTML='<div style="color:#666">No files</div>';return;}
        var h='<table style="width:100%;border-collapse:collapse">';
        h+='<tr style="color:#0ff;border-bottom:1px solid #444"><td>Name</td><td style="text-align:right">Size</td><td></td></tr>';
        d.files.forEach(function(f){
            h+='<tr style="border-bottom:1px solid #333"><td>'+f.name+'</td><td style="text-align:right;color:#0f0">'+fmtBytes(f.size)+'</td>';
            h+='<td style="text-align:right"><a href="/api/sd/file/'+f.name+'" target="_blank" style="color:#0ff;text-decoration:none;margin-right:8px">&#x1F517;</a>';
            h+='<span style="color:#f55;cursor:pointer" onclick="sdDelete(\''+f.name+'\')">&#x1F5D1;</span></td></tr>';
        });
        h+='</table>';c.innerHTML=h;
    }).catch(function(){});
}
function sdCapture(){
    document.getElementById('sd-cap-btn').disabled=true;
    fetch('/api/sd/capture',{method:'POST'}).then(function(r){return r.json()}).then(function(d){
        document.getElementById('sd-cap-btn').disabled=false;
        if(d.filename) sdRefresh(); else alert(d.error||'Capture failed');
    }).catch(function(){document.getElementById('sd-cap-btn').disabled=false;alert('Capture failed');});
}
function sdDelete(name){
    if(!confirm('Delete '+name+'?'))return;
    fetch('/api/sd/delete',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({filename:name})})
    .then(function(r){return r.json()}).then(function(){sdRefresh();}).catch(function(){});
}
setTimeout(sdRefresh, 2000);
fetch('/api/system/info').then(function(r){return r.json()}).then(function(d){
    if(d.chip) document.getElementById('si-chip').textContent=d.chip.model+' ('+d.chip.cores+'-core)';
    if(d.idf_version) document.getElementById('si-idf').textContent=d.idf_version;
    if(d.memory) document.getElementById('si-psram').textContent=Math.round(d.memory.psram_free/1024)+'KB / '+Math.round(d.memory.psram_total/1024)+'KB';
    if(d.task_count) document.getElementById('si-tasks').textContent=d.task_count;
}).catch(function(){});
