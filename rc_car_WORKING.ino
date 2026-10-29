#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// --- Motor Pins ---
const int IN1 = 26, IN2 = 33, IN3 = 13, IN4 = 4;
// --- LED Pins ---
const int L1 = 18, L2 = 23;
bool lightsOn = true;

// --- SPEED SETTINGS ---
const int MAX_SPEED = 125;  // 0 to 255, lower this number to make the car slower!
const int RAMP_STEP = 10;   // how much speed to add per step (lower = smoother acceleration)
const int RAMP_DELAY = 15;  // milliseconds between steps (lower = faster acceleration)

// --- WiFi AP ---
const char* AP_SSID = "ESP32_RC_Car";
const char* AP_PASS = "rcCar1234";

WebServer server(80);

// --- State Management ---
String targetDirection = "S"; // 'F', 'B', 'LF', 'RF', 'LB', 'RB' or 'S'
String currentDirection = "S";
int currentSpeed = 0;
unsigned long lastRampTime = 0;
unsigned long lastMoveMs = 0;
const unsigned long CMD_TIMEOUT = 800; // watchdog (stop if connection drops)

// --- Web UI ---
const char PAGE[] = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>RC Car</title>
<style>
  body{
    margin:0;
    background:#111;
    color:#eee;
    font-family:system-ui,sans-serif;
    height:100vh;
    display:flex;
    flex-direction:column;
    align-items:center;
    justify-content:center;
    overflow:hidden;
    user-select:none;
    -webkit-user-select:none
  }
  #status{
    position:fixed;
    top:14px;
    display:flex;
    align-items:center;
    gap:8px;
    font-size:14px
  }
  #dot{
    width:12px;
    height:12px;
    border-radius:50%;
    background:#555;
    transition:background .2s
  }
  .pad{
    display:grid;
    grid-template-columns:repeat(3,90px);
    grid-template-rows:repeat(3,90px);
    gap:10px
  }
  button{
    font-size:28px;
    background:#222;
    color:#eee;
    border:2px solid #444;
    border-radius:16px;
    touch-action:none;
    -webkit-tap-highlight-color:transparent
  }
  button:active{
    background:#0a7
  }
  #lights{
    grid-column:1/4;
    font-size:18px
  }
  .empty{visibility:hidden}
  #hint{
    position:fixed;
    bottom:14px;
    font-size:12px;
    color:#777;
    text-align:center;
    padding:0 16px
  }
</style></head><body>
<div id="status"><span id="dot"></span><span id="statustext">Connecting…</span></div>
<div class="pad">
  <button data-cmd="LF">▲◀</button><button data-cmd="F">▲</button><button data-cmd="RF">▶▲</button>
  <button class="empty"></button><button data-cmd="S">■</button><button class="empty"></button>
  <button data-cmd="LB">▼◀</button><button data-cmd="B">▼</button><button data-cmd="RB">▶▼</button>
  <button id="lights">Lights: OFF</button>
</div>
<div id="hint">Hold a direction to drive, release to stop. Click middle button to stop as well.</div>
<script>
const dot=document.getElementById('dot'), st=document.getElementById('statustext');
let held=null, repeatTimer=null, lightsOnFlag=true;

function setStatus(ok){
  dot.style.background = ok ? '#0f0' : '#f33';
  st.textContent = ok ? 'Connected' : 'Disconnected — join "ESP32_RC_Car"';
}
async function send(c){
  try{ await fetch('/cmd?c='+c, {cache:'no-store'}); return true; }
  catch(e){ setStatus(false); return false; }
}
function press(cmd){
  held=cmd; send(cmd);
  clearInterval(repeatTimer);
  repeatTimer=setInterval(()=>{ if(held) send(held); }, 120);
}
function release(){
  if(!held) return;
  clearInterval(repeatTimer); repeatTimer=null;
  send('S'); held=null;
}
setInterval(async ()=>{
  try{ const r=await fetch('/ping',{cache:'no-store'}); setStatus(r.ok); }
  catch(e){ setStatus(false); }
},1000);
setStatus(false);

document.querySelectorAll('button[data-cmd]').forEach(b=>{
  const c=b.dataset.cmd;
  if(c==='S') return;
  b.addEventListener('pointerdown',e=>{e.preventDefault(); b.setPointerCapture(e.pointerId); press(c);});
  b.addEventListener('pointerup',e=>{e.preventDefault(); release();});
  b.addEventListener('pointercancel',release);
  b.addEventListener('lostpointercapture',release);
});
document.querySelector('button[data-cmd="S"]').addEventListener('pointerdown',e=>{
  e.preventDefault(); release(); send('S');
});
const lb=document.getElementById('lights');
lb.addEventListener('pointerdown',async e=>{
  e.preventDefault(); await send('T');
  lightsOnFlag=!lightsOnFlag; lb.textContent='Lights: '+(lightsOnFlag?'OFF':'ON');
});
document.addEventListener('visibilitychange',()=>{ if(document.hidden){ release(); send('S'); }});
window.addEventListener('blur',()=>{ release(); send('S'); });
window.addEventListener('beforeunload',()=>{ navigator.sendBeacon('/cmd?c=S'); });
</script></body></html>
)rawliteral";

