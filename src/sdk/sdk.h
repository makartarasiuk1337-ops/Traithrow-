#pragma once

#include <Windows.h>
#include <cstdint>
#include "runtime_log.h"

/*
 * Source SDK 2013 Interface Grabber
 * 
 * Source Engine exposes its subsystems through interfaces.
 * Each DLL exports a CreateInterface function that returns
 * a pointer to the requested interface by name.
 *
 * Fortress Forever (Source 2013 branch) uses these key modules:
 *   - client.dll   → IBaseClientDLL, IClientEntityList
 *   - engine.dll   → IVEngineClient, IEngineTrace
 *   - vguimatsurface.dll → ISurface
 *   - vstdlib.dll  → ICvar
 */

namespace SDK {

    // CreateInterface function signature
    using CreateInterfaceFn = void* (*)(const char* name, int* returnCode);

    // Get CreateInterface export from a loaded module
    inline CreateInterfaceFn GetFactory(const char* moduleName)
    {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule)
            return nullptr;

        return reinterpret_cast<CreateInterfaceFn>(
            GetProcAddress(hModule, "CreateInterface")
        );
    }

    // Get interface pointer by name from a module
    inline void* GetInterface(const char* moduleName, const char* interfaceName)
    {
        auto factory = GetFactory(moduleName);
        if (!factory)
            return nullptr;

        int retCode = 0;
        void* pInterface = factory(interfaceName, &retCode);
        return pInterface;
    }

    // Wait for a module to be loaded (useful during injection)
    inline HMODULE WaitForModule(const char* moduleName, DWORD timeoutMs = 30000)
    {
        DWORD elapsed = 0;
        HMODULE hMod = nullptr;
        while (!(hMod = GetModuleHandleA(moduleName))) {
            if (elapsed >= timeoutMs)
                return nullptr;
            Sleep(100);
            elapsed += 100;
        }
        return hMod;
    }

    // Avoid WinAPI macro collision with interface constant name below.
