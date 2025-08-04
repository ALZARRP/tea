#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <ArduinoOTA.h>
#include <vector>
#include <map>

// --- Pinout & Hardware Configuration ---
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   15

// --- Global Objects & State ---
TFT_eSPI tft = TFT_eSPI();
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
struct CheatState {
    // Shared states
    bool aimbot_active = false; float aimbot_fov = 10.0f; float aimbot_smooth = 5.0f;
    bool esp_active = false; bool esp_box = true; bool esp_line = false;
    bool no_recoil = false; float recoil_percent = 0.0f;
    // Game specific
    bool wz_uav = false; bool fn_autobuild = false; bool r6_unlock = false;
    bool bo6_omni_aim = false; bool ap_glow = false;
};
CheatState cheats;
String currentGame = "None";
int activeCheatsCount = 0;
std::map<uint32_t, unsigned long> lastMessageTime;
const char* ap_ssid = "VENDOR.ME-ULTIMATE";
const char* ap_password = "cyberpunk123";

// --- HTML & UI ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" /><title>VENDOR.ME NEON</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
    <meta name="apple-mobile-web-app-capable" content="yes" /><meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />
    <link rel="apple-touch-icon" href="https://i.imgur.com/2aP5p52.png">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@700&family=Roboto:wght@400;700&display=swap');
        :root { --neon-glow: #00ffff; --bg-color: #00001a; --text-color: #00ffff; --container-bg: rgba(10, 10, 30, 0.8); --border-color: rgba(0, 255, 255, 0.5); }
        body, html { margin: 0; padding: 0; font-family: 'Roboto', sans-serif; background: var(--bg-color); color: var(--text-color); }
        .container { max-width: 800px; margin: 0 auto; padding: 15px; backdrop-filter: blur(10px); }
        h1, h2, h3 { font-family: 'Orbitron', sans-serif; text-align: center; color: var(--neon-glow); text-shadow: 0 0 5px #fff, 0 0 10px var(--neon-glow), 0 0 15px var(--neon-glow); }
        .view { display: none; } .view.active { display: flex; flex-direction: column; height: 95vh; }
        #game-selection { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; }
        .game-card { background: var(--container-bg); border: 1px solid var(--border-color); border-radius: 10px; padding: 20px; width: 45%; text-align: center; cursor: pointer; transition: all 0.3s ease; }
        .game-card:hover { border-color: var(--neon-glow); box-shadow: 0 0 15px var(--neon-glow); }
        .cheat-container { flex-grow: 1; overflow-y: auto; display: grid; grid-template-columns: 1fr 1fr; gap: 20px; padding-right: 10px; }
        .cheat-category { background: var(--container-bg); padding: 15px; border-radius: 8px; border: 1px solid var(--border-color); }
        .cheat-item { margin-bottom: 15px; } .cheat-item label { display: block; margin-bottom: 5px; }
        button { width: 100%; padding: 14px; margin-top: 20px; font-size: 1.1rem; font-family: var(--font-main); color: #000; background: var(--neon-glow); border: 2px solid var(--neon-glow); border-radius: 15px; cursor: pointer; box-shadow: 0 0 10px var(--neon-glow); }
        input { width: 100%; padding: 12px; margin: 8px 0; background: rgba(0,0,0,0.5); border: 1px solid var(--border-color); border-radius: 8px; color: var(--text-color); }
        .toggle-switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .ios-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .ios-slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .ios-slider { background-color: var(--neon-glow); }
        input:checked + .ios-slider:before { transform: translateX(26px); }
        input[type=range] { width: 100%; -webkit-appearance: none; background: #333; height: 5px; border-radius: 5px; }
        input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; background: var(--neon-glow); cursor: pointer; border-radius: 50%; }
    </style>
</head>
<body>
    <div class="container">
        <section id="loginView" class="view active"><h1>V.E.N.D.O.R</h1><input id="user" placeholder="Username"><input id="pass" type="password" placeholder="Password"><button id="loginBtn">Connect</button></section>
        <section id="selectionView" class="view"><h1>SELECT DEPLOYMENT</h1><div id="game-selection"></div></section>
        <section id="cheatView" class="view"><h2 id="gameTitle"></h2><div id="cheat-container"></div><button id="backToSelectionBtn">Disconnect</button></section>
    </div>
    <script>
        const games = {
            warzone: { name: "Warzone", cheats: [ { id: 'aimbot_active', label: 'Aimbot', type: 'toggle', category: 'Combat' }, { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 10 } }, { id: 'aimbot_smooth', label: 'Aimbot Smooth', type: 'slider', category: 'Combat', params: { min: 1, max: 20, default: 5 } }, { id: 'esp_active', label: 'Player ESP', type: 'toggle', category: 'Visuals' }, { id: 'esp_box', label: 'ESP Box', type: 'toggle', category: 'Visuals' }, { id: 'esp_line', label: 'ESP Line', type: 'toggle', category: 'Visuals' }, { id: 'no_recoil', label: 'No Recoil', type: 'toggle', category: 'Combat' }, { id: 'recoil_percent', label: 'Recoil %', type: 'slider', category: 'Combat', params: { min: 0, max: 100, default: 0 } }, { id: 'wz_wallhack', label: 'Solid Wallhack', type: 'toggle', category: 'Visuals' }, { id: 'wz_uav', label: 'Constant UAV', type: 'toggle', category: 'Utility' }, { id: 'wz_speed', label: 'Speed Hack', type: 'toggle', category: 'Movement' }, { id: 'wz_slidecancel', label: 'Auto Slide-Cancel', type: 'toggle', category: 'Movement' }, { id: 'wz_heartbeat', label: 'Unlimited Heartbeat', type: 'toggle', category: 'Utility' }, { id: 'wz_armor', label: 'Auto-Plate', type: 'toggle', category: 'Utility' }, { id: 'wz_cash', label: 'Cash Multiplier', type: 'toggle', category: 'Utility' }, { id: 'wz_radar', label: 'Mini-Map Radar', type: 'toggle', category: 'Visuals' }, { id: 'wz_silent', label: 'Silent Aim', type: 'toggle', category: 'Combat' }, { id: 'wz_spinbot', label: 'Spin Bot', type: 'toggle', category: 'Combat' }, { id: 'wz_longslide', label: 'Long Slide', type: 'toggle', category: 'Movement' }, { id: 'wz_thirdperson', label: 'Third Person', type: 'toggle', category: 'Visuals' } ] },
            fortnite: { name: "Fortnite", cheats: [ { id: 'aimbot_active', label: 'Aimbot', type: 'toggle', category: 'Combat' }, { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 15 } }, { id: 'fn_build', label: 'Auto 1x1 Build', type: 'toggle', category: 'Utility' }, { id: 'fn_edit', label: 'Instant Edit', type: 'toggle', category: 'Utility' }, { id: 'esp_active', label: 'Player ESP', type: 'toggle', category: 'Visuals' }, { id: 'esp_box', label: 'Player Box ESP', type: 'toggle', category: 'Visuals' }, { id: 'fn_chams', label: 'Player Chams', type: 'toggle', category: 'Visuals' }, { id: 'fn_fly', label: 'Fly Hack', type: 'toggle', category: 'Movement' }, { id: 'fn_vbuck', label: 'V-Buck Generator', type: 'toggle', category: 'Utility' }, { id: 'no_recoil', label: 'No Bloom', type: 'toggle', category: 'Combat' }, { id: 'fn_doublepump', label: 'Double Pump', type: 'toggle', category: 'Combat' }, { id: 'fn_invis', label: 'Invisibility', type: 'toggle', category: 'Utility' }, { id: 'fn_item_esp', label: 'Item ESP', type: 'toggle', category: 'Visuals' }, { id: 'fn_weakpoint', label: 'Auto Weak-Point', type: 'toggle', category: 'Combat' }, { id: 'fn_revive', label: 'Instant Revive', type: 'toggle', category: 'Utility' }, { id: 'fn_speed', label: 'Player Speed', type: 'slider', category: 'Movement', params: { min: 1, max: 5, default: 1 } }, { id: 'fn_bhop', label: 'Bunny Hop', type: 'toggle', category: 'Movement' }, { id: 'fn_noreload', label: 'No Reload', type: 'toggle', category: 'Combat' }, { id: 'fn_carfly', label: 'Car Fly', type: 'toggle', category: 'Movement' }, { id: 'fn_emote', label: 'Emote Anywhere', type: 'toggle', category: 'Utility' } ] },
            r6: { name: "Rainbow Six", cheats: [ { id: 'r6_aimbot', label: 'Aimbot', type: 'toggle', category: 'Combat' }, { id: 'r6_esp', label: 'ESP', type: 'toggle', category: 'Visuals' }, { id: 'r6_norecoil', label: 'No Recoil', type: 'toggle', category: 'Combat' }, { id: 'r6_nospread', label: 'No Spread', type: 'toggle', category: 'Combat' }, { id: 'r6_unlock', label: 'Unlock All', type: 'toggle', category: 'Utility' }, { id: 'r6_chams', label: 'Player Chams', type: 'toggle', category: 'Visuals' }, { id: 'r6_noflash', label: 'No Flash', type: 'toggle', category: 'Visuals' }, { id: 'r6_speed', label: 'Speed Hack', type: 'toggle', category: 'Movement' }, { id: 'r6_teleport', label: 'Teleport', type: 'toggle', category: 'Movement' }, { id: 'r6_drone', label: 'Drone View', type: 'toggle', category: 'Utility' }, { id: 'r6_fov', label: 'FOV Changer', type: 'slider', category: 'Visuals', params: { min: 75, max: 120, default: 90 } }, { id: 'r6_wallbang', label: 'Wallbang', type: 'toggle', category: 'Combat' }, { id: 'r6_silentwalk', label: 'Silent Walk', type: 'toggle', category: 'Movement' }, { id: 'r6_glow', label: 'Player Glow', type: 'toggle', category: 'Visuals' }, { id: 'r6_rapidfire', label: 'Rapid Fire', type: 'toggle', category: 'Combat' }, { id: 'r6_autopeek', label: 'Auto Peek', type: 'toggle', category: 'Utility' }, { id: 'r6_knife', label: 'Long Knife', type: 'toggle', category: 'Combat' }, { id: 'r6_reload', label: 'Fast Reload', type: 'toggle', category: 'Combat' }, { id: 'r6_friendly', label: 'No Team Damage', type: 'toggle', category: 'Utility' }, { id: 'r6_scan', label: 'Scanline ESP', type: 'toggle', category: 'Visuals' } ] },
            apex: { name: "Apex Legends", cheats: [ { id: 'ap_aimbot', label: 'Aimbot', type: 'toggle', category: 'Combat' }, { id: 'ap_esp', label: 'ESP', type: 'toggle', category: 'Visuals' }, { id: 'ap_glow', label: 'Player Glow', type: 'toggle', category: 'Visuals' }, { id: 'ap_norecoil', label: 'No Recoil', type: 'toggle', category: 'Combat' }, { id: 'ap_bhop', label: 'Bunny Hop', type: 'toggle', category: 'Movement' }, { id: 'ap_thirdperson', label: 'Third Person', type: 'toggle', category: 'Visuals' }, { id: 'ap_heirloom', label: 'Heirloom Unlocker', type: 'toggle', category: 'Utility' }, { id: 'ap_wallclimb', label: 'Infinite Wall Climb', type: 'toggle', category: 'Movement' }, { id: 'ap_fastheal', label: 'Fast Heal', type: 'toggle', category: 'Utility' }, { id: 'ap_radar', label: '2D Radar', type: 'toggle', category: 'Visuals' }, { id: 'ap_skydive', label: 'Skydive Control', type: 'toggle', category: 'Movement' }, { id: 'ap_fakelag', label: 'Fake Lag', type: 'toggle', category: 'Utility' }, { id: 'ap_trigger', label: 'Trigger Bot', type: 'toggle', category: 'Combat' }, { id: 'ap_charge', label: 'Force Charge Rifle', type: 'toggle', category: 'Combat' }, { id: 'ap_loot', label: 'Loot ESP', type: 'toggle', category: 'Visuals' }, { id: 'ap_silent', label: 'Silent Aim', type: 'toggle', category: 'Combat' }, { id: 'ap_strafe', label: 'Auto Strafe', type: 'toggle', category: 'Movement' }, { id: 'ap_skin', label: 'Skin Changer', type: 'toggle', category: 'Utility' }, { id: 'ap_reconnect', label: 'Instant Reconnect', type: 'toggle', category: 'Utility' }, { id: 'ap_fov', label: 'FOV Slider', type: 'slider', category: 'Visuals', params: { min: 90, max: 140, default: 110 } } ] },
            bo6: { name: "Black Ops 6", cheats: [ { id: 'bo6_aimbot', label: 'Omni-Aim', type: 'toggle', category: 'Combat' }, { id: 'bo6_esp', label: 'Cognitive ESP', type: 'toggle', category: 'Visuals' }, { id: 'bo6_norecoil', label: 'Recoil Control', type: 'toggle', category: 'Combat' }, { id: 'bo6_rapidfire', label: 'Rapid Fire', type: 'toggle', category: 'Combat' }, { id: 'bo6_speed', label: 'Super Sprint', type: 'toggle', category: 'Movement' }, { id: 'bo6_wallhack', label: 'Quantum Wallhack', type: 'toggle', category: 'Visuals' }, { id: 'bo6_uav', label: 'Persistent UAV', type: 'toggle', category: 'Utility' }, { id: 'bo6_fastads', label: 'Fast ADS', type: 'toggle', category: 'Combat' }, { id: 'bo6_autospot', label: 'Auto Spot', type: 'toggle', category: 'Utility' }, { id: 'bo6_fov', label: 'FOV Changer', type: 'slider', category: 'Visuals', params: { min: 80, max: 130, default: 100 } }, { id: 'bo6_slide', label: 'Infinite Slide', type: 'toggle', category: 'Movement' }, { id: 'bo6_lagswitch', label: 'Lag Switch', type: 'toggle', category: 'Utility' }, { id: 'bo6_chams', label: 'Player Chams', type: 'toggle', category: 'Visuals' }, { id: 'bo6_autofire', label: 'Auto Fire', type: 'toggle', category: 'Combat' }, { id: 'bo6_headshots', label: 'Headshot Only', type: 'toggle', category: 'Combat' }, { id: 'bo6_autodoor', label: 'Auto Open Doors', type: 'toggle', category: 'Utility' }, { id: 'bo6_nospread', label: 'No Spread', type: 'toggle', category: 'Combat' }, { id: 'bo6_drop', label: 'Auto Drop Shot', type: 'toggle', category: 'Movement' }, { id: 'bo6_autoping', label: 'Auto Ping', type: 'toggle', category: 'Utility' }, { id: 'bo6_radar', label: 'Enhanced Radar', type: 'toggle', category: 'Visuals' } ] }
        };
        const $ = (s) => document.querySelector(s);
        function switchView(toView) { document.querySelectorAll('.view').forEach(v => v.classList.remove('active')); toView.classList.add('active'); }
        function sendMessage(msg) { ws.send(JSON.stringify(msg)); }
        function showAlert(message) { const a = document.createElement('div'); a.style.cssText = 'padding:10px 20px;background:#00ffff;color:#000;border-radius:5px;margin-bottom:5px;animation:fadeOut 3s forwards;'; a.textContent = message; $('#alert-container').appendChild(a); setTimeout(() => a.remove(), 3000); }
        function selectGame(gameKey) {
            $('#gameTitle').textContent = games[gameKey].name;
            const container = $('#cheat-container'); container.innerHTML = "";
            const categories = {};
            games[gameKey].cheats.forEach(cheat => {
                if (!categories[cheat.category]) categories[cheat.category] = [];
                categories[cheat.category].push(cheat);
            });
            for (const category in categories) {
                const catDiv = document.createElement('div'); catDiv.className = 'cheat-category';
                const title = document.createElement('h3'); title.textContent = category; catDiv.appendChild(title);
                categories[category].forEach(cheat => {
                    const item = document.createElement("div"); item.className = 'cheat-item';
                    const label = document.createElement("label"); label.textContent = cheat.label; item.appendChild(label);
                    if (cheat.type === 'toggle') {
                        const toggle = document.createElement("label"); toggle.className = 'toggle-switch';
                        const input = document.createElement("input"); input.type = "checkbox";
                        input.onchange = () => { sendMessage({type: 'setCheat', cheatId: cheat.id, value: input.checked}); showAlert(`${cheat.label} ${input.checked ? 'ON' : 'OFF'}`); };
                        toggle.append(input, Object.assign(document.createElement('span'), {className:'ios-slider'})); item.appendChild(toggle);
                    } else if (cheat.type === 'slider') {
                        const slider = document.createElement("input"); slider.type = "range";
                        slider.min = cheat.params.min; slider.max = cheat.params.max; slider.value = cheat.params.default;
                        slider.oninput = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: parseFloat(slider.value)});
                        item.appendChild(slider);
                    }
                    catDiv.appendChild(item);
                });
                container.appendChild(catDiv);
            }
            switchView($('#cheatView'));
            sendMessage({type: 'selectGame', game: gameKey});
        }
        window.onload = () => {
            const ws = new WebSocket(`ws://${window.location.hostname}/ws`);
            $('#loginBtn').onclick = () => { switchView($('#selectionView')); };
            const gameSelection = $('#game-selection');
            Object.keys(games).forEach(key => {
                const card = document.createElement('div'); card.className = 'game-card';
                card.innerHTML = `<h3>${games[key].name}</h3>`;
                card.onclick = () => selectGame(key);
                gameSelection.appendChild(card);
            });
            $('#backToSelectionBtn').onclick = () => switchView($('#selectionView'));
        };
    </script>
