#include "Hook.h"
#include <iostream>
#include <psapi.h>

twglSwapBuffers owglSwapBuffers = nullptr;

// Helper to find a memory region within 2GB of the target
static void* AllocateNear(void* target) {
    uintptr_t base = (uintptr_t)target;
    for (uintptr_t addr = base - 0x7FFFF000; addr < base + 0x7FFFF000; addr += 0x10000) {
        void* p = VirtualAlloc((void*)addr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (p) return p;
    }
    return nullptr;
}

bool Hook::Detour(void* pTarget, void* pDetour, void** ppOriginal, size_t size) {
    if (size < 5) return false;

    // Use a 5-byte relative JMP (E9)
    // We need a relay because pDetour is likely > 2GB away
    void* pRelay = AllocateNear(pTarget);
    if (!pRelay) return false;

    // Relay: 14-byte absolute jump to our actual detour
    uint8_t relayCode[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    *(uintptr_t*)(&relayCode[6]) = (uintptr_t)pDetour;
    memcpy(pRelay, relayCode, 14);

    // Trampoline: Original code + jump back
    void* pTrampoline = VirtualAlloc(nullptr, size + 14, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pTrampoline) return false;
    memcpy(pTrampoline, pTarget, size);
    uintptr_t jumpBackAddr = (uintptr_t)pTarget + size;
    uint8_t jmpBack[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    *(uintptr_t*)(&jmpBack[6]) = jumpBackAddr;
    memcpy((uint8_t*)pTrampoline + size, jmpBack, 14);
    if (ppOriginal) *ppOriginal = pTrampoline;

    // Final overwrite: 5-byte relative JMP to Relay
    DWORD oldProtect;
    VirtualProtect(pTarget, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    memset(pTarget, 0x90, size); // NOP out for safety
    uint8_t jmp5[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
    int32_t relAddr = (int32_t)((uintptr_t)pRelay - (uintptr_t)pTarget - 5);
    *(int32_t*)(&jmp5[1]) = relAddr;
    memcpy(pTarget, jmp5, 5);

    VirtualProtect(pTarget, size, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pTarget, size);
    return true;
}

static bool SmartIATHook(HMODULE hMod, const char* targetModName, const char* targetFuncName, void* pTargetAddr, void* pDetour, void** ppOriginal) {
    uintptr_t base = (uintptr_t)hMod;
    if (!base) return false;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)base;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(base + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;
    
    DWORD importVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!importVA) return false;
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(base + importVA);
    
    bool hooked = false;
    while (importDesc->Name) {
        const char* modName = (const char*)(base + importDesc->Name);
        if (_stricmp(modName, targetModName) == 0) {
            PIMAGE_THUNK_DATA firstThunk = (PIMAGE_THUNK_DATA)(base + importDesc->FirstThunk);
            PIMAGE_THUNK_DATA origFirstThunk = (PIMAGE_THUNK_DATA)(base + importDesc->OriginalFirstThunk);
            
            while (firstThunk->u1.Function) {
                bool match = false;
                
                // 1. Check by current address
                if ((void*)firstThunk->u1.Function == pTargetAddr) match = true;
                
                // 2. Check by name if available (more robust if already hooked)
                if (!match && !(origFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
                    PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)(base + origFirstThunk->u1.AddressOfData);
                    if (_stricmp(importByName->Name, targetFuncName) == 0) match = true;
                }

                if (match) {
                    DWORD oldProtect;
                    if (VirtualProtect(&firstThunk->u1.Function, sizeof(uintptr_t), PAGE_READWRITE, &oldProtect)) {
                        if (ppOriginal && !*ppOriginal) *ppOriginal = (void*)firstThunk->u1.Function;
                        firstThunk->u1.Function = (uintptr_t)pDetour;
                        VirtualProtect(&firstThunk->u1.Function, sizeof(uintptr_t), oldProtect, &oldProtect);
                        hooked = true;
                    }
                }
                firstThunk++;
                origFirstThunk++;
            }
        }
        importDesc++;
    }
    return hooked;
}

bool Hook::IATHook(const char* moduleName, const char* functionModule, const char* functionName, void* pDetour, void** ppOriginal) {
    void* targetAddr = (void*)GetProcAddress(GetModuleHandleA(functionModule), functionName);
    if (!targetAddr) return false;

    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
        bool totalSuccess = false;
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            if (SmartIATHook(hMods[i], functionModule, functionName, targetAddr, pDetour, ppOriginal)) {
                char szModName[MAX_PATH];
                if (GetModuleFileNameA(hMods[i], szModName, sizeof(szModName))) {
                    std::cout << "[+] IAT Hooked [" << functionName << "] in: " << (strrchr(szModName, '\\') ? strrchr(szModName, '\\') + 1 : szModName) << "\n";
                }
                totalSuccess = true;
            }
        }
        return totalSuccess;
    }
    return false;
}

// --- MinHook API Implementation (Compatibility Layer) ---

extern "C" MH_STATUS WINAPI MH_Initialize(VOID) {
    return MH_OK;
}

extern "C" MH_STATUS WINAPI MH_Uninitialize(VOID) {
    return MH_OK;
}

extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal) {
    // We use our stable 5-byte Detour as the backend for MinHook API
    if (Hook::Detour(pTarget, pDetour, ppOriginal, 5)) {
        return MH_OK;
    }
    return MH_UNKNOWN;
}

extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget) {
    // For our simplified implementation, CreateHook already enables it
    return MH_OK;
}

extern "C" MH_STATUS WINAPI MH_DisableHook(LPVOID pTarget) {
    return MH_OK;
}
