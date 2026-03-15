// Navigation logic
const navBtns = document.querySelectorAll('.nav-btn');
const tabPanes = document.querySelectorAll('.tab-pane');

navBtns.forEach(btn => {
    btn.addEventListener('click', () => {
        // Tab highlight
        navBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');

        // Show pane
        const targetTab = btn.getAttribute('data-tab');
        tabPanes.forEach(pane => {
            pane.classList.remove('active');
            if(pane.id === `tab-${targetTab}`) {
                pane.classList.add('active');
            }
        });
    });
});

// Aimbot Settings Logic
const aimbotToggle = document.getElementById('toggle-aimbot');
const aimbotSpeed = document.getElementById('aimbot-speed');
const aimbotSpeedVal = document.getElementById('aimbot-speed-val');
const aimbotFov = document.getElementById('aimbot-fov');
const aimbotFovVal = document.getElementById('aimbot-fov-val');

// Sync Display Values
aimbotSpeed.addEventListener('input', (e) => {
    aimbotSpeedVal.innerText = e.target.value;
    sendToCPP("setAimbotSpeed", parseFloat(e.target.value));
});

aimbotFov.addEventListener('input', (e) => {
    aimbotFovVal.innerHTML = e.target.value + '&deg;';
    sendToCPP("setAimbotFov", parseFloat(e.target.value));
});

// Toggle Aimbot
aimbotToggle.addEventListener('change', (e) => {
    const isEnabled = e.target.checked;
    sendToCPP("toggleAimbot", isEnabled);
});

// --- Bridge to C++ ---
// In a real WebView2/CEF implementation, you use window.chrome.webview.postMessage 
// or a custom JS binding created by your C++ code.

function sendToCPP(action, value) {
    const payload = JSON.stringify({ action: action, value: value });
    console.log("[JS -> C++] " + payload);
    
    // WebView2 standard bridge
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage(payload);
    } 
    // CEF / Ultralight custom JS binding (if registered as window.CheatAPI)
    else if (window.CheatAPI) {
        window.CheatAPI.onMessage(payload);
    }
}

// Draggable window logic (simplified for browser preview, C++ WndProc usually handles this natively)
const menu = document.getElementById('cheat-menu');
const header = menu.querySelector('.menu-header');
let isDragging = false;
let offsetX, offsetY;

header.addEventListener('mousedown', (e) => {
    isDragging = true;
    offsetX = e.clientX - menu.offsetLeft;
    offsetY = e.clientY - menu.offsetTop;
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    menu.style.left = `${e.clientX - offsetX}px`;
    menu.style.top = `${e.clientY - offsetY}px`;
});

document.addEventListener('mouseup', () => {
    isDragging = false;
});

// Center menu on load
window.onload = () => {
    menu.style.left = `${(window.innerWidth - menu.offsetWidth) / 2}px`;
    menu.style.top = `${(window.innerHeight - menu.offsetHeight) / 2}px`;
};