//actually applies the PWM signal to the pins
void applyMotorPWM(String dir, int speed) {
  if(dir=="B"){ // Forward
    analogWrite(IN1, speed); analogWrite(IN2, 0);
    analogWrite(IN3, 0); analogWrite(IN4, speed);
  }else if(dir== "F"){// Reverse
      analogWrite(IN1, 0); analogWrite(IN2, speed);
      analogWrite(IN3, speed); analogWrite(IN4, 0);
  }else if(dir== "LF"){ // Left
      analogWrite(IN1, 0); analogWrite(IN2, speed);
      analogWrite(IN3, 0);     analogWrite(IN4, 0);
  }else if(dir== "RF"){// Right
      analogWrite(IN1, 0);     analogWrite(IN2, 0);
      analogWrite(IN3, speed); analogWrite(IN4, 0);
  }else if(dir== "RB"){
      analogWrite(IN1, speed); analogWrite(IN2, 0);
      analogWrite(IN3, 0);     analogWrite(IN4, 0);
  }else if(dir=="LB"){
      analogWrite(IN1, 0); analogWrite(IN2, 0);
      analogWrite(IN3, 0);     analogWrite(IN4, speed);
  }else{
      analogWrite(IN1, 0); analogWrite(IN2, 0);
      analogWrite(IN3, 0); analogWrite(IN4, 0);
  }
}

void stopMotors() {
  targetDirection = "S";
  // don't set speed to 0 instantly, the updateMotors() loop will smoothly ramp it down
}

void toggleLights(){
  lightsOn = !lightsOn; 
  digitalWrite(L1, lightsOn?HIGH:LOW); 
  digitalWrite(L2, lightsOn?HIGH:LOW); 
}

//smooth acceleration and deceleration
void updateMotors() {
  if (millis() - lastRampTime > RAMP_DELAY) {
    lastRampTime = millis();
    //changing direction -> must brake first
    if (targetDirection != currentDirection) {
      if (currentSpeed > 0) {
        currentSpeed -= RAMP_STEP; //ramp down
        if (currentSpeed < 0) currentSpeed = 0;
      } else {
        currentDirection = targetDirection; //safe to change direction
      }
    } 
    //going the same direction -> adjust speed to target
    else {
      int targetSpeed = (targetDirection == "S") ? 0 : MAX_SPEED;
      if (currentSpeed < targetSpeed) {
        currentSpeed += RAMP_STEP; //ramp up
        if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
      } else if (currentSpeed > targetSpeed) {
        currentSpeed -= RAMP_STEP; //ramp down
        if (currentSpeed < targetSpeed) currentSpeed = targetSpeed;
      }
    }
    applyMotorPWM(currentDirection, currentSpeed);
  }
}

void handleRoot(){ 
  server.send(200, "text/html", PAGE); 
}

void handleCmd(){
  if(!server.hasArg("c")){ server.send(400,"text/plain","missing c"); return; }
  //read the whole String instead of just the first char
  String cmd = server.arg("c");
  
  if (cmd == "F" || cmd == "B" || cmd == "LF" || cmd == "RF" || cmd == "LB" || cmd == "RB") {
    targetDirection = cmd; 
    lastMoveMs = millis(); 
  } else if (cmd == "S") {
    stopMotors(); 
  } else if (cmd == "T") {
    toggleLights(); 
  } else {
    server.send(400,"text/plain","bad cmd"); 
    return;
  }
  server.send(200,"text/plain","ok");
}

void handlePing(){ server.send(200,"text/plain","pong"); }

void setup(){
  Serial.begin(115200);

  //setup PWM pins (ESP32 handles analogWrite natively)
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(L1,OUTPUT);  pinMode(L2,OUTPUT);
  
  applyMotorPWM("S", 0);
  digitalWrite(L1,HIGH); digitalWrite(L2,HIGH);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP %s  IP: %s\n", ok ? "up" : "FAILED", WiFi.softAPIP().toString().c_str());

  if(MDNS.begin("esp32car")){
    MDNS.addService("http","tcp",80);
    Serial.println("mDNS: http://esp32car.local");
  }

  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/ping", handlePing);
  server.onNotFound([]{ server.send(404,"text/plain","not found"); });
  server.begin();

  Serial.println("Open http://192.168.4.1  (or http://esp32car.local)");
}

void loop(){
  server.handleClient();
  updateMotors(); //smooth acceleration

  //if we're moving but no command arrived, stop
  if (targetDirection != "S" && millis() - lastMoveMs > CMD_TIMEOUT){
    stopMotors(); 
    Serial.println("watchdog: stopped");
  }
  delay(1);
}