</body>
</html>
)rawliteral";

// --- CYD DISPLAY CODE ---
void updateCydDisplay() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    String ip_addr = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : "Connecting...";
    tft.drawString("IP: " + ip_addr, 5, 10);
    tft.drawString("Clients: " + String(ws.count()), 5, 30);
    activeCheatsCount = 0;
    if(cheats.aimbot) activeCheatsCount++; if(cheats.esp) activeCheatsCount++; if(cheats.noRecoil) activeCheatsCount++;
    tft.drawString("Active Cheats: " + String(activeCheatsCount), 5, 50);
    tft.drawString("Game: " + currentGame, 5, 70);
    tft.drawString("VENDOR.ME Ultimate", 5, 100);
}

// --- WebSocket & Simulation Logic ---
void handleWebSocketMessage(AsyncWebSocketClient *client, char *data) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, data);
    const char* type = doc["type"];
    if (strcmp(type, "setCheat") == 0) {
        const char* cheatId = doc["cheatId"];
        // This is a simplified handler. A full implementation would have a case for all 100+ cheats.
        if (strstr(cheatId, "aimbot")) cheats.aimbot = doc["value"];
        else if (strstr(cheatId, "esp")) cheats.esp = doc["value"];
        else if (strstr(cheatId, "recoil")) cheats.noRecoil = doc["value"];
    } else if (strcmp(type, "selectGame") == 0) {
        currentGame = doc["game"].as<String>();
    }
}

void onEvent(AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) { Serial.printf("Client #%u connected\n", c->id()); }
    else if (type == WS_EVT_DISCONNECT) { Serial.printf("Client #%u disconnected\n", c->id()); }
    else if (type == WS_EVT_DATA) {
        AwsFrameInfo *i=(AwsFrameInfo*)arg;
        if(i->final&&i->index==0&&i->len==len&&i->opcode==WS_TEXT){data[len]=0;handleWebSocketMessage(c,(char*)data);}
    }
}

// --- Main Setup & Loop ---
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  WiFi.softAP(ap_ssid, ap_password);
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send_P(200, "text/html", index_html); });
  ArduinoOTA.begin();
  server.begin();
}

unsigned long lastDisplayUpdateTime = 0;
void loop() {
  ws.cleanupClients();
  ArduinoOTA.handle();
  if (millis() - lastDisplayUpdateTime > 1000) {
    lastDisplayUpdateTime = millis();
    updateCydDisplay();
  }
}
