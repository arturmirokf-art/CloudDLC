#include "minhook/MinHook.h"

class Hook {
public:
    static bool Detour(void* pTarget, void* pDetour, void** ppOriginal, size_t size);
    static bool Restore(void* pTarget, void* pOriginal, size_t size);
    static bool IATHook(const char* moduleName, const char* functionModule, const char* functionName, void* pDetour, void** ppOriginal);
};

typedef BOOL(WINAPI* twglSwapBuffers)(HDC hdc);
extern twglSwapBuffers owglSwapBuffers;

BOOL WINAPI hkwglSwapBuffers(HDC hdc);
