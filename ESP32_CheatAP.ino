#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>

// --- Simulation State & Data ---
struct Player {
  int id;
  String name;
  int health;
  bool isTarget;
};

std::vector<Player> lobby;
unsigned long lastPlayerActionTime = 0;

struct CheatState {
  bool aimbot = false;
  bool esp = false;
  bool infiniteAmmo = false;
};
CheatState cheats;

// --- Web Server and WebSocket ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>V.E.N.D.O.R Ultimate</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@700&family=Roboto:wght@400;700&display=swap');
        body, html { margin: 0; padding: 0; font-family: 'Roboto', sans-serif; background: #111; color: white; }
        .container { max-width: 600px; margin: 0 auto; padding: 10px; }
        .view { display: none; } .view.active { display: block; }
        #player-status { position: fixed; bottom: 0; left: 0; width: 100%; background: #222; padding: 10px; display: flex; justify-content: space-around; border-top: 2px solid #00BFFF; }
        .cheat-item { background: #282c34; border: 1px solid #444; border-radius: 5px; padding: 10px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; }
        .player-list-item { background: #333; padding: 8px; margin-bottom: 5px; border-left: 3px solid #555; }
        .player-list-item.esp-active { border-left: 3px solid #FFD700; }
        .player-list-item.aimbot-target { background: #8A2BE2; border-left: 3px solid #ff0000; }
        #kill-feed { position: fixed; top: 10px; right: 10px; width: 200px; text-align: right; }
        .kill-feed-entry { background: rgba(0,0,0,0.5); padding: 5px; margin-bottom: 5px; border-radius: 5px; animation: fadeOut 5s forwards; }
        @keyframes fadeOut { 0% { opacity: 1; } 80% { opacity: 1; } 100% { opacity: 0; } }
    </style>
</head>
<body>
    <div class="container">
        <h1 class="vendor-text">V.E.N.D.O.R <span id="gameTitle"></span></h1>
        <div id="cheatList"></div>
        <hr>
        <h2>Player Lobby</h2>
        <div id="playerLobby"></div>
    </div>
    <div id="player-status">
        <div>Aimbot Target: <span id="aimbotTarget">None</span></div>
    </div>
    <div id="kill-feed"></div>
    <script>
        const $ = (s) => document.querySelector(s);
        const ws = new WebSocket(`ws://${window.location.hostname}/ws`);

        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);
            if (data.type === 'lobbyUpdate') {
                const lobbyDiv = $('#playerLobby');
                lobbyDiv.innerHTML = '';
                let aimbotTargetName = 'None';
                data.lobby.forEach(p => {
                    const playerEl = document.createElement('div');
                    playerEl.className = 'player-list-item';
                    if (data.cheats.esp) playerEl.classList.add('esp-active');
                    if (p.isTarget) {
                        playerEl.classList.add('aimbot-target');
                        aimbotTargetName = p.name;
                    }
                    playerEl.innerHTML = `${p.name} - Health: ${p.health}`;
                    lobbyDiv.appendChild(playerEl);
                });
                $('#aimbotTarget').textContent = aimbotTargetName;
            } else if (data.type === 'killFeed') {
                const feedEl = document.createElement('div');
                feedEl.className = 'kill-feed-entry';
                feedEl.textContent = `${data.killer} eliminated ${data.victim}`;
                $('#kill-feed').appendChild(feedEl);
                setTimeout(() => feedEl.remove(), 5000);
            }
        };

        function buildCheatUI() {
            const cheats = [
                { id: 'aimbot', label: 'Aimbot' },
                { id: 'esp', label: 'Player ESP' }
            ];
            const list = $('#cheatList'); list.innerHTML = "";
            cheats.forEach(cheat => {
                const item = document.createElement("div"); item.className = 'cheat-item';
                const label = document.createElement("span"); label.textContent = cheat.label;
                const toggle = document.createElement("input"); toggle.type = "checkbox";
                toggle.onchange = () => ws.send(JSON.stringify({type: 'setCheat', cheatId: cheat.id, value: toggle.checked}));
                item.append(label, toggle); list.appendChild(item);
            });
        }
        window.onload = buildCheatUI;
    </script>
</body>
</html>
)rawliteral";

void createLobby() {
    lobby.clear();
    for (int i = 0; i < 10; i++) {
        Player p = {i, "Player_" + String(i), 100, false};
        lobby.push_back(p);
    }
}

void sendLobbyUpdate() {
    StaticJsonDocument<1024> doc;
    doc["type"] = "lobbyUpdate";
    doc["cheats"]["esp"] = cheats.esp;
    JsonArray lobbyData = doc.createNestedArray("lobby");
    for (const auto& p : lobby) {
        JsonObject playerObj = lobbyData.createNestedObject();
        playerObj["id"] = p.id;
        playerObj["name"] = p.name;
        playerObj["health"] = p.health;
        playerObj["isTarget"] = p.isTarget;
    }
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void sendKillFeed(String killer, String victim) {
    StaticJsonDocument<200> doc;
    doc["type"] = "killFeed";
    doc["killer"] = killer;
    doc["victim"] = victim;
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void gameTick() {
    if (millis() - lastPlayerActionTime > 2000) {
        lastPlayerActionTime = millis();
        if (lobby.size() > 1) {
            int attackerIdx = random(lobby.size());
            int victimIdx = random(lobby.size());
            if (attackerIdx != victimIdx) {
                lobby[victimIdx].health -= random(10, 25);
                if (lobby[victimIdx].health <= 0) {
                    sendKillFeed(lobby[attackerIdx].name, lobby[victimIdx].name);
                    lobby.erase(lobby.begin() + victimIdx);
                }
            }
        }
    }

    // Aimbot logic
    if (cheats.aimbot) {
        bool targetFound = false;
        for (auto& p : lobby) {
            if (!targetFound) {
                p.isTarget = true;
                targetFound = true;
            } else {
                p.isTarget = false;
            }
        }
    } else {
         for (auto& p : lobby) { p.isTarget = false; }
    }
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
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("Client connected #%u\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Client disconnected #%u\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            std::string msg; msg.assign((char*)data, len);
            handleWebSocketMessage(client, msg.c_str());
        }
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.softAP("VENDOR.ME-SIM-V4", "password");
    createLobby();
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    server.begin();
}

unsigned long lastGameTick = 0;
unsigned long lastLobbyUpdate = 0;

void loop() {
    ws.cleanupClients();
    if (millis() - lastGameTick > 1000) {
        lastGameTick = millis();
        gameTick();
    }
    if (millis() - lastLobbyUpdate > 500) {
        lastLobbyUpdate = millis();
        sendLobbyUpdate();
    }
}
