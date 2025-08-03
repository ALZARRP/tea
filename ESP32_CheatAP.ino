#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <TFT_eSPI.h>

// --- Display & Simulation Globals ---
TFT_eSPI tft = TFT_eSPI();
struct Player { int id; String name; int health; bool isTarget; };
std::vector<Player> lobby;
struct CheatState { bool aimbot = false; bool esp = false; };
CheatState cheats;
String currentGame = "None";
unsigned long lastPlayerActionTime = 0;

// --- Web Server and WebSocket ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <title>VENDOR.ME Console</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
    <meta name="apple-mobile-web-app-capable" content="yes" />
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />
    <link rel="apple-touch-icon" href="https://i.imgur.com/2aP5p52.png">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@700&family=Roboto:wght@400;700&display=swap');
        :root { --primary-glow: #ff66cc; --container-bg: linear-gradient(135deg, #330033 0%, #660066 100%); --text-color: #ff66cc; --button-bg: #ff66cc; --button-text: #330033; }
        body, html { margin: 0; padding: 0; font-family: 'Roboto', sans-serif; background: #1a001a; color: var(--text-color); }
        .container { max-width: 500px; margin: 0 auto; padding: 15px; }
        h1, h2 { font-family: 'Orbitron', sans-serif; text-align: center; color: var(--primary-glow); text-shadow: 0 0 10px var(--primary-glow); }
        .view { display: none; } .view.active { display: block; }
        .game-card { background: #1c1c1c; border: 2px solid #444; border-radius: 10px; padding: 20px; text-align: center; cursor: pointer; transition: all 0.3s ease; margin-bottom: 10px;}
        .game-card:hover { border-color: var(--primary-glow); box-shadow: 0 0 15px var(--primary-glow); }
        .cheat-item { background: rgba(0,0,0,0.2); border: 1px solid var(--primary-glow); border-radius: 8px; padding: 10px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; }
        .toggle-switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .ios-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .ios-slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .ios-slider { background-color: var(--primary-glow); }
        input:checked + .ios-slider:before { transform: translateX(26px); }
        button { width: 100%; padding: 14px; margin: 10px 0; font-size: 1.1rem; font-family: 'Orbitron', sans-serif; color: var(--button-text); background: var(--button-bg); border: 2px solid var(--primary-glow); border-radius: 15px; cursor: pointer; }
    </style>
</head>
<body>
    <div class="container">
        <section id="selectionView" class="view active"><h1>SELECT GAME</h1><div id="game-selection"></div></section>
        <section id="cheatView" class="view"><h2 id="gameTitle"></h2><div id="cheatList"></div><button id="backToSelectionBtn">Disconnect</button></section>
    </div>
    <script>
        const games = {
            warzone: { name: "Warzone", cheats: [{ id: 'aimbot', label: 'Aimbot'}, { id: 'esp', label: 'ESP' }] },
            fortnite: { name: "Fortnite", cheats: [{ id: 'aimbot', label: 'Magic Bullet'}, { id: 'esp', label: 'Loot ESP' }] }
        };
        let currentGameKey = null;
        const $ = (s) => document.querySelector(s);
        const ws = new WebSocket(`ws://${window.location.hostname}/ws`);
        function switchView(toView) { document.querySelectorAll('.view').forEach(v => v.classList.remove('active')); toView.classList.add('active'); }
        function sendMessage(msg) { ws.send(JSON.stringify(msg)); }
        function selectGame(gameKey) {
            currentGameKey = gameKey;
            $('#gameTitle').textContent = games[gameKey].name;
            buildCheatList(gameKey);
            switchView($('#cheatView'));
            sendMessage({type: 'selectGame', game: gameKey});
        }
        function buildCheatList(gameKey) {
            const list = $('#cheatList'); list.innerHTML = "";
            games[gameKey].cheats.forEach(cheat => {
                const item = document.createElement("div"); item.className = 'cheat-item';
                const label = document.createElement("span"); label.textContent = cheat.label;
                item.appendChild(label);
                const toggle = document.createElement("label"); toggle.className = 'toggle-switch';
                const input = document.createElement("input"); input.type = "checkbox";
                input.onchange = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: input.checked});
                toggle.append(input, Object.assign(document.createElement('span'), {className:'ios-slider'}));
                item.appendChild(toggle);
                list.appendChild(item);
            });
        }
        window.onload = () => {
            const gameSelection = $('#game-selection');
            Object.keys(games).forEach(key => {
                const card = document.createElement('div'); card.className = 'game-card';
                card.innerHTML = `<h3>${games[key].name}</h3>`;
                card.onclick = () => selectGame(key);
                gameSelection.appendChild(card);
            });
            $('#backToSelectionBtn').onclick = () => {
                sendMessage({type: 'selectGame', game: 'None'});
                switchView($('#selectionView'));
            };
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

    int activeCheats = (cheats.aimbot ? 1 : 0) + (cheats.esp ? 1 : 0);
    tft.drawString("Active Cheats: " + String(activeCheats), 5, 50);

    tft.drawString("Game: " + currentGame, 5, 70);
}

void handleWebSocketMessage(AsyncWebSocketClient *client, const String& message) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, message);
    String type = doc["type"];
    if (type == "setCheat") {
        String cheatId = doc["cheatId"];
        bool value = doc["value"];
        if (cheatId == "aimbot") cheats.aimbot = value;
        else if (cheatId == "esp") cheats.esp = value;
    } else if (type == "selectGame") {
        currentGame = doc["game"].as<String>();
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) Serial.printf("Client connected #%u\n", client->id());
    else if (type == WS_EVT_DISCONNECT) Serial.printf("Client disconnected #%u\n", client->id());
    else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            std::string msg; msg.assign((char*)data, len);
            handleWebSocketMessage(client, msg.c_str());
        }
    }
}

void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(1);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // Keep backlight always on

    WiFi.softAP("VENDOR.ME-CONSOLE", "cyberpunk");
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    server.begin();
}

unsigned long lastDisplayUpdate = 0;

void loop() {
    ws.cleanupClients();
    if (millis() - lastDisplayUpdate > 1000) { // Update display every second
        lastDisplayUpdate = millis();
        updateCydDisplay();
    }
}
