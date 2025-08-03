#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "time.h"
#include <vector>
#include <map>

// --- Display Pin Configuration ---
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL   15

TFT_eSPI tft = TFT_eSPI();

// --- Simulation & State ---
struct Player { int id; String name; int health; int armor; bool isTarget; bool isVisible; };
std::vector<Player> lobby;
unsigned long lastPlayerActionTime = 0;
String currentGame = "None";
int activeCheatCount = 0;

struct CheatState {
    bool aimbot_active = false; float aimbot_fov = 10.0f; float aimbot_smooth = 5.0f;
    bool esp_active = false; bool esp_box = true; bool esp_line = false;
    bool no_recoil = false; float recoil_percent = 0.0f;
};
CheatState cheats;

// --- Web Server & Networking ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;
std::map<uint32_t, unsigned long> lastMessageTime;
const char* ap_ssid = "VENDOR.ME-ULTIMATE";
const char* ap_password = "cyberpunk123";

// --- HTML & UI ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <title>VENDOR.ME Ultimate Console</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
    <meta name="apple-mobile-web-app-capable" content="yes" />
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />
    <link rel="apple-touch-icon" href="https://i.imgur.com/2aP5p52.png">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@700&family=Roboto:wght@400;700&display=swap');
        :root {
          --font-main: 'Orbitron', sans-serif; --font-body: 'Roboto', sans-serif;
          --bg-color: #1a001a; --primary-glow: #ff66cc; --secondary-glow: #ff99dd;
          --container-bg: linear-gradient(135deg, #330033 0%, #660066 100%);
          --text-color: #ff66cc; --input-bg: #330033; --button-bg: #ff66cc;
          --button-text: #330033;
        }
        body, html { margin: 0; padding: 0; font-family: var(--font-body); background: var(--bg-color); color: var(--text-color); }
        .container { max-width: 800px; margin: 0 auto; padding: 10px; }
        h1, h2, h3 { font-family: var(--font-main); text-align: center; color: var(--primary-glow); text-shadow: 0 0 10px var(--primary-glow); }
        .view { display: none; } .view.active { display: block; }
        #game-selection { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; }
        .game-card { background: #1c1c1c; border: 2px solid #444; border-radius: 10px; padding: 20px; width: 45%; text-align: center; cursor: pointer; transition: all 0.3s ease; }
        .game-card:hover { border-color: var(--primary-glow); box-shadow: 0 0 15px var(--primary-glow); }
        .cheat-container { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        .cheat-category { background: #1c1c1c; padding: 15px; border-radius: 8px; border: 1px solid #444; }
        .cheat-item { margin-bottom: 15px; }
        .cheat-item label { display: block; margin-bottom: 5px; }
        .toggle-switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .ios-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .ios-slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .ios-slider { background-color: var(--primary-glow); }
        input:checked + .ios-slider:before { transform: translateX(26px); }
        input[type=range] { width: 100%; -webkit-appearance: none; background: #333; height: 5px; border-radius: 5px; }
        input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; background: var(--primary-glow); cursor: pointer; border-radius: 50%; }
        button { width: 100%; padding: 14px; margin-top: 20px; font-size: 1.1rem; font-family: var(--font-main); color: var(--button-text); background: var(--button-bg); border: 2px solid var(--primary-glow); border-radius: 15px; cursor: pointer; }
    </style>
</head>
<body>
    <div class="container">
        <section id="selectionView" class="view active"><h1>SELECT DEPLOYMENT</h1><div id="game-selection"></div></section>
        <section id="cheatView" class="view"><h2 id="gameTitle"></h2><div id="cheat-container"></div><button id="backToSelectionBtn">Disconnect</button></section>
    </div>
    <script>
        const games = {
            warzone: {
                name: "Warzone",
                cheats: [
                    { id: 'aimbot_active', label: 'Enable Aimbot', type: 'toggle', category: 'Combat' }, { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 10 } }, { id: 'aimbot_smooth', label: 'Aimbot Smooth', type: 'slider', category: 'Combat', params: { min: 1, max: 20, default: 5 } },
                    { id: 'esp_active', label: 'Enable ESP', type: 'toggle', category: 'Visuals' }, { id: 'esp_box', label: 'ESP Box', type: 'toggle', category: 'Visuals' }, { id: 'esp_line', label: 'ESP Line', type: 'toggle', category: 'Visuals' },
                    { id: 'no_recoil', label: 'No Recoil', type: 'toggle', category: 'Combat' }, { id: 'recoil_percent', label: 'Recoil %', type: 'slider', category: 'Combat', params: { min: 0, max: 100, default: 0 } },
                    { id: 'wz_wallhack', label: 'Solid Wallhack', type: 'toggle', category: 'Visuals' }, { id: 'wz_uav', label: 'Constant UAV', type: 'toggle', category: 'Utility' },
                    { id: 'wz_speed', label: 'Speed Hack', type: 'toggle', category: 'Movement' }, { id: 'wz_slidecancel', label: 'Auto Slide-Cancel', type: 'toggle', category: 'Movement' },
                    { id: 'wz_heartbeat', label: 'Unlimited Heartbeat', type: 'toggle', category: 'Utility' }, { id: 'wz_armor', label: 'Auto-Plate', type: 'toggle', category: 'Utility' },
                    { id: 'wz_cash', label: 'Cash Multiplier', type: 'toggle', category: 'Utility' }, { id: 'wz_radar', label: 'Mini-Map Radar', type: 'toggle', category: 'Visuals' },
                    { id: 'wz_silent', label: 'Silent Aim', type: 'toggle', category: 'Combat' }, { id: 'wz_spinbot', label: 'Spin Bot', type: 'toggle', category: 'Combat' },
                    { id: 'wz_longslide', label: 'Long Slide', type: 'toggle', category: 'Movement' }, { id: 'wz_thirdperson', label: 'Third Person', type: 'toggle', category: 'Visuals' }
                ]
            },
            fortnite: {
                name: "Fortnite",
                cheats: [
                    { id: 'aimbot_active', label: 'Enable Aimbot', type: 'toggle', category: 'Combat' }, { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 15 } },
                    { id: 'fn_build', label: 'Auto 1x1 Build', type: 'toggle', category: 'Utility' }, { id: 'fn_edit', label: 'Instant Edit', type: 'toggle', category: 'Utility' },
                    { id: 'esp_active', label: 'Enable ESP', type: 'toggle', category: 'Visuals' }, { id: 'esp_box', label: 'Player Box ESP', type: 'toggle', category: 'Visuals' },
                    { id: 'fn_chams', label: 'Player Chams', type: 'toggle', category: 'Visuals' }, { id: 'fn_fly', label: 'Fly Hack', type: 'toggle', category: 'Movement' },
                    { id: 'fn_vbuck', label: 'V-Buck Generator', type: 'toggle', category: 'Utility' }, { id: 'no_recoil', label: 'No Bloom', type: 'toggle', category: 'Combat' },
                    { id: 'fn_doublepump', label: 'Double Pump', type: 'toggle', category: 'Combat' }, { id: 'fn_invis', label: 'Invisibility', type: 'toggle', category: 'Utility' },
                    { id: 'fn_item_esp', label: 'Item ESP', type: 'toggle', category: 'Visuals' }, { id: 'fn_weakpoint', label: 'Auto Weak-Point', type: 'toggle', category: 'Combat' },
                    { id: 'fn_revive', label: 'Instant Revive', type: 'toggle', category: 'Utility' }, { id: 'fn_speed', label: 'Player Speed', type: 'slider', category: 'Movement', params: { min: 1, max: 5, default: 1 } },
                    { id: 'fn_bhop', label: 'Bunny Hop', type: 'toggle', category: 'Movement' }, { id: 'fn_noreload', label: 'No Reload', type: 'toggle', category: 'Combat' },
                    { id: 'fn_carfly', label: 'Car Fly', type: 'toggle', category: 'Movement' }, { id: 'fn_emote', label: 'Emote Anywhere', type: 'toggle', category: 'Utility' }
                ]
            }
        };
        const $ = (s) => document.querySelector(s);
        function switchView(toView) { document.querySelectorAll('.view').forEach(v => v.classList.remove('active')); toView.classList.add('active'); }
        function sendMessage(msg) { ws.send(JSON.stringify(msg)); }
        function selectGame(gameKey) {
            $('#gameTitle').textContent = games[gameKey].name;
            buildCheatList(gameKey);
            switchView($('#cheatView'));
            sendMessage({type: 'selectGame', game: gameKey});
        }
        function buildCheatList(gameKey) {
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
                    const label = document.createElement("label"); label.textContent = cheat.label;
                    item.appendChild(label);
                    if (cheat.type === 'toggle') {
                        const toggle = document.createElement("label"); toggle.className = 'toggle-switch';
                        const input = document.createElement("input"); input.type = "checkbox";
                        input.onchange = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: input.checked});
                        toggle.append(input, Object.assign(document.createElement('span'), {className:'ios-slider'}));
                        item.appendChild(toggle);
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
        }
        window.onload = () => {
            const gameSelection = $('#game-selection');
            Object.keys(games).forEach(key => {
                const card = document.createElement('div'); card.className = 'game-card';
                card.innerHTML = `<h3>${games[key].name}</h3>`;
                card.onclick = () => selectGame(key);
                gameSelection.appendChild(card);
            });
            $('#backToSelectionBtn').onclick = () => { sendMessage({type: 'selectGame', game: 'None'}); switchView($('#selectionView')); };
            switchView($('#selectionView'));
        };
    </script>
</body>
</html>
)rawliteral";

