#include <Windows.h>
#include <thread>
#include <iostream>
#include <gl/GL.h>
#include <vector>
#include <string>
#include "JNIHelper.h"
#include "Aimbot.h"
#include "Hook.h"
#include "Gui.h"
#include "DiscordRPC.h"

// --- Global Hook Data ---
WNDPROC oWndProc = nullptr;
HWND gameHwnd = nullptr;

LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN) {
        // std::cout << "[D] Key Down: " << wParam << "\n";
        if (wParam == 'R') {
            GlobalConfig::aimbotEnabled = !GlobalConfig::aimbotEnabled;
            std::cout << "[+] AimAssist " << (GlobalConfig::aimbotEnabled ? "ENABLED" : "DISABLED") << "\n";
            return 0;
        }
        if (wParam == VK_INSERT) {
            std::cout << "[D] Insert Pressed - Toggling GUI\n";
            Gui::Toggle();
            return 0;
        }
    }
    
    if (Gui::IsVisible()) {
        Gui::HandleInput(msg, wParam, lParam);
        // Block mouse click if in menu area
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) return 0;
    }

    return CallWindowProc(oWndProc, hwnd, msg, wParam, lParam);
}

// Global hook for wglSwapBuffers - PROPER FPS SYNC
// owglSwapBuffers is defined in Hook.cpp

BOOL WINAPI hkwglSwapBuffers(HDC hdc) {
    __try {
        if (hdc) {
            HWND currentHwnd = WindowFromDC(hdc);
            
            static bool windowLogged = false;
            if (!windowLogged) {
                std::cout << "[D] Hooked HDC " << hdc << " -> HWND " << currentHwnd << " (Target: " << gameHwnd << ")\n";
                windowLogged = true;
            }

            if (currentHwnd == gameHwnd) {
                Gui::Render(hdc);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    
    __try {
        return owglSwapBuffers(hdc);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
}

bool g_Running = true;
void AimbotThread() {
    while (g_Running) {
        __try {
            if (GlobalConfig::aimbotEnabled) {
                Aimbot::OnUpdate();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // Anti-crash: Prevent JNI errors from crashing the game
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Cool down
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // ~200 TPS
    }
}

void MainThread(HMODULE hModule) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    
    std::cout << "[+] CloudDLC Internal v1.5.1 injected!\n";
    std::cout << "[+] Mode: Smart IAT Hook (Stable)\n";

    // Discord RPC Initialization
    DiscordRPC::Initialize("1482452528359674157"); // User Application ID
    DiscordRPC::Activity act;
    char windowTitle[256];
    HWND mcHwnd = FindWindowA("GLFW30", nullptr);
    if (!mcHwnd) mcHwnd = GetForegroundWindow();
    GetWindowTextA(mcHwnd, windowTitle, 256);
    std::string title = windowTitle;
    if (title.empty()) title = "Minecraft";
    
    act.details = "Injected in " + title;
    act.state = "Role: Dev";
    act.largeImage = "https://cdn.discordapp.com/attachments/1482591752794673152/1482592074380345435/cloud_8.gif?ex=69b782f7&is=69b63177&hm=00676447868c159fde56b5d2055300d46fac5c1cd40beb835eaa5d1d83316aac&";
    act.largeText = "CloudDLC";
    
    // Tiny sleep to ensure Discord has processed our handshake
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    DiscordRPC::UpdateActivity(act);
    std::cout << "[+] Discord RPC Activity Sent\n";

    if (!JNIHelper::Initialize()) {
        std::cout << "[-] JNI Init Failed.\n";
        FreeLibraryAndExitThread(hModule, 0);
        return;
    }

    if (!JNIHelper::InitializeCache()) {
        std::cout << "[-] JNI Cache incomplete.\n";
    }

    gameHwnd = FindWindowA("GLFW30", nullptr);
    if (!gameHwnd) gameHwnd = GetForegroundWindow();

    oWndProc = (WNDPROC)SetWindowLongPtr(gameHwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
    
    // Use IAT Hook - MUCH SAFER
    // Try both opengl32!wglSwapBuffers and gdi32!SwapBuffers as they are often interchangeable
    bool hooked = false;
    if (Hook::IATHook(nullptr, "opengl32.dll", "wglSwapBuffers", (void*)hkwglSwapBuffers, (void**)&owglSwapBuffers)) {
        hooked = true;
    }
    
    if (Hook::IATHook(nullptr, "gdi32.dll", "SwapBuffers", (void*)hkwglSwapBuffers, (void**)&owglSwapBuffers)) {
        hooked = true;
    }

    if (hooked) {
        std::cout << "[+] Safe IAT FPS Sync Attached! \n";
    } else {
        std::cout << "[-] Failed to attach IAT hook on any module.\n";
        std::cout << "[!] Skipping unsafe inline hook to prevent crash.\n";
        // If IAT fails, we DON'T fallback to MinHook as it's known to crash for this user.
    }

    std::cout << "[+] Built for Luna Client. Project finished (CloudDLC).\n";
    
    std::thread aThread(AimbotThread);
    aThread.detach();

    while (!GetAsyncKeyState(VK_END)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }

    g_Running = false;

    SetWindowLongPtr(gameHwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    if (f) fclose(f);
    DiscordRPC::Shutdown();
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}