#ifdef STUDIO_RENDER
#undef STUDIO_RENDER
#endif

    // ============================================================
    // Known interface version strings for Source 2013
    // ============================================================
    namespace Interfaces {
        // client.dll
        constexpr const char* CLIENT           = "VClient017";
        constexpr const char* CLIENT_ENTLIST   = "VClientEntityList003";
        constexpr const char* CLIENT_PREDICTION = "VClientPrediction001";

        // engine.dll
        constexpr const char* ENGINE_CLIENT    = "VEngineClient014";
        constexpr const char* ENGINE_TRACE     = "EngineTraceClient004";
        constexpr const char* ENGINE_MODEL     = "VModelInfoClient004";
        constexpr const char* ENGINE_RENDER    = "VEngineRenderView014"; // Updated for FF 2013
        constexpr const char* ENGINE_EFFECTS   = "VEngineEffects001";
        constexpr const char* ENGINE_SOUND     = "IEngineSoundClient003";
        constexpr const char* GAME_EVENTS      = "GAMEEVENTSMANAGER002";
        constexpr const char* DEBUG_OVERLAY    = "VDebugOverlay004";

        // vguimatsurface.dll
        constexpr const char* VGUI_SURFACE     = "VGUI_Surface030"; // Updated for FF 2013

        // vgui2.dll
        constexpr const char* VGUI_PANEL       = "VGUI_Panel009";

        // vstdlib.dll
        constexpr const char* CVAR             = "VEngineCvar007";

        // materialsystem.dll
        constexpr const char* MATERIAL_SYSTEM  = "VMaterialSystem080";

        // inputsystem.dll
        constexpr const char* INPUT_SYSTEM     = "InputSystemVersion001";

        // studiorender.dll
        constexpr const char* STUDIO_RENDER    = "VStudioRender025"; // Updated for FF 2013
    }

    // ============================================================
    // Global interface pointers (populated on init)
    // ============================================================
    inline void* g_pClient        = nullptr;
    inline void* g_pEntityList    = nullptr;
    inline void* g_pEngineClient  = nullptr;
    inline void* g_pEngineTrace   = nullptr;
    inline void* g_pModelInfo     = nullptr;
    inline void* g_pSurface       = nullptr;
    inline void* g_pPanel         = nullptr;
    inline void* g_pCvar          = nullptr;
    inline void* g_pMaterialSys   = nullptr;
    inline void* g_pStudioRender  = nullptr;
    inline void* g_pGameEvents    = nullptr;
    inline void* g_pDebugOverlay  = nullptr;
    inline void* g_pInputSystem   = nullptr;

    // ============================================================
    // Initialize all interfaces
    // ============================================================
    inline bool Init()
    {
        // Wait for critical modules
        if (!WaitForModule("client.dll")) {
            RuntimeLog::Logf("[traithrow] Timeout waiting for client.dll");
            return false;
        }
        if (!WaitForModule("engine.dll")) {
            RuntimeLog::Logf("[traithrow] Timeout waiting for engine.dll");
            return false;
        }

        auto FindInterface = [](const char* moduleName, const char* prefix, int maxVersion = 100) -> void* {
            char buffer[128];
            for (int i = 0; i < maxVersion; i++) {
                sprintf(buffer, "%s%03d", prefix, i);
                void* p = GetInterface(moduleName, buffer);
                if (p) {
                    RuntimeLog::Logf("[traithrow] Found %s interface: %s", moduleName, buffer);
                    return p;
                }
            }
            return nullptr;
        };

        // Try to find the exact strings first or fallback to brute-force
        g_pClient       = GetInterface("client.dll", Interfaces::CLIENT);
        if (!g_pClient) g_pClient = FindInterface("client.dll", "VClient");

        g_pEntityList   = GetInterface("client.dll", Interfaces::CLIENT_ENTLIST);
        if (!g_pEntityList) g_pEntityList = FindInterface("client.dll", "VClientEntityList");

        g_pEngineClient = GetInterface("engine.dll", Interfaces::ENGINE_CLIENT);
        if (!g_pEngineClient) g_pEngineClient = FindInterface("engine.dll", "VEngineClient");

        g_pEngineTrace  = GetInterface("engine.dll", Interfaces::ENGINE_TRACE);
        g_pModelInfo    = GetInterface("engine.dll", Interfaces::ENGINE_MODEL);
        g_pGameEvents   = GetInterface("engine.dll", Interfaces::GAME_EVENTS);
        g_pDebugOverlay = GetInterface("engine.dll", Interfaces::DEBUG_OVERLAY);
        g_pSurface      = GetInterface("vguimatsurface.dll", Interfaces::VGUI_SURFACE);
        g_pPanel        = GetInterface("vgui2.dll", Interfaces::VGUI_PANEL);
        g_pCvar         = GetInterface("vstdlib.dll", Interfaces::CVAR);
        g_pMaterialSys  = GetInterface("materialsystem.dll", Interfaces::MATERIAL_SYSTEM);
        g_pStudioRender = GetInterface("studiorender.dll", Interfaces::STUDIO_RENDER);
        g_pInputSystem  = GetInterface("inputsystem.dll", Interfaces::INPUT_SYSTEM);

        // Verify critical interfaces
        if (!g_pClient) RuntimeLog::Logf("[traithrow] Failed to get IBaseClientDLL (%s)", Interfaces::CLIENT);
        if (!g_pEntityList) RuntimeLog::Logf("[traithrow] Failed to get IClientEntityList (%s)", Interfaces::CLIENT_ENTLIST);
        if (!g_pEngineClient) RuntimeLog::Logf("[traithrow] Failed to get IVEngineClient (%s)", Interfaces::ENGINE_CLIENT);

        if (!g_pClient || !g_pEntityList || !g_pEngineClient)
            return false;

        return true;
    }
}
