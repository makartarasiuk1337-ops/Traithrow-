// ============================================================
// hooks.cpp  —  D3D9 EndScene/Reset, WndProc, CreateMove
//
// CreateMove is hooked through IClientMode vtable index 21.
// FindClientMode scans HudUpdate bytes for the MOV ECX/EAX
// instruction that references g_pClientMode.
//
// Aimbot::Run()  is called from hkCreateMove (with cmd).
// ESP::Render()  is called from hkEndScene.
// ============================================================
#include <Windows.h>
#include <d3d9.h>
#include <basehandle.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include <ai_activity.h>
#include <cbase.h>
#include <ff/c_ff_player.h>
#include <icliententitylist.h>
#include <cdll_int.h>
#include <usercmd.h>

// Our modules
#include "hooks.h"
#include "gui.h"
#include "sdk.h"
#include "config.h"
#include "aimbot.h"
#include "esp.h"
#include "misc.h"
#include "debug_state.h"
#include "runtime_log.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx9.h>

// MinHook
#include <MinHook.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hooks {

    // -------------------------------------------------------
    // Trampoline types (defined in hooks.h as inline statics)
    // -------------------------------------------------------
    // fnEndScene oEndScene, fnReset oReset,
    // fnCreateMove oCreateMove, WNDPROC oWndProc, HWND hGameWindow
    // are all declared in hooks.h

    // CreateMove is __thiscall (virtual member of IClientMode).
    // In 32-bit MSVC, __fastcall with a dummy EDX parameter is used
    // to intercept __thiscall from outside the class.
    using fnCreateMoveFast = bool(__fastcall*)(void*, void*, float, CUserCmd*);
    static fnCreateMoveFast oCreateMoveFast = nullptr;
    static void* gEndSceneTarget = nullptr;
    static void* gResetTarget = nullptr;
    static void* gCreateMoveTarget = nullptr;
    static bool gMinHookInitialized = false;
    static bool gEspFaulted = false;
    static bool gAimbotFaulted = false;
    static bool gCreateMoveAvailable = false;
    static bool gWndProcHooked = false;
    static bool gInsertWasDown = false;
    static DWORD gLastInsertToggleTick = 0;
    static DWORD gLastFallbackAimLogTick = 0;

    static void ToggleMenuOpen()
    {
        Config::MenuOpen = !Config::MenuOpen;
        gLastInsertToggleTick = GetTickCount();
    }

    static bool IsReadableAddress(const void* address, size_t size)
    {
        if (!address || size == 0) return false;
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(address, &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT) return false;
        if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) return false;

        uintptr_t start = reinterpret_cast<uintptr_t>(address);
        uintptr_t end = start + size;
        uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return end <= regionEnd;
    }

    static bool IsExecutableAddress(const void* address)
    {
        if (!address) return false;
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(address, &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT) return false;
        if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) return false;

        DWORD execMask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        return (mbi.Protect & execMask) != 0;
    }

    static bool IsAddressInModule(const void* address, HMODULE module)
    {
        if (!address || !module) return false;
        uintptr_t base = reinterpret_cast<uintptr_t>(module);
        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!IsReadableAddress(dos, sizeof(IMAGE_DOS_HEADER))) return false;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (!IsReadableAddress(nt, sizeof(IMAGE_NT_HEADERS))) return false;
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

        uintptr_t addr = reinterpret_cast<uintptr_t>(address);
        uintptr_t size = static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
        return addr >= base && addr < (base + size);
    }

    static bool IsValidClientMode(void* clientMode)
    {
        if (!clientMode) return false;
        if (!IsReadableAddress(clientMode, sizeof(void*))) return false;

        void** cmVTable = *(void***)clientMode;
        if (!cmVTable) return false;
        if (!IsReadableAddress(cmVTable, sizeof(void*) * 22)) return false;
        if (!IsExecutableAddress(cmVTable[21])) return false;

        HMODULE clientModule = GetModuleHandleA("client.dll");
        return IsAddressInModule(cmVTable[21], clientModule);
    }

    static void RollbackHooks()
    {
        if (gCreateMoveTarget) {
            MH_DisableHook(gCreateMoveTarget);
            MH_RemoveHook(gCreateMoveTarget);
            gCreateMoveTarget = nullptr;
        }
        if (gResetTarget) {
            MH_DisableHook(gResetTarget);
            MH_RemoveHook(gResetTarget);
            gResetTarget = nullptr;
        }
        if (gEndSceneTarget) {
            MH_DisableHook(gEndSceneTarget);
            MH_RemoveHook(gEndSceneTarget);
            gEndSceneTarget = nullptr;
        }

        oCreateMoveFast = nullptr;
        oEndScene = nullptr;
        oReset = nullptr;
        gCreateMoveAvailable = false;
        gWndProcHooked = false;
        DebugState::EndSceneHookEnabled = false;
        DebugState::ResetHookEnabled = false;
        DebugState::CreateMoveHookEnabled = false;
        DebugState::WndProcHookEnabled = false;

        if (gMinHookInitialized) {
            MH_Uninitialize();
            gMinHookInitialized = false;
        }
    }

    // -----------------------------------------------------------
    // WndProc hook — INSERT toggles the menu
    // -----------------------------------------------------------
    static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
            // Filter key-repeat messages to avoid double toggle while held.
            if ((lParam & (1u << 30)) == 0) {
                ToggleMenuOpen();
            }
            return 0;
        }
        if (Config::MenuOpen && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return 1;

        return CallWindowProcW(oWndProc, hWnd, uMsg, wParam, lParam);
    }

    // -----------------------------------------------------------
    // CreateMove hook  (IClientMode::CreateMove, vtable index 21)
    // Called every game tick — ideal place for aimbot.
    // -----------------------------------------------------------
    static bool __fastcall hkCreateMove(void* thisptr, void* /*edx*/,
                                         float flInputSampleTime, CUserCmd* cmd)
    {
        if (!oCreateMoveFast)
            return false;

        bool ret = oCreateMoveFast(thisptr, nullptr, flInputSampleTime, cmd);
        if (!gCreateMoveAvailable)
            return ret;

        if (cmd && cmd->command_number && !gAimbotFaulted) {
            __try {
                Aimbot::Run(cmd);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                gAimbotFaulted = true;
                Config::AimbotEnabled = false;
                RuntimeLog::Logf("[traithrow] Aimbot::Run exception, disabling aimbot.");
            }
        }
        return ret;
    }

    // -----------------------------------------------------------
    // EndScene hook — ImGui init, ESP draw, GUI render
    // -----------------------------------------------------------
    static HRESULT APIENTRY hkEndScene(IDirect3DDevice9* pDevice) {
        // First call: initialise ImGui + hook WndProc
        if (!GUI::bInitialized) {
            D3DDEVICE_CREATION_PARAMETERS params;
            pDevice->GetCreationParameters(&params);
            hGameWindow = params.hFocusWindow;
            SetLastError(0);
            oWndProc = (WNDPROC)SetWindowLongPtrW(hGameWindow, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            if (!oWndProc && GetLastError() != 0) {
                gWndProcHooked = false;
                DebugState::WndProcHookEnabled = false;
                RuntimeLog::Logf("[traithrow] WndProc hook fail");
            } else {
                gWndProcHooked = true;
                DebugState::WndProcHookEnabled = true;
                RuntimeLog::Logf("[traithrow] WndProc hook ok");
            }
            GUI::Init(pDevice, hGameWindow);
        }

        {
            // Keep polling path as a robust fallback for games that swallow WndProc keys.
            bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
            if (insertDown && !gInsertWasDown) {
                DWORD now = GetTickCount();
                if (now - gLastInsertToggleTick > 125) {
                    ToggleMenuOpen();
                }
            }
            gInsertWasDown = insertDown;
        }

        // Begin a new ImGui frame so ESP can use the DrawList
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Misc::OnFrameTick();

        // --- ESP (uses ImGui background DrawList) ---
        if (!gEspFaulted) {
            DebugState::EspScanned = 0;
            DebugState::EspValidTargets = 0;
            DebugState::EspDrawnBoxes = 0;
            DebugState::EspNameFailCount = 0;
            __try {
                ESP::Render(pDevice);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                gEspFaulted = true;
                Config::ESPEnabled = false;
                RuntimeLog::Logf("[traithrow] ESP::Render exception, disabling ESP.");
            }
        } else {
            DebugState::EspScanned = 0;
            DebugState::EspValidTargets = 0;
            DebugState::EspDrawnBoxes = 0;
            DebugState::EspNameFailCount = 0;
            DebugState::EspW2SFailCount = 0;
            DebugState::EspExceptionCount = 0;
            DebugState::EspFlatBoxRejectCount = 0;
            DebugState::EspProjectedPoints = 0;
        }

        if (!gCreateMoveAvailable && !gAimbotFaulted && Config::AimbotEnabled && !Config::AimbotSilent) {
            __try {
                Aimbot::Run(nullptr);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                gAimbotFaulted = true;
                Config::AimbotEnabled = false;
                RuntimeLog::Logf("[traithrow] Aimbot fallback exception, disabling aimbot.");
            }

            DWORD now = GetTickCount();
            if (now - gLastFallbackAimLogTick > 2000) {
                RuntimeLog::Logf("[traithrow] Aimbot fallback path active (CreateMove disabled).");
                gLastFallbackAimLogTick = now;
            }
        } else if (!Config::AimbotEnabled || gAimbotFaulted || Config::AimbotSilent) {
            DebugState::AimbotFoundTarget = false;
            DebugState::AimbotPredictionMs = 0;
        }

        // --- Menu ---
        GUI::RenderFrame(); // see gui.h / gui.cpp

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        return oEndScene(pDevice);
    }

    // -----------------------------------------------------------
    // Reset hook — handles device lost/reset for ImGui
    // -----------------------------------------------------------
    static HRESULT APIENTRY hkReset(IDirect3DDevice9* pDevice,
                                     D3DPRESENT_PARAMETERS* pParams)
    {
        GUI::OnDeviceLost();
        HRESULT hr = oReset(pDevice, pParams);
        if (SUCCEEDED(hr)) GUI::OnDeviceReset(pDevice);
        return hr;
    }

    // -----------------------------------------------------------
    // Helper: obtain the D3D9 device vtable via a null device
    // -----------------------------------------------------------
    static bool GetD3D9VTable(void** pVTable, size_t size) {
        IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (!pD3D) return false;

        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed     = TRUE;
        pp.SwapEffect   = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = GetDesktopWindow();

        IDirect3DDevice9* pDev = nullptr;
        if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF,
                                       pp.hDeviceWindow,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                       &pp, &pDev)))
        {
            pD3D->Release();
            return false;
        }

        memcpy(pVTable, *(void***)pDev, size);
        pDev->Release();
        pD3D->Release();
        return true;
    }

    // -----------------------------------------------------------
    // FindClientMode — scans IBaseClientDLL::HudUpdate (vtable 11)
    // for a MOV ECX/EAX, [g_pClientMode] pattern.
    //
    // Pattern 0x8B 0x0D = MOV ECX, [imm32]
    //         0xA1      = MOV EAX, [imm32]
    // The imm32 is the address of g_pClientMode (a void**).
    // -----------------------------------------------------------
    static void* FindClientMode() {
        if (!SDK::g_pClient) return nullptr;
        HMODULE clientModule = GetModuleHandleA("client.dll");
        if (!clientModule) return nullptr;
        if (!IsReadableAddress(SDK::g_pClient, sizeof(void*))) return nullptr;

        // HudUpdate is at vtable index 11 in IBaseClientDLL
        void** clientVTable = *(void***)SDK::g_pClient;
        if (!clientVTable) return nullptr;
        if (!IsReadableAddress(clientVTable, sizeof(void*) * 12)) return nullptr;

        uintptr_t fnHudUpdate = reinterpret_cast<uintptr_t>(clientVTable[11]);
        if (!fnHudUpdate) return nullptr;
        if (!IsExecutableAddress(reinterpret_cast<void*>(fnHudUpdate))) return nullptr;
        if (!IsAddressInModule(reinterpret_cast<void*>(fnHudUpdate), clientModule)) return nullptr;

        auto* p = (uint8_t*)fnHudUpdate;
        for (int i = 0; i < 128; i++) {
            if (!IsReadableAddress(p + i, 6)) break;

            // MOV ECX, [imm32]
            if (p[i] == 0x8B && p[i+1] == 0x0D) {
                uintptr_t pPtr = *(uintptr_t*)(p + i + 2);
                if (!IsReadableAddress(reinterpret_cast<void*>(pPtr), sizeof(void*))) continue;
                void* candidate = *(void**)pPtr;
                if (IsValidClientMode(candidate)) return candidate;
            }
            // MOV EAX, [imm32]
            if (p[i] == 0xA1) {
                uintptr_t pPtr = *(uintptr_t*)(p + i + 1);
                if (!IsReadableAddress(reinterpret_cast<void*>(pPtr), sizeof(void*))) continue;
                void* candidate = *(void**)pPtr;
                if (IsValidClientMode(candidate)) return candidate;
            }
        }
        return nullptr;
    }

    // -----------------------------------------------------------
    // Init — set up all hooks
    // -----------------------------------------------------------
    bool Init() {
        // --- D3D9 hooks ---
        void* vTable[119] = {};
        if (!GetD3D9VTable(vTable, sizeof(vTable))) {
            RuntimeLog::Logf("[traithrow] Hook EndScene fail: vtable");
            return false;
        }
        if (MH_Initialize() != MH_OK) {
            RuntimeLog::Logf("[traithrow] MinHook init fail");
            return false;
        }
        gMinHookInitialized = true;
        gCreateMoveAvailable = false;
        gWndProcHooked = false;
        gEspFaulted = false;
        gAimbotFaulted = false;
        gLastFallbackAimLogTick = 0;
        DebugState::EndSceneHookEnabled = false;
        DebugState::ResetHookEnabled = false;
        DebugState::CreateMoveHookEnabled = false;
        DebugState::WndProcHookEnabled = false;

        gEndSceneTarget = vTable[42];
        gResetTarget = vTable[16];
        if (!gEndSceneTarget || !gResetTarget) {
            RuntimeLog::Logf("[traithrow] Hook EndScene/Reset fail: null target");
            RollbackHooks();
            return false;
        }
        if (!IsExecutableAddress(gEndSceneTarget) || !IsExecutableAddress(gResetTarget)) {
            RuntimeLog::Logf("[traithrow] Hook EndScene/Reset fail: target not executable");
            RollbackHooks();
            return false;
        }

        if (MH_CreateHook(gEndSceneTarget, &hkEndScene,
                          reinterpret_cast<void**>(&oEndScene)) != MH_OK) {
            RuntimeLog::Logf("[traithrow] Hook EndScene fail: create");
            RollbackHooks();
            return false;
        }
        RuntimeLog::Logf("[traithrow] Hook EndScene ok");
        if (MH_CreateHook(gResetTarget, &hkReset,
                          reinterpret_cast<void**>(&oReset)) != MH_OK) {
            RuntimeLog::Logf("[traithrow] Hook Reset fail: create");
            RollbackHooks();
            return false;
        }
        RuntimeLog::Logf("[traithrow] Hook Reset ok");

        // --- CreateMove hook via IClientMode vtable ---
        // IClientMode::CreateMove = vtable index 21
        // (counted from FF SDK iclientmode.h)
        void* pClientMode = FindClientMode();
        if (IsValidClientMode(pClientMode)) {
            void** cmVTable = *(void***)pClientMode;
            gCreateMoveTarget = cmVTable[21];
            if (!gCreateMoveTarget || !IsExecutableAddress(gCreateMoveTarget)) {
                gCreateMoveTarget = nullptr;
                oCreateMoveFast = nullptr;
                gCreateMoveAvailable = false;
                RuntimeLog::Logf("[traithrow] Hook CreateMove disabled: invalid target");
            } else if (MH_CreateHook(gCreateMoveTarget, &hkCreateMove,
                                     reinterpret_cast<void**>(&oCreateMoveFast)) == MH_OK) {
                gCreateMoveAvailable = true;
                RuntimeLog::Logf("[traithrow] Hook CreateMove ok");
            } else {
                gCreateMoveTarget = nullptr;
                oCreateMoveFast = nullptr;
                gCreateMoveAvailable = false;
                RuntimeLog::Logf("[traithrow] Hook CreateMove disabled: create failed");
            }
        } else {
            gCreateMoveTarget = nullptr;
            oCreateMoveFast = nullptr;
            gCreateMoveAvailable = false;
            RuntimeLog::Logf("[traithrow] Hook CreateMove disabled: ClientMode not found");
        }

        if (MH_EnableHook(gEndSceneTarget) != MH_OK) {
            RuntimeLog::Logf("[traithrow] Hook EndScene fail: enable");
            RollbackHooks();
            return false;
        }
        DebugState::EndSceneHookEnabled = true;
        if (MH_EnableHook(gResetTarget) != MH_OK) {
            RuntimeLog::Logf("[traithrow] Hook Reset fail: enable");
            RollbackHooks();
            return false;
        }
        DebugState::ResetHookEnabled = true;
        if (gCreateMoveAvailable && gCreateMoveTarget) {
            if (MH_EnableHook(gCreateMoveTarget) != MH_OK) {
                MH_RemoveHook(gCreateMoveTarget);
                gCreateMoveTarget = nullptr;
                oCreateMoveFast = nullptr;
                gCreateMoveAvailable = false;
                RuntimeLog::Logf("[traithrow] Hook CreateMove disabled: enable failed");
            } else {
                DebugState::CreateMoveHookEnabled = true;
            }
        }

        return true;
    }

    // -----------------------------------------------------------
    // Shutdown — restore everything
    // -----------------------------------------------------------
    void Shutdown() {
        if (gCreateMoveTarget) MH_DisableHook(gCreateMoveTarget);
        if (gResetTarget) MH_DisableHook(gResetTarget);
        if (gEndSceneTarget) MH_DisableHook(gEndSceneTarget);

        if (gCreateMoveTarget) MH_RemoveHook(gCreateMoveTarget);
        if (gResetTarget) MH_RemoveHook(gResetTarget);
        if (gEndSceneTarget) MH_RemoveHook(gEndSceneTarget);

        if (gMinHookInitialized) {
            MH_Uninitialize();
            gMinHookInitialized = false;
        }

        gCreateMoveTarget = nullptr;
        gResetTarget = nullptr;
        gEndSceneTarget = nullptr;
        oCreateMoveFast = nullptr;
        oEndScene = nullptr;
        oReset = nullptr;
        gCreateMoveAvailable = false;
        gWndProcHooked = false;
        DebugState::EndSceneHookEnabled = false;
        DebugState::ResetHookEnabled = false;
        DebugState::CreateMoveHookEnabled = false;
        DebugState::WndProcHookEnabled = false;

        if (hGameWindow && oWndProc)
            SetWindowLongPtrW(hGameWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        oWndProc = nullptr;
        hGameWindow = nullptr;
        Misc::Shutdown();
        GUI::Shutdown();
    }
}
