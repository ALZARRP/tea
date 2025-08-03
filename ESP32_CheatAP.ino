#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>

// --- Simulation State & Data ---
struct Player {
    int id; String name; int health; int armor; bool isTarget; bool isVisible;
};
std::vector<Player> lobby;
unsigned long lastPlayerActionTime = 0;

struct CheatState {
    // Shared states
    bool aimbot_active = false; float aimbot_fov = 10.0f; float aimbot_smooth = 5.0f;
    bool esp_active = false; bool esp_box = true; bool esp_line = false;
    bool no_recoil = false; float recoil_percent = 0.0f;
    // Game specific
    bool wz_wallhack = false;
    bool fn_autobuild = false;
    bool r6_unlock = false;
    bool bo6_omni_aim = false;
};
CheatState cheats;

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
        body, html { margin: 0; padding: 0; font-family: 'Roboto', sans-serif; background: #0a0a0a; color: white; }
        h1, h2, h3 { font-family: 'Orbitron', sans-serif; text-align: center; color: var(--primary-glow, #00BFFF); text-shadow: 0 0 10px var(--primary-glow, #00BFFF); }
        .container { max-width: 800px; margin: 0 auto; padding: 10px; }
        .view { display: none; } .view.active { display: block; }
        #game-selection { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; }
        .game-card { background: #1a1a1a; border: 2px solid #333; border-radius: 10px; padding: 20px; width: 45%; text-align: center; cursor: pointer; transition: all 0.3s ease; }
        .game-card:hover { border-color: var(--primary-glow, #00BFFF); box-shadow: 0 0 15px var(--primary-glow, #00BFFF); }
        .cheat-container { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        .cheat-category { background: #1c1c1c; padding: 15px; border-radius: 8px; border: 1px solid #444; }
        .cheat-item { margin-bottom: 15px; }
        .cheat-item label { display: block; margin-bottom: 5px; }
        .toggle-switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .ios-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .ios-slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .ios-slider { background-color: var(--primary-glow, #00BFFF); }
        input:checked + .ios-slider:before { transform: translateX(26px); }
        input[type=range] { width: 100%; }
        .back-button { margin-top: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <section id="selectionView" class="view active"><h1>Select Deployment</h1><div id="game-selection"></div></section>
        <section id="cheatView" class="view"><h2 id="gameTitle"></h2><div id="cheat-container"></div><button id="backToSelectionBtn" class="back-button">Disconnect</button></section>
    </div>
    <script>
        const games = {
            warzone: {
                name: "Warzone", theme: "--primary-glow: #FFD700;",
                cheats: [
                    { id: 'aimbot_active', label: 'Enable Aimbot', type: 'toggle', category: 'Combat' },
                    { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 10 } },
                    { id: 'aimbot_smooth', label: 'Aimbot Smooth', type: 'slider', category: 'Combat', params: { min: 1, max: 20, default: 5 } },
                    { id: 'esp_active', label: 'Enable ESP', type: 'toggle', category: 'Visuals' },
                    { id: 'esp_box', label: 'ESP Box', type: 'toggle', category: 'Visuals' },
                    { id: 'esp_line', label: 'ESP Line', type: 'toggle', category: 'Visuals' },
                    { id: 'no_recoil', label: 'No Recoil', type: 'toggle', category: 'Combat' },
                    { id: 'recoil_percent', label: 'Recoil %', type: 'slider', category: 'Combat', params: { min: 0, max: 100, default: 0 } },
                    { id: 'wz_wallhack', label: 'Solid Wallhack', type: 'toggle', category: 'Visuals' },
                    { id: 'wz_uav', label: 'Constant UAV', type: 'toggle', category: 'Utility' },
                    { id: 'wz_speed', label: 'Speed Hack', type: 'toggle', category: 'Movement' },
                    { id: 'wz_slidecancel', label: 'Auto Slide-Cancel', type: 'toggle', category: 'Movement' },
                    { id: 'wz_heartbeat', label: 'Unlimited Heartbeat', type: 'toggle', category: 'Utility' },
                    { id: 'wz_armor', label: 'Auto-Plate', type: 'toggle', category: 'Utility' },
                    { id: 'wz_cash', label: 'Cash Multiplier', type: 'toggle', category: 'Utility' },
                    { id: 'wz_radar', label: 'Mini-Map Radar', type: 'toggle', category: 'Visuals' },
                    { id: 'wz_silent', label: 'Silent Aim', type: 'toggle', category: 'Combat' },
                    { id: 'wz_spinbot', label: 'Spin Bot', type: 'toggle', category: 'Combat' },
                    { id: 'wz_longslide', label: 'Long Slide', type: 'toggle', category: 'Movement' },
                    { id: 'wz_thirdperson', label: 'Third Person', type: 'toggle', category: 'Visuals' }
                ]
            },
            fortnite: {
                name: "Fortnite", theme: "--primary-glow: #8A2BE2;",
                cheats: [
                    { id: 'aimbot_active', label: 'Enable Aimbot', type: 'toggle', category: 'Combat' },
                    { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 15 } },
                    { id: 'fn_build', label: 'Auto 1x1 Build', type: 'toggle', category: 'Utility' },
                    { id: 'fn_edit', label: 'Instant Edit', type: 'toggle', category: 'Utility' },
                    { id: 'esp_active', label: 'Enable ESP', type: 'toggle', category: 'Visuals' },
                    { id: 'esp_box', label: 'Player Box ESP', type: 'toggle', category: 'Visuals' },
                    { id: 'fn_chams', label: 'Player Chams', type: 'toggle', category: 'Visuals' },
                    { id: 'fn_fly', label: 'Fly Hack', type: 'toggle', category: 'Movement' },
                    { id: 'fn_vbuck', label: 'V-Buck Generator', type: 'toggle', category: 'Utility' },
                    { id: 'no_recoil', label: 'No Bloom', type: 'toggle', category: 'Combat' },
                    { id: 'fn_doublepump', label: 'Double Pump', type: 'toggle', category: 'Combat' },
                    { id: 'fn_invis', label: 'Invisibility', type: 'toggle', category: 'Utility' },
                    { id: 'fn_item_esp', label: 'Item ESP', type: 'toggle', category: 'Visuals' },
                    { id: 'fn_weakpoint', label: 'Auto Weak-Point', type: 'toggle', category: 'Combat' },
                    { id: 'fn_revive', label: 'Instant Revive', type: 'toggle', category: 'Utility' },
                    { id: 'fn_speed', label: 'Player Speed', type: 'slider', category: 'Movement', params: { min: 1, max: 5, default: 1 } },
                    { id: 'fn_bhop', label: 'Bunny Hop', type: 'toggle', category: 'Movement' },
                    { id: 'fn_noreload', label: 'No Reload', type: 'toggle', category: 'Combat' },
                    { id: 'fn_carfly', label: 'Car Fly', type: 'toggle', category: 'Movement' },
                    { id: 'fn_emote', label: 'Emote Anywhere', type: 'toggle', category: 'Utility' }
                ]
            }
        };
        const $ = (s) => document.querySelector(s);
        function switchView(toView) { document.querySelectorAll('.view').forEach(v => v.classList.remove('active')); toView.classList.add('active'); }
        function sendMessage(msg) { console.log(JSON.stringify(msg)); } // Placeholder
        function selectGame(gameKey) {
            document.documentElement.style.cssText = games[gameKey].theme;
            $('#gameTitle').textContent = games[gameKey].name;
            buildCheatList(gameKey);
            switchView($('#cheatView'));
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
                        slider.oninput = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: slider.value});
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
            $('#backToSelectionBtn').onclick = () => switchView($('#selectionView'));
            switchView($('#selectionView'));
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    WiFi.softAP("VENDOR.ME-ULTIMATE", "password");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    server.begin();
}

void loop() {}
