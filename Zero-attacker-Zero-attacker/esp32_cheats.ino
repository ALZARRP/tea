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

struct Player { int id; String name; int health = 100; int armor = 100; bool isTarget = false; bool isVisible = false; int x = 50; int y = 50; };
std::vector<Player> lobby;
unsigned long lastActionTime = 0;
String currentGame = "None";
int activeCheatsCount = 0;

struct CheatState {
    bool aimbot_active = false; float aimbot_fov = 10.0f; float aimbot_smooth = 5.0f;
    bool esp_active = false; bool esp_box = true; bool esp_line = false;
    bool no_recoil = false; float recoil_percent = 0.0f;
    bool wz_uav = false; bool fn_autobuild = false; bool r6_unlock = false;
    bool bo6_omni_aim = false; bool ap_glow = false;
    // Add all 100+ cheat variables here...
};
CheatState cheats;

std::map<uint32_t, unsigned long> lastMessageTime;
const char* ap_ssid = "VENDOR.ME-ULTIMATE";
const char* ap_password = "cyberpunk123";

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

        /* --- CSS Variables --- */
        :root {
            --neon-glow: #00ffff;
            --bg-color: #00001a;
            --text-color: #00ffff;
            --container-bg: rgba(10, 10, 30, 0.8);
            --border-color: rgba(0, 255, 255, 0.5);
            --font-main: 'Roboto', sans-serif;
            --font-title: 'Orbitron', sans-serif;
        }

        /* --- General Styles --- */
        body, html {
            margin: 0;
            padding: 0;
            font-family: var(--font-main);
            background: var(--bg-color);
            color: var(--text-color);
            overflow: hidden;
        }

        body::before {
            content: "";
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: linear-gradient(
                rgba(0, 20, 40, 0.8),
                rgba(0, 10, 20, 0.9)
            ), url('https://i.imgur.com/Pijv2cM.gif');
            background-blend-mode: overlay;
            z-index: -1;
            animation: background-pan 30s linear infinite;
        }

        @keyframes background-pan {
            0% { background-position: 0% 0%; }
            100% { background-position: 100% 100%; }
        }

        .container {
            max-width: 1000px;
            margin: 0 auto;
            padding: 15px;
            backdrop-filter: blur(10px);
        }

        h1, h2, h3 {
            font-family: var(--font-title);
            text-align: center;
            color: var(--neon-glow);
            text-shadow: 0 0 5px #fff, 0 0 10px var(--neon-glow), 0 0 15px var(--neon-glow);
        }

        /* --- View Management --- */
        .view {
            display: none;
        }
        .view.active {
            display: flex;
            flex-direction: column;
            height: 95vh;
        }

        /* --- Game Selection --- */
        #game-selection {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 15px;
        }

        .game-card {
            background: var(--container-bg);
            border: 1px solid var(--border-color);
            border-radius: 10px;
            padding: 20px;
            width: 45%;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s ease;
        }

        .game-card:hover {
            border-color: var(--neon-glow);
            box-shadow: 0 0 15px var(--neon-glow);
            transform: translateY(-5px);
        }

        /* --- Main Cheat Layout --- */
        .main-layout {
            display: flex;
            gap: 20px;
            flex-grow: 1;
            overflow-y: hidden;
        }

        .cheat-container {
            width: 70%;
            overflow-y: auto;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            padding-right: 10px;
        }

        .sidebar {
            width: 30%;
        }

        /* --- Cheat Styles --- */
        .cheat-category {
            background: var(--container-bg);
            padding: 15px;
            border-radius: 8px;
            border: 1px solid var(--border-color);
            margin-bottom: 20px;
        }

        .cheat-item {
            margin-bottom: 15px;
            position: relative;
            padding: 5px;
            border-radius: 5px;
            transition: background-color 0.3s;
        }

        .cheat-item.active {
            background-color: rgba(0, 255, 255, 0.1);
        }

        .cheat-item label {
            display: block;
            margin-bottom: 5px;
        }

        .cheat-item .tooltip {
            visibility: hidden;
            width: 200px;
            background-color: #555;
            color: #fff;
            text-align: center;
            border-radius: 6px;
            padding: 5px 0;
            position: absolute;
            z-index: 1;
            bottom: 125%;
            left: 50%;
            margin-left: -100px;
            opacity: 0;
            transition: opacity 0.3s;
        }

        .cheat-item:hover .tooltip {
            visibility: visible;
            opacity: 1;
        }

        /* --- Form Elements --- */
        button {
            width: 100%;
            padding: 14px;
            margin-top: 10px;
            font-size: 1.1rem;
            font-family: var(--font-main);
            color: #000;
            background: var(--neon-glow);
            border: 2px solid var(--neon-glow);
            border-radius: 15px;
            cursor: pointer;
            box-shadow: 0 0 10px var(--neon-glow);
            transition: all 0.3s ease;
        }

        button:hover {
            background: #000;
            color: var(--neon-glow);
        }

        input, select {
            width: 100%;
            padding: 12px;
            margin: 8px 0;
            background: rgba(0,0,0,0.5);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            color: var(--text-color);
        }

        /* --- Alert & Notifications --- */
        #alert-container {
            position: fixed;
            top: 10px;
            left: 50%;
            transform: translateX(-50%);
            z-index: 1000;
        }

        .alert {
            padding: 10px 20px;
            background: var(--neon-glow);
            color: #000;
            border-radius: 5px;
            margin-bottom: 5px;
            animation: fadeOut 3s forwards;
        }

        @keyframes fadeOut {
            0% { opacity: 1; }
            80% { opacity: 1; }
            100% { opacity: 0; }
        }

        /* --- Sidebar Components --- */
        #radar {
            width: 200px;
            height: 200px;
            background: #000;
            border: 2px solid var(--neon-glow);
            border-radius: 50%;
            margin: 20px auto;
            position: relative;
        }

        .blip {
            position: absolute;
            width: 5px;
            height: 5px;
            background: red;
            border-radius: 50%;
        }

        /* --- Toggle Switch --- */
        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .toggle-switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .ios-slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }

        .ios-slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked + .ios-slider {
            background-color: var(--neon-glow);
        }

        input:checked + .ios-slider:before {
            transform: translateX(26px);
        }

        /* --- Range Slider --- */
        input[type=range] {
            width: 100%;
            -webkit-appearance: none;
            background: #333;
            height: 5px;
            border-radius: 5px;
        }

        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            background: var(--neon-glow);
            cursor: pointer;
            border-radius: 50%;
        }

        /* --- Console Log --- */
        #console-container {
            margin-top: 20px;
            background: var(--container-bg);
            padding: 15px;
            border-radius: 8px;
            border: 1px solid var(--border-color);
        }

        #console-log {
            height: 150px;
            overflow-y: auto;
            background: #000;
            padding: 10px;
            font-family: 'Courier New', Courier, monospace;
            font-size: 0.9em;
            border-radius: 5px;
        }

        .console-line {
            margin-bottom: 5px;
            white-space: pre-wrap;
            word-break: break-all;
        }

        .console-line.error {
            color: #ff4444;
        }

        .console-line.success {
            color: #00ff88;
        }

        .console-line.info {
            color: #88aaff;
        }
    </style>
