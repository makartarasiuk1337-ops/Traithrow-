#pragma once

#include <d3d9.h>

namespace GUI {
    // Initialize ImGui context and backends
    void Init(IDirect3DDevice9* pDevice, HWND hWnd);

    // Shutdown ImGui
    void Shutdown();

    // Full frame render (NewFrame + windows + EndFrame + draw) — legacy
    void Render(IDirect3DDevice9* pDevice);

    // Draw only the menu window — used when hooks.cpp owns the ImGui frame
    void RenderFrame();

    // Handle device reset (release/recreate DX9 resources)
    void OnDeviceLost();
    void OnDeviceReset(IDirect3DDevice9* pDevice);

    // State
    inline bool bInitialized = false;
}
