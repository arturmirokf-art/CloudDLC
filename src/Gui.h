#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <string>

class Gui {
public:
    static void Render(HDC hdc);
    static void Toggle();
    static bool IsVisible();
    static void HandleInput(UINT msg, WPARAM wParam, LPARAM lParam);
    static void Initialize(HDC hdc);

private:
    static void DrawRect(float x, float y, float w, float h, float r, float g, float b, float a);
    static void DrawText(float x, float y, const std::string& text, float r, float g, float b);
    static bool Button(float x, float y, float w, float h, const std::string& text, bool active);
    static void Slider(float x, float y, float w, float h, const std::string& label, float& value, float min, float max);

    static bool s_visible;
    static int s_activeTab; // 0: Combat, 1: Visuals, 2: Movement, 3: Settings
};