</head>
<body>
    <div id="alert-container"></div>
    <div class="container">
        <section id="loginView" class="view active"><h1>V.E.N.D.O.R</h1><input id="user" placeholder="Username"><input id="pass" type="password" placeholder="Password"><button id="loginBtn">Connect</button></section>
        <section id="selectionView" class="view"><h1>SELECT DEPLOYMENT</h1><div id="game-selection"></div></section>
        <section id="cheatView" class="view">
            <h2 id="gameTitle"></h2>
            <div class="main-layout">
                <div id="cheat-container"></div>
                <div class="sidebar">
                    <div id="radar"></div>
                    <h3>Lobby</h3>
                    <div id="player-lobby"></div>
                    <div id="console-container">
                        <h3>CONSOLE LOG</h3>
                        <div id="console-log"></div>
                    </div>
                </div>
            </div>
            <button id="backToSelectionBtn">Disconnect</button>
        </section>
    </div>
    <script>
        // --- Game and Cheat Data ---
        // This object contains all the games and their available cheats.
        // Each cheat has a unique ID, a label for the UI, a type (toggle, slider, etc.),
        // a category, and a description for the tooltip.
        const games = {
            warzone: {
                name: "Warzone",
                cheats: [
                    { id: 'aimbot_active', label: 'Enable Aimbot', type: 'toggle', category: 'Combat', description: 'Automatically aim at enemies.' },
                    { id: 'aimbot_key', label: 'Aimbot Key', type: 'dropdown', category: 'Combat', params: { options: ['ALT', 'SHIFT', 'MOUSE1', 'MOUSE2'] }, description: 'The key to activate the aimbot.' },
                    { id: 'aimbot_fov', label: 'Aimbot FOV', type: 'slider', category: 'Combat', params: { min: 1, max: 50, default: 10 }, description: 'The field of view for the aimbot.' },
                    { id: 'aimbot_smooth', label: 'Aimbot Smooth', type: 'slider', category: 'Combat', params: { min: 1, max: 20, default: 5 }, description: 'The smoothing for the aimbot.' },
                    { id: 'esp_active', label: 'Enable Player ESP', type: 'toggle', category: 'Visuals', description: 'See players through walls.' },
                    { id: 'esp_box', label: 'ESP Box Type', type: 'dropdown', category: 'Visuals', params: { options: ['2D', '3D', 'Corner'] }, description: 'The type of box to draw around players.' },
                    { id: 'esp_line', label: 'ESP Snapline', type: 'toggle', category: 'Visuals', description: 'Draw lines to players.' },
                    { id: 'esp_health', label: 'Health Bar', type: 'toggle', category: 'Visuals', description: 'Show player health bars.' },
                    { id: 'esp_distance', label: 'Show Distance', type: 'toggle', category: 'Visuals', description: 'Show distance to players.' },
                    { id: 'esp_color', label: 'Visible Color', type: 'color', category: 'Visuals', params: { default: '#00ff00' }, description: 'The color of visible players.' },
                    { id: 'no_recoil', label: 'No Recoil', type: 'toggle', category: 'Combat', description: 'Remove weapon recoil.' },
                    { id: 'recoil_percent', label: 'Recoil Control %', type: 'slider', category: 'Combat', params: { min: 0, max: 100, default: 75 }, description: 'The percentage of recoil to control.' },
                    { id: 'wz_uav', label: 'Constant UAV', type: 'toggle', category: 'Utility', description: 'Always have a UAV active.' },
                    { id: 'wz_radar', label: 'Mini-map Radar', type: 'toggle', category: 'Utility', description: 'See enemies on the mini-map.' },
                    { id: 'unlock_all', label: 'Unlock All Items', type: 'button', category: 'Utility', params: { text: 'Unlock Now' }, description: 'Unlock all in-game items.' }
                ]
            },
            r6: {
                name: "Rainbow Six",
                cheats: [
                    { id: 'r6_aimbot', label: 'Enable Aimbot', type: 'toggle', category: 'Combat', description: 'Automatically aim at enemies.' },
                    { id: 'r6_aim_key', label: 'Aimbot Key', type: 'dropdown', category: 'Combat', params: { options: ['ALT', 'SHIFT', 'MOUSE1', 'MOUSE2'] }, description: 'The key to activate the aimbot.' },
                    { id: 'r6_esp', label: 'Player ESP', type: 'toggle', category: 'Visuals', description: 'See players through walls.' },
                    { id: 'r6_glow', label: 'Player Glow', type: 'toggle', category: 'Visuals', description: 'Make players glow.' },
                    { id: 'r6_no_recoil', label: 'No Recoil', type: 'toggle', category: 'Combat', description: 'Remove weapon recoil.' },
                    { id: 'r6_no_spread', label: 'No Spread', type: 'toggle', category: 'Combat', description: 'Remove weapon spread.' },
                    { id: 'r6_unlock', label: 'Unlock All Operators', type: 'button', category: 'Utility', params: { text: 'Unlock' }, description: 'Unlock all operators.' },
                    { id: 'r6_fov', label: 'FOV Changer', type: 'slider', category: 'Visuals', params: { min: 90, max: 120, default: 90 }, description: 'Change your field of view.' },
                ]
            },
            apex: {
                name: "Apex Legends",
                cheats: [
                    { id: 'ap_aimbot', label: 'Enable Aimbot', type: 'toggle', category: 'Combat', description: 'Automatically aim at enemies.' },
                    { id: 'ap_aim_key', label: 'Aimbot Key', type: 'dropdown', category: 'Combat', params: { options: ['ALT', 'SHIFT', 'MOUSE1', 'MOUSE2'] }, description: 'The key to activate the aimbot.' },
                    { id: 'ap_esp', label: 'Player ESP', type: 'toggle', category: 'Visuals', description: 'See players through walls.' },
                    { id: 'ap_glow', label: 'Item Glow', type: 'toggle', category: 'Visuals', description: 'Make items glow.' },
                    { id: 'ap_no_recoil', label: 'No Recoil', type: 'toggle', category: 'Combat', description: 'Remove weapon recoil.' },
                    { id: 'ap_bhop', label: 'Auto Bunnyhop', type: 'toggle', category: 'Movement', description: 'Automatically bunnyhop.' },
                    { id: 'ap_skin', label: 'Skin Changer', type: 'button', category: 'Utility', params: { text: 'Open Skin Selector' }, description: 'Change your weapon skins.' },
                ]
            },
            bo6: {
                name: "Black Ops 6",
                cheats: [
                    { id: 'bo6_aimbot', label: 'Enable Aimbot', type: 'toggle', category: 'Combat', description: 'Automatically aim at enemies.' },
                    { id: 'bo6_omni_aim', label: 'Omni-directional Aim', type: 'toggle', category: 'Combat', description: 'Aim in any direction.' },
                    { id: 'bo6_esp', label: 'Player ESP', type: 'toggle', category: 'Visuals', description: 'See players through walls.' },
                    { id: 'bo6_zombie_esp', label: 'Zombie ESP', type: 'toggle', category: 'Visuals', description: 'See zombies through walls.' },
                    { id: 'bo6_rapid_fire', label: 'Rapid Fire', type: 'toggle', category: 'Combat', description: 'Increase weapon fire rate.' },
                    { id: 'bo6_inf_ammo', label: 'Infinite Ammo', type: 'toggle', category: 'Combat', description: 'Never run out of ammo.' },
                    { id: 'bo6_prestige', label: 'Set Prestige Level', type: 'slider', category: 'Utility', params: { min: 1, max: 100, default: 1 }, description: 'Set your prestige level.' },
                ]
            },
            fortnite: {
                name: "Fortnite",
                cheats: [
                    { id: 'fn_aimbot', label: 'Enable Aimbot', type: 'toggle', category: 'Combat', description: 'Automatically aim at enemies.' },
                    { id: 'fn_autobuild', label: 'Auto-Build', type: 'toggle', category: 'Building', description: 'Automatically build structures.' },
                    { id: 'fn_edit_assist', label: 'Edit Assist', type: 'toggle', category: 'Building', description: 'Assist with editing structures.' },
                    { id: 'fn_esp', label: 'Player ESP', type: 'toggle', category: 'Visuals', description: 'See players through walls.' },
                    { id: 'fn_chest_esp', label: 'Chest ESP', type: 'toggle', category: 'Visuals', description: 'See chests through walls.' },
                    { id: 'fn_vbucks', label: 'Generate V-Bucks', type: 'button', category: 'Utility', params: { text: 'Generate' }, description: 'Generate V-Bucks.' },
                ]
            },
            valorant: {
                name: "Valorant",
                cheats: [
                    { id: 'val_aimbot', label: 'Enable Aimbot', type: 'toggle', category: 'Combat', description: 'Automatically aim at enemies.' },
                    { id: 'val_triggerbot', label: 'Triggerbot', type: 'toggle', category: 'Combat', description: 'Automatically shoot when an enemy is in your crosshair.' },
                    { id: 'val_esp', label: 'Player ESP', type: 'toggle', 'category': 'Visuals', description: 'See players through walls.' },
                    { id: 'val_spike_timer', label: 'Spike Timer', type: 'toggle', category: 'Utility', description: 'Show a timer for the spike.' },
                    { id: 'val_skin_changer', label: 'Skin Changer', type: 'button', category: 'Utility', params: { text: 'Select Skins' }, description: 'Change your weapon skins.' },
                ]
            }
        };

        // --- Utility Functions ---
        const $ = (s) => document.querySelector(s);

        /**
         * Logs a message to the on-screen console.
         * @param {string} message The message to log.
         * @param {string} type The type of message (info, success, error).
         */
        function logToConsole(message, type = 'info') {
            const log = $('#console-log');
            const line = document.createElement('div');
            line.className = `console-line ${type}`;
            line.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
            log.appendChild(line);
            log.scrollTop = log.scrollHeight;
        }

        /**
         * Switches the current view.
         * @param {HTMLElement} toView The view element to switch to.
         */
        function switchView(toView) {
            document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
            toView.classList.add('active');
            logToConsole(`Switched to view: ${toView.id}`);
        }

        /**
         * Sends a message to the backend (currently stubbed).
         * @param {object} msg The message object to send.
         */
        function sendMessage(msg) {
            logToConsole(`Sending message: ${JSON.stringify(msg)}`, 'info');
            // In a real application, you would send this message over a WebSocket.
            // ws.send(JSON.stringify(msg));
        }

        /**
         * Shows a temporary alert at the top of the screen.
         * @param {string} message The message to display.
         */
        function showAlert(message) {
            const a = document.createElement('div');
            a.className = 'alert';
            a.textContent = message;
            $('#alert-container').appendChild(a);
            setTimeout(() => a.remove(), 3000);
            logToConsole(message, 'success');
        }

        /**
         * Populates the cheat menu for a selected game.
         * @param {string} gameKey The key of the selected game in the `games` object.
         */
        function selectGame(gameKey) {
            $('#gameTitle').textContent = games[gameKey].name;
            const container = $('#cheat-container');
            container.innerHTML = "";
            const categories = {};
            games[gameKey].cheats.forEach(cheat => {
                if (!categories[cheat.category]) {
                    categories[cheat.category] = [];
                }
                categories[cheat.category].push(cheat);
            });

            for (const category in categories) {
                const catDiv = document.createElement('div');
                catDiv.className = 'cheat-category';
                const title = document.createElement('h3');
                title.textContent = category;
                catDiv.appendChild(title);

                categories[category].forEach(cheat => {
                    const item = document.createElement("div");
                    item.className = 'cheat-item';

                    const label = document.createElement("label");
                    label.textContent = cheat.label;
                    item.appendChild(label);

                    const tooltip = document.createElement('span');
                    tooltip.className = 'tooltip';
                    tooltip.textContent = cheat.description;
                    item.appendChild(tooltip);

                    if (cheat.type === 'toggle') {
                        const toggle = document.createElement("label");
                        toggle.className = 'toggle-switch';
                        const input = document.createElement("input");
                        input.type = "checkbox";
                        input.onchange = () => {
                            sendMessage({type: 'setCheat', cheatId: cheat.id, value: input.checked});
                            showAlert(`${cheat.label} ${input.checked ? 'ON' : 'OFF'}`);
                            item.classList.toggle('active', input.checked);
                        };
                        toggle.append(input, Object.assign(document.createElement('span'), {className:'ios-slider'}));
                        item.appendChild(toggle);
                    } else if (cheat.type === 'slider') {
                        const slider = document.createElement("input");
                        slider.type = "range";
                        slider.min = cheat.params.min;
                        slider.max = cheat.params.max;
                        slider.value = cheat.params.default;
                        slider.oninput = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: parseFloat(slider.value)});
                        item.appendChild(slider);
                    } else if (cheat.type === 'dropdown') {
                        const select = document.createElement("select");
                        cheat.params.options.forEach(opt => {
                            const option = document.createElement("option");
                            option.value = opt;
                            option.textContent = opt;
                            select.appendChild(option);
                        });
                        select.onchange = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: select.value});
                        item.appendChild(select);
                    } else if (cheat.type === 'color') {
                        const colorInput = document.createElement("input");
                        colorInput.type = "color";
                        colorInput.value = cheat.params.default;
                        colorInput.onchange = () => sendMessage({type: 'setCheat', cheatId: cheat.id, value: colorInput.value});
                        item.appendChild(colorInput);
                    } else if (cheat.type === 'button') {
                        const button = document.createElement("button");
                        button.textContent = cheat.params.text;
                        button.onclick = () => {
                            sendMessage({type: 'executeCheat', cheatId: cheat.id });
                            showAlert(`Executed: ${cheat.label}`);
                        };
                        item.appendChild(button);
                    }

                    catDiv.appendChild(item);
                });
                container.appendChild(catDiv);
            }
            switchView($('#cheatView'));
            sendMessage({type: 'selectGame', game: gameKey});
        }

        // --- Initialization ---
        window.onload = () => {
            logToConsole("System Initialized. Standby for user input.");

            $('#loginBtn').onclick = () => {
                // This is a fake authentication check.
                logToConsole("Authenticating...");
                setTimeout(() => {
                    showAlert("Connection Established. Welcome!");
                    switchView($('#selectionView'));
                }, 1000);
            };

            const gameSelection = $('#game-selection');
            Object.keys(games).forEach(key => {
                const card = document.createElement('div');
                card.className = 'game-card';
                card.innerHTML = `<h3>${games[key].name}</h3>`;
                card.onclick = () => selectGame(key);
                gameSelection.appendChild(card);
            });

            $('#backToSelectionBtn').onclick = () => {
                showAlert("Disconnected.");
                switchView($('#selectionView'));
            };
        };
    </script>