void updateCydDisplay() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String ip_addr = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    tft.drawString("IP: " + ip_addr, 5, 10);
    tft.drawString("Clients: " + String(ws.count()), 5, 30);
    activeCheatCount = (cheats.aimbot_active ? 1 : 0) + (cheats.esp_active ? 1 : 0) + (cheats.no_recoil ? 1 : 0);
    tft.drawString("Active Cheats: " + String(activeCheatCount), 5, 50);
    tft.drawString("Game: " + currentGame, 5, 70);
}

void handleWebSocketMessage(AsyncWebSocketClient *client, char *data) {
    if (lastMessageTime.count(client->id()) && (millis() - lastMessageTime[client->id()] < 100)) return;
    lastMessageTime[client->id()] = millis();
    StaticJsonDocument<256> doc;
    deserializeJson(doc, data);
    const char* type = doc["type"];
    if (strcmp(type, "setCheat") == 0) {
        const char* cheatId = doc["cheatId"];
        if (strcmp(cheatId, "aimbot_active") == 0) cheats.aimbot_active = doc["value"];
        else if (strcmp(cheatId, "esp_active") == 0) cheats.esp_active = doc["value"];
        else if (strcmp(cheatId, "no_recoil") == 0) cheats.no_recoil = doc["value"];
        else if (strcmp(cheatId, "aimbot_fov") == 0) cheats.aimbot_fov = doc["value"];
        else if (strcmp(cheatId, "aimbot_smooth") == 0) cheats.aimbot_smooth = doc["value"];
        else if (strcmp(cheatId, "recoil_percent") == 0) cheats.recoil_percent = doc["value"];
    } else if (strcmp(type, "selectGame") == 0) {
        currentGame = doc["game"].as<String>();
    }
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) { lastMessageTime[client->id()] = 0; }
  else if (type == WS_EVT_DISCONNECT) { lastMessageTime.erase(client->id()); }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0; handleWebSocketMessage(client, (char*)data);
    }
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  WiFi.softAP(ap_ssid, ap_password);
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html); });
  ArduinoOTA.begin();
  server.begin();
}

unsigned long lastDisplayUpdate = 0;
void loop() {
  ws.cleanupClients();
  ArduinoOTA.handle();
  if (millis() - lastDisplayUpdate > 1000) {
    lastDisplayUpdate = millis();
    updateCydDisplay();
  }
}
