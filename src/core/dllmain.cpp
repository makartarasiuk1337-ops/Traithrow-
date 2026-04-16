#include <Windows.h>
#include "hooks.h"
#include "sdk.h"
#include "runtime_log.h"
#include "runtime_state.h"

static HMODULE g_hModule = nullptr;

static DWORD WINAPI MainThread(LPVOID lpParam)
{
    Sleep(2000);
    RuntimeLog::InitConsole();
    RuntimeLog::Logf("[traithrow] main thread started");

    if (!SDK::Init()) {
        RuntimeLog::Logf("[traithrow] SDK init fail");
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
    RuntimeLog::Logf("[traithrow] SDK init ok");

    if (!Hooks::Init()) {
        RuntimeLog::Logf("[traithrow] Hooks init fail");
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
    RuntimeLog::Logf("[traithrow] Hooks init ok");

    while (!RuntimeState::RequestUnload.load(std::memory_order_acquire)) {
        Sleep(100);
    }
    RuntimeLog::Logf("[traithrow] unload request received");

    Hooks::Shutdown();
    Sleep(500);

    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }

    return TRUE;
}
