#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "time.h"
#include <map>

// --- Display Configuration for CYD (Cheap Yellow Display) ---
#define TFT_MODULE_ST7789
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO -1
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#define TFT_BL 15
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);

const char* ap_ssid = "VENDOR.ME-CHEATAP";
const char* ap_password = "cyberpunk123";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;
std::map<uint32_t, unsigned long> lastMessageTime;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" >
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
  <title>VENDOR.ME Console</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@700&family=Roboto:wght@400;700&display=swap');
    :root {
      --font-main: 'Orbitron', sans-serif; --font-body: 'Roboto', sans-serif;
      --bg-color: #1a001a; --primary-glow: #ff66cc; --secondary-glow: #ff99dd;
      --container-bg: linear-gradient(135deg, #330033 0%, #660066 100%);
      --text-color: #ff66cc; --input-bg: #330033; --button-bg: #ff66cc;
      --button-text: #330033; --button-hover-bg: #ff99dd; --button-hover-text: #550055;
    }
    .theme-matrix {
      --bg-color: #000000; --primary-glow: #00ff00; --secondary-glow: #33ff33;
      --container-bg: linear-gradient(135deg, #021a02 0%, #042a04 100%);
      --text-color: #00ff00; --input-bg: #032003; --button-bg: #00ff00;
      --button-text: #002200; --button-hover-bg: #33ff33; --button-hover-text: #004400;
    }
    .theme-classic {
      --bg-color: #111; --primary-glow: #00aaff; --secondary-glow: #33ccff;
      --container-bg: linear-gradient(135deg, #222 0%, #444 100%);
      --text-color: #00aaff; --input-bg: #333; --button-bg: #00aaff;
      --button-text: #111; --button-hover-bg: #33ccff; --button-hover-text: #222;
    }
    * { box-sizing: border-box; }
    body, html { margin: 0; padding: 0; height: 100%; width: 100%; font-family: var(--font-main); background: var(--bg-color); color: var(--text-color); -webkit-tap-highlight-color: transparent; user-select: none; overflow: hidden; transition: background-color 0.5s ease; }
    body::before { content: ''; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: linear-gradient(rgba(255, 255, 255, 0) 50%, rgba(255, 255, 255, 0.05) 50%); background-size: 100% 4px; opacity: 0.2; animation: scanlines 10s linear infinite; pointer-events: none; z-index: -1; }
    @keyframes scanlines { from { background-position: 0 0; } to { background-position: 0 -400px; } }
    .container { display: flex; flex-direction: column; height: 100%; max-width: 500px; margin: 0 auto; padding: 15px; border: 2px solid var(--primary-glow); border-radius: 15px; background: var(--container-bg); box-shadow: 0 0 10px var(--primary-glow), 0 0 30px var(--primary-glow), inset 0 0 20px var(--primary-glow); transition: all 0.5s ease; position: relative; }
    h1, h2, h3 { margin: 0 0 15px 0; text-align: center; text-shadow: 0 0 10px var(--primary-glow), 0 0 20px var(--primary-glow); }
    h3 {font-size: 1rem; margin-bottom: 5px;}
    .password-container { position: relative; display: flex; align-items: center; }
    .password-toggle { position: absolute; right: 15px; cursor: pointer; font-size: 1.2rem; opacity: 0.7; }
    input, select, textarea { width: 100%; padding: 12px 15px; margin: 8px 0; background: var(--input-bg); border: 2px solid var(--primary-glow); border-radius: 12px; color: var(--text-color); font-size: 1rem; font-family: var(--font-body); outline: none; transition: border-color 0.3s ease; -webkit-appearance: none; }
    input:focus, select:focus, textarea:focus { border-color: var(--secondary-glow); }
    button { width: 100%; padding: 14px 20px; margin: 10px 0; font-size: 1.1rem; font-family: var(--font-main); color: var(--button-text); background: var(--button-bg); border: 2px solid var(--primary-glow); border-radius: 15px; cursor: pointer; box-shadow: 0 0 8px var(--primary-glow), 0 0 15px var(--primary-glow); transition: all 0.3s ease; user-select: none; }
    button:hover { background: var(--button-hover-bg); color: var(--button-hover-text); }
    .view { display: none; flex-direction: column; flex-grow: 1; overflow-y: hidden; }
    .view.active { display: flex; }
    .cheat-list-container { flex-grow: 1; overflow-y: auto; }
    .cheat-item { background: rgba(0,0,0,0.2); border: 1px solid var(--primary-glow); border-radius: 8px; padding: 10px 15px; display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
    .header-bar { display: flex; justify-content: space-between; align-items: center; padding: 0 5px 10px 5px; }
    .status-indicator { width: 12px; height: 12px; background-color: #ff0000; border-radius: 50%; box-shadow: 0 0 8px #ff0000; animation: blink 1.5s infinite; }
    .status-indicator.connected { background-color: #00ff00; animation: none; }
    @keyframes blink { 50% { opacity: 0.5; } }
    .stealth { background: #e0e0e0 !important; border: 1px solid #ccc !important; color: #333 !important; font-family: Arial, sans-serif !important; }
    .stealth h1, .stealth h2, .stealth h3 { text-shadow: none !important; color: #333 !important; }
    .stealth button, .stealth input, .stealth select { background: #fff !important; border: 1px solid #ccc !important; color: #333 !important; box-shadow: none !important; }
    .stealth .profile-avatar, .stealth .status-indicator, .stealth .vendor-text { display: none; }
    .secure-mode .vendor-text { display: none; }
    .guest-mode .cheat-item input, .guest-mode .cheat-item button, #panicBtn, #saveCustomCheatBtn, #changePassBtn, #saveThemeBtn { pointer-events: none; opacity: 0.5; }
    .toggle-switch { position: relative; display: inline-block; width: 50px; height: 26px; }
    .toggle-switch input { opacity: 0; width: 0; height: 0; }
    .slider-switch { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 26px; }
    .slider-switch:before { position: absolute; content: ""; height: 20px; width: 20px; left: 3px; bottom: 3px; background-color: white; transition: .4s; border-radius: 50%; }
    input:checked + .slider-switch { background-color: var(--primary-glow); }
    input:checked + .slider-switch:before { transform: translateX(24px); }
  </style>
</head>
<body>
  <div class="container" id="mainContainer">
    <section id="loginView" class="view active"><h1 class="vendor-text">VENDOR.ME LOGIN</h1><input type="text" id="user" placeholder="Username" autocomplete="username"><div class="password-container"><input type="password" id="pass" placeholder="Password"><span id="passToggle" class="password-toggle">👁️</span></div><div style="display:flex;align-items:center;margin-bottom:10px;"><input type="checkbox" id="rememberMe" style="width:auto;margin:0 10px 0 0;"><label for="rememberMe">Remember Me</label></div><div style="display:flex;gap:10px;"><button id="loginBtn">ACCESS</button><button id="guestLoginBtn">GUEST</button></div></section>
    <section id="selectionView" class="view"><div class="header-bar"><div id="statusIndicator" class="status-indicator"></div><div id="clock" class="clock"></div></div><div class="profile"><div class="profile-avatar"></div><div id="profileName" class="profile-name">Guest</div></div><h2>SELECT GAME</h2><div class="game-list"></div><div style="display:flex;gap:10px;margin-top:10px;"><button id="systemBtn" class="back-button" style="width:33%;margin:0;">SYSTEM</button><button id="wifiBtn" class="back-button" style="width:33%;margin:0;">WIFI</button><button id="profileBtn" class="back-button" style="width:33%;margin:0;">PROFILE</button></div><button id="addCheatNavBtn" class="back-button" style="margin-top:5px;">ADD CUSTOM CHEAT</button><button id="logoutBtn" class="back-button">LOG OUT</button></section>
    <section id="cheatView" class="view"><h2 id="gameTitle">GAME CHEATS</h2><div class="cheat-list-container" id="cheatListContainer"></div><button id="panicBtn" class="panic-button">PANIC</button><button id="backToSelectionBtn" class="back-button">BACK</button></section>
    <section id="systemView" class="view"><h2>SYSTEM INFO</h2><div id="systemStats" style="flex-grow:1;white-space:pre-wrap;">Loading...</div><p style="font-size:0.8rem;text-align:center;">Update via Arduino IDE (Network Port)</p><button id="backToSelectionFromSystem" class="back-button">BACK</button></section>
    <section id="wifiView" class="view"><h2>WIFI SETTINGS</h2><p>Connect to a local network.</p><input type="text" id="wifiSSID" placeholder="WiFi SSID"><input type="password" id="wifiPass" placeholder="WiFi Password"><button id="saveWifiBtn">SAVE & RESTART</button><button id="backToSelectionFromWifi" class="back-button">BACK</button></section>
    <section id="profileView" class="view"><h2>PROFILE & SETTINGS</h2><div style="flex-grow:1;overflow-y:auto;padding-right:5px;"><h3>Change Password</h3><input type="password" id="oldPass" placeholder="Old Password"><input type="password" id="newPass" placeholder="New Password"><button id="changePassBtn">Change</button><hr><h3>Secure Mode</h3><label class="toggle-switch"><input type="checkbox" id="secureModeToggle"><span></span></label></div><button id="backToSelectionFromProfile" class="back-button">BACK</button></section>
    <section id="customCheatView" class="view"><h2>ADD CUSTOM CHEAT</h2><div style="flex-grow:1;overflow-y:auto;"><input type="text" id="customCheatName" placeholder="Cheat Name"><select id="customCheatType"><option value="toggle">Toggle</option></select><input type="text" id="customCheatCategory" placeholder="Category"><button id="saveCustomCheatBtn">SAVE CHEAT</button></div><button id="backToSelectionFromCustom" class="back-button">BACK</button></section>
  </div>
  <script>
    const games = { custom: { name: "Custom Cheats", cheats: [] } };
    let currentUser = null, currentGameKey = null, cheatStates = {}, websocket;
    const $ = (s) => document.querySelector(s);
    const elements = { mainContainer: $('#mainContainer'), loginView: $('#loginView'), selectionView: $('#selectionView'), cheatView: $('#cheatView'), systemView: $('#systemView'), wifiView: $('#wifiView'), profileView: $('#profileView'), customCheatView: $('#customCheatView'), profileName: $('#profileName'), logoutBtn: $('#logoutBtn') };
    function switchView(toView) { document.querySelectorAll('.view').forEach(v => v.classList.remove('active')); toView.classList.add('active'); }
    function sendMessage(msg) { websocket.send(JSON.stringify(msg)); }
    function setupWebSocket() {
        websocket = new WebSocket(`ws://${window.location.hostname}/ws`);
        websocket.onopen = () => { $('#statusIndicator').classList.add('connected'); sendMessage({type:"getInitData"}); };
        websocket.onclose = () => { $('#statusIndicator').classList.remove('connected'); setTimeout(setupWebSocket, 2000); };
        websocket.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if(data.type === 'initData'){
                games.custom.cheats = data.customCheats;
                const gameList = $('.game-list'); gameList.innerHTML = '';
                const baseGames = { r6: { name: "Rainbow Six Siege" }, minecraft: { name: "Minecraft" } };
                const allGames = {...baseGames, ... (games.custom.cheats.length > 0 ? {custom: games.custom} : {})};
                Object.keys(allGames).forEach(key => {
                    const btn = document.createElement('button'); btn.className = 'game-btn';
                    btn.textContent = allGames[key].name; btn.onclick = () => selectGame(key);
                    gameList.appendChild(btn);
                });
            } else if (data.type === 'loginResponse'){
                if(data.success){
                    currentUser = data.user; elements.profileName.textContent = currentUser;
                    elements.mainContainer.classList.toggle('guest-mode', data.guest || false);
                    switchView(elements.selectionView);
                } else { alert("Login Failed: " + data.message); }
            } else if (data.type === 'systemStats') {
                $('#systemStats').textContent = `Uptime: ${data.uptime}s\nFree Heap: ${data.freeHeap} bytes\nIP: ${data.ip}`;
            }
        };
    }
    function selectGame(gameKey) {
        currentGameKey = gameKey;
        $('#gameTitle').textContent = games[gameKey] ? games[gameKey].name : "CHEATS";
        buildCheatList(); switchView(elements.cheatView);
    }
    function buildCheatList() {
        const list = $('#cheatList'); list.innerHTML = "";
        const cheats = (games[currentGameKey] || {}).cheats || [];
        if (cheats.length === 0) { list.innerHTML = "<p>No cheats available.</p>"; return; }
        cheats.forEach(cheat => {
            const item = document.createElement("div"); item.className = 'cheat-item';
            const label = document.createElement("span"); label.textContent = cheat.label;
            const toggle = document.createElement("label"); toggle.className = 'toggle-switch';
            const input = document.createElement("input"); input.type = "checkbox";
            input.onchange = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: input.checked});
            toggle.append(input, Object.assign(document.createElement('span'),{className:'slider-switch'}));
            item.append(label, toggle); list.appendChild(item);
        });
    }
    window.onload = () => {
        setupWebSocket();
        $('#loginBtn').onclick = () => sendMessage({type: 'login', user: $('#user').value, pass: $('#pass').value});
        $('#guestLoginBtn').onclick = () => sendMessage({type: 'guestLogin'});
        elements.logoutBtn.onclick = () => { currentUser = null; switchView(elements.loginView); };
        $('#systemBtn').onclick = () => switchView(elements.systemView);
        $('#wifiBtn').onclick = () => switchView(elements.wifiView);
        $('#profileBtn').onclick = () => switchView(elements.profileView);
        $('#addCheatNavBtn').onclick = () => switchView(elements.customCheatView);
        $('#saveWifiBtn').onclick = () => sendMessage({type:'setWifi', ssid:$('#wifiSSID').value, pass:$('#wifiPass').value});
        $('#saveCustomCheatBtn').onclick = () => {
            const cheat = { id: 'c_' + Date.now(), label: $('#customCheatName').value, type: $('#customCheatType').value, category: $('#customCheatCategory').value };
            sendMessage({type: 'saveCustomCheat', cheat: cheat});
            alert('Custom cheat saved!'); switchView(elements.selectionView);
        };
        $('#secureModeToggle').onchange = (e) => elements.mainContainer.classList.toggle('secure-mode', e.target.checked);
        document.querySelectorAll('.back-button').forEach(b => { if(!b.onclick) b.onclick = () => switchView(elements.selectionView); });
    };
  </script>
</body>
</html>
)rawliteral";

void handleSerialCommand(String cmd) {
    if (cmd == "help") Serial.println("Commands: help, reboot, clear_wifi, clear_cheats");
    else if (cmd == "reboot") ESP.restart();
    else if (cmd == "clear_wifi") { preferences.begin("wifi-creds", false); preferences.clear(); preferences.end(); ESP.restart(); }
    else if (cmd == "clear_cheats") { preferences.begin("custom-cheats", false); preferences.clear(); preferences.end(); Serial.println("Custom cheats cleared."); }
    else Serial.println("Unknown command.");
}

void handleWebSocketMessage(AsyncWebSocketClient *client, char *data) {
    if (lastMessageTime.count(client->id()) && (millis() - lastMessageTime[client->id()] < 100)) return;
    lastMessageTime[client->id()] = millis();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) return;
    const char* type = doc["type"];
    if (strcmp(type, "login") == 0) {
        if (strcmp(doc["user"], "admin") == 0 && strcmp(doc["pass"], "password") == 0) client->text("{\"type\":\"loginResponse\",\"success\":true,\"user\":\"admin\"}");
        else client->text("{\"type\":\"loginResponse\",\"success\":false,\"message\":\"Invalid credentials\"}");
    } else if (strcmp(type, "guestLogin") == 0) {
        client->text("{\"type\":\"loginResponse\",\"success\":true,\"user\":\"Guest\",\"guest\":true}");
    } else if (strcmp(type, "getInitData") == 0) {
        preferences.begin("custom-cheats", true);
        String cheatsJson = preferences.getString("cheats", "[]");
        preferences.end();
        String response = "{\"type\":\"initData\",\"customCheats\":" + cheatsJson + "}";
        client->text(response);
    } else if (strcmp(type, "saveCustomCheat") == 0) {
        preferences.begin("custom-cheats", false);
        String cheatsJson = preferences.getString("cheats", "[]");
        StaticJsonDocument<1024> cheatsDoc;
        deserializeJson(cheatsDoc, cheatsJson);
        JsonArray cheats = cheatsDoc.as<JsonArray>();
        cheats.add(doc["cheat"]);
        String newCheatsJson;
        serializeJson(cheats, newCheatsJson);
        preferences.putString("cheats", newCheatsJson);
        preferences.end();
    }
}

void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) { lastMessageTime[client->id()] = 0; Serial.printf("Client #%u connected\n", client->id()); }
  else if (type == WS_EVT_DISCONNECT) { lastMessageTime.erase(client->id()); Serial.printf("Client #%u disconnected\n", client->id()); }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0; handleWebSocketMessage(client, (char*)data);
    }
  }
}

void setupDisplay() { tft.init(); tft.setRotation(1); pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH); }

void setup() {
  Serial.begin(115200);
  setupDisplay();
  preferences.begin("wifi-creds", true);
  String sta_ssid = preferences.getString("ssid", "");
  String sta_pass = preferences.getString("pass", "");
  preferences.end();
  WiFi.mode(WIFI_AP_STA);
  if (sta_ssid.length() > 0) {
    WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
    if (WiFi.waitForConnectResult(10000) != WL_CONNECTED) WiFi.mode(WIFI_AP);
  } else { WiFi.softAP(ap_ssid, ap_password); }
  if (MDNS.begin("vendor")) { Serial.println("MDNS responder started"); }
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html); });
  ArduinoOTA.begin();
  server.begin();
}

void loop() {
  ws.cleanupClients();
  ArduinoOTA.handle();
  if (Serial.available() > 0) { handleSerialCommand(Serial.readStringUntil('\n')); }
}
