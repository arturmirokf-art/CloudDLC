#include "Gui.h"
#include "JNIHelper.h"
#include <gl/GL.h>
#include <iostream>
#include <vector>

bool Gui::s_visible = false;
int Gui::s_activeTab = 0;
static uint32_t fontBase = 0;
static bool fontInitialized = false;

struct MouseState {
    float x, y;
    bool leftDown;
    bool wasLeftDown;
} mouse;

void Gui::Toggle() {
    s_visible = !s_visible;
}

bool Gui::IsVisible() {
    return s_visible;
}

void Gui::Initialize(HDC hdc) {
    if (fontInitialized) return;
    
    // Safety check for HDC
    if (!hdc) return;

    fontBase = glGenLists(96);
    HFONT font = CreateFontA(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Segoe UI");
    if (!font) return; // Should not happen but for safety

    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    wglUseFontBitmapsA(hdc, 32, 96, fontBase);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
    fontInitialized = true;
}

void Gui::DrawText(float x, float y, const std::string& text, float r, float g, float b) {
    if (!fontInitialized) return;
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(fontBase - 32);
    glCallLists((GLsizei)text.length(), GL_UNSIGNED_BYTE, text.c_str());
    glPopAttrib();
}

void Gui::DrawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

bool IsInArea(float x, float y, float w, float h) {
    return (mouse.x >= x && mouse.x <= x + w && mouse.y >= y && mouse.y <= y + h);
}

void Gui::Render(HDC hdc) {
    if (!s_visible || !hdc) return;
    Initialize(hdc);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    HWND hwnd = WindowFromDC(hdc);
    if (!hwnd) return; // Fail safe if no window associated with DC

    RECT rect;
    if (!GetClientRect(hwnd, &rect)) return;
    float screenW = (float)(rect.right - rect.left);
    float screenH = (float)(rect.bottom - rect.top);

    // Update mouse coords for the window
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    mouse.x = (float)p.x;
    mouse.y = (float)p.y;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenW, screenH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // --- Modern GL States Reset ---
    // Minecraft uses shaders, we need to disable them for fixed-function pipeline to work
    typedef void (APIENTRY* PFNGLUSEPROGRAMPROC)(uint32_t program);
    static PFNGLUSEPROGRAMPROC glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    if (glUseProgram) glUseProgram(0);

    // Reset active texture to 0
    typedef void (APIENTRY* PFNGLACTIVETEXTUREPROC)(uint32_t texture);
    static PFNGLACTIVETEXTUREPROC glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    if (glActiveTexture) glActiveTexture(0x84C0); // GL_TEXTURE0

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float menuW = 500, menuH = 350;
    float menuX = (screenW - menuW) / 2.0f;
    float menuY = (screenH - menuH) / 2.0f;

    // Background
    DrawRect(menuX, menuY, menuW, menuH, 0.07f, 0.07f, 0.09f, 0.98f);
    
    // Sidebar
    DrawRect(menuX, menuY, 140, menuH, 0.1f, 0.1f, 0.12f, 1.0f);
    DrawText(menuX + 15, menuY + 30, "CLOUDDLC", 0.43f, 0.34f, 0.81f);

    auto TabBtn = [&](int id, const char* label, float y) {
        bool hover = IsInArea(menuX, y, 140, 40);
        bool active = (s_activeTab == id);
        if (active) DrawRect(menuX, y, 4, 40, 0.43f, 0.34f, 0.81f, 1.0f);
        if (hover && mouse.leftDown) s_activeTab = id;
        DrawText(menuX + 20, y + 25, label, active ? 1 : 0.6f, active ? 1 : 0.6f, active ? 1 : 0.6f);
    };

    TabBtn(0, "COMBAT", menuY + 60);
    TabBtn(1, "VISUALS", menuY + 100);
    TabBtn(2, "SETTINGS", menuY + 140);

    // Combat
    if (s_activeTab == 0) {
        float cx = menuX + 160;
        float cy = menuY + 30;
        DrawText(cx, cy, "AIM ASSIST SETTINGS", 0.8f, 0.8f, 0.8f);

        // Toggle
        DrawText(cx, cy + 50, "Enabled", 1, 1, 1);
        bool enabled = GlobalConfig::aimbotEnabled;
        DrawRect(cx + 250, cy + 35, 40, 20, enabled ? 0.43f : 0.25f, enabled ? 0.34f : 0.25f, enabled ? 0.81f : 0.25f, 1.0f);
        if (IsInArea(cx + 250, cy + 35, 40, 20) && mouse.leftDown && !mouse.wasLeftDown) {
            GlobalConfig::aimbotEnabled = !GlobalConfig::aimbotEnabled;
        }

        // Speed
        DrawText(cx, cy + 90, "Smooth Speed", 0.9f, 0.9f, 0.9f);
        float sw = 250;
        DrawRect(cx, cy + 105, sw, 6, 1, 1, 1, 0.1f);
        float speedRatio = GlobalConfig::aimbotSpeed / 100.0f;
        DrawRect(cx, cy + 105, sw * speedRatio, 6, 0.43f, 0.34f, 0.81f, 1.0f);
        if (IsInArea(cx, cy + 95, sw, 20) && mouse.leftDown) {
            GlobalConfig::aimbotSpeed = ((mouse.x - cx) / sw) * 100.0f;
            if (GlobalConfig::aimbotSpeed < 1) GlobalConfig::aimbotSpeed = 1;
            if (GlobalConfig::aimbotSpeed > 100) GlobalConfig::aimbotSpeed = 100;
        }

        // FOV
        DrawText(cx, cy + 140, "FOV Range", 0.9f, 0.9f, 0.9f);
        DrawRect(cx, cy + 155, sw, 6, 1, 1, 1, 0.1f);
        float fovRatio = GlobalConfig::aimbotFov / 360.0f;
        DrawRect(cx, cy + 155, sw * fovRatio, 6, 0.43f, 0.34f, 0.81f, 1.0f);
        if (IsInArea(cx, cy + 145, sw, 20) && mouse.leftDown) {
            GlobalConfig::aimbotFov = ((mouse.x - cx) / sw) * 360.0f;
            if (GlobalConfig::aimbotFov < 10) GlobalConfig::aimbotFov = 10;
        }
    }

    // Custom Cursor for Menu
    DrawRect(mouse.x - 2, mouse.y - 2, 4, 4, 1, 1, 1, 1.0f);

    mouse.wasLeftDown = mouse.leftDown;

    glPopMatrix();
    glPopAttrib();
}

void Gui::HandleInput(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!s_visible) return;
    if (msg == WM_LBUTTONDOWN) mouse.leftDown = true;
    if (msg == WM_LBUTTONUP) mouse.leftDown = false;
}
