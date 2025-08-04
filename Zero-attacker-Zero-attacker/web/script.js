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
            { id: 'val_esp', label: 'Player ESP', type: 'toggle', category: 'Visuals', description: 'See players through walls.' },
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

// --- WebSocket Handler ---
// This is where you would handle messages from the backend.
// ws.onmessage = (event) => {
//     const data = JSON.parse(event.data);
//     logToConsole(`Received message: ${JSON.stringify(data)}`, 'info');
//     if (data.type === 'gameState') {
//         const lobbyDiv = $('#player-lobby');
//         lobbyDiv.innerHTML = '';
//         const radar = $('#radar');
//         radar.innerHTML = '';
//         data.lobby.forEach(p => {
//             const playerEl = document.createElement('div');
//             playerEl.innerHTML = `${p.name} - HP: ${p.health}`;
//             lobbyDiv.appendChild(playerEl);
//             if(p.isVisible) {
//                 const blip = document.createElement('div');
//                 blip.className = 'blip';
//                 blip.style.left = `${p.x}%`;
//                 blip.style.top = `${p.y}%`;
//                 if (p.isTarget) blip.style.background = 'yellow';
//                 radar.appendChild(blip);
//             }
//         });
//     }
// };

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