</body>
</html>
)rawliteral";

void createLobby(int numPlayers) { lobby.clear(); for (int i = 0; i < numPlayers; i++) lobby.push_back({i, "Player_" + String(i), 100, 100, false, false, random(100), random(100)}); }
void gameTick() {
    if (millis() - lastActionTime > 2000) {
        lastActionTime = millis();
        if (lobby.size() > 1) { int victimIdx = random(lobby.size()); lobby[victimIdx].health -= random(10, 20); if (lobby[victimIdx].health <= 0) lobby.erase(lobby.begin() + victimIdx); }
    }
    bool targetSet = false;
    for (auto& p : lobby) {
        p.isVisible = cheats.esp_active || cheats.wz_uav;
        if (cheats.aimbot_active && !targetSet) { p.isTarget = true; targetSet = true; } else { p.isTarget = false; }
    }
}
void sendGameState() {
    StaticJsonDocument<3072> doc; doc["type"] = "gameState";
    JsonArray lobbyData = doc.createNestedArray("lobby");
    for (const auto& p : lobby) {
        JsonObject playerObj = lobbyData.createNestedObject();
        playerObj["id"] = p.id; playerObj["name"] = p.name; playerObj["health"] = p.health;
        playerObj["armor"] = p.armor; playerObj["isTarget"] = p.isTarget; playerObj["isVisible"] = p.isVisible;
        playerObj["x"] = p.x; playerObj["y"] = p.y;
    }
    String output; serializeJson(doc, output); ws.textAll(output);
}
void handleWebSocketMessage(AsyncWebSocketClient *client, char *data) {
    if (lastMessageTime.count(client->id()) && (millis() - lastMessageTime[client->id()] < 100)) return;
    lastMessageTime[client->id()] = millis();
    StaticJsonDocument<256> doc; deserializeJson(doc, data);
    const char* type = doc["type"];
    if (strcmp(type, "setCheat") == 0) {
        const char* cheatId = doc["cheatId"];
        if (strcmp(cheatId, "aimbot_active") == 0) cheats.aimbot_active = doc["value"];
        else if (strcmp(cheatId, "esp_active") == 0) cheats.esp_active = doc["value"];
        else if (strcmp(cheatId, "no_recoil") == 0) cheats.no_recoil = doc["value"];
        else if (strcmp(cheatId, "wz_uav") == 0) cheats.wz_uav = doc["value"];
        // ... handlers for all 100+ cheats
    } else if (strcmp(type, "selectGame") == 0) {
        currentGame = doc["game"].as<String>();
    }
}
void onEvent(AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) { lastMessageTime[c->id()] = 0; }
    else if (type == WS_EVT_DISCONNECT) { lastMessageTime.erase(c->id()); }
    else if (type == WS_EVT_DATA) { AwsFrameInfo *i=(AwsFrameInfo*)arg; if(i->final&&i->index==0&&i->len==len&&i->opcode==WS_TEXT){data[len]=0;handleWebSocketMessage(c,(char*)data);}}
}
void updateCydDisplay() {
    tft.fillScreen(TFT_BLACK); tft.setTextDatum(TL_DATUM); tft.setTextColor(TFT_CYAN, TFT_BLACK);
    String ip_addr = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : "Connecting...";
    tft.drawString("IP: " + ip_addr, 5, 10);
    tft.drawString("Clients: " + String(ws.count()), 5, 30);
    activeCheatsCount = (cheats.aimbot_active ? 1 : 0) + (cheats.esp_active ? 1 : 0) + (cheats.no_recoil ? 1 : 0);
    tft.drawString("Active Cheats: " + String(activeCheatsCount), 5, 50);
    tft.drawString("Game: " + currentGame, 5, 70);
    tft.drawString("VENDOR.ME Ultimate", 5, 100);
}
void setup() {
  Serial.begin(115200); tft.init(); tft.setRotation(1); pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  createLobby(15); WiFi.softAP(ap_ssid, ap_password);
  ws.onEvent(onEvent); server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send_P(200, "text/html", index_html); });
  ArduinoOTA.begin(); server.begin();
}
unsigned long lastDisplayUpdateTime = 0, lastGameTickTime = 0, lastSocketUpdateTime = 0;
void loop() {
  ws.cleanupClients(); ArduinoOTA.handle();
  if (millis() - lastGameTickTime > 1000) { lastGameTickTime = millis(); gameTick(); }
  if (millis() - lastSocketUpdateTime > 500) { lastSocketUpdateTime = millis(); sendGameState(); }
  if (millis() - lastDisplayUpdateTime > 1000) { lastDisplayUpdateTime = millis(); updateCydDisplay(); }
}
