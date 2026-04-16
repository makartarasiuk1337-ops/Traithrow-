#pragma once

#include <Windows.h>
#include <d3d9.h>

// Forward declare CUserCmd for CreateMove
class CUserCmd;

namespace Hooks {
    // Initialize all hooks (EndScene, WndProc, CreateMove)
    bool Init();

    // Remove all hooks and cleanup
    void Shutdown();

    // Typedefs for hooked functions
    using fnEndScene = HRESULT(APIENTRY*)(IDirect3DDevice9*);
    using fnReset = HRESULT(APIENTRY*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    using fnCreateMove = bool(__stdcall*)(float, CUserCmd*);

    // Original trampolines
    inline fnEndScene oEndScene = nullptr;
    inline fnReset oReset = nullptr;
    inline fnCreateMove oCreateMove = nullptr;
    inline WNDPROC oWndProc = nullptr;

    // Game window handle
    inline HWND hGameWindow = nullptr;
}
