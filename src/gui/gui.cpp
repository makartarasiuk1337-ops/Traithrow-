#include "gui.h"
#include "config.h"
#include "sdk.h"
#include "runtime_state.h"
#include "runtime_log.h"
#include "IconsFontAwesome6.h"
#include "keybinds.h"

#include <cdll_int.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

namespace {
    int g_selectedTab = 0;
    float g_menuAlpha = 1.0f;
    float g_tabBlend[3] = { 1.0f, 0.0f, 0.0f };

    static const ImU32 kPalette[] = {
        IM_COL32(255, 74, 74, 230),
        IM_COL32(255, 140, 59, 230),
        IM_COL32(255, 210, 70, 230),
        IM_COL32(103, 222, 103, 230),
        IM_COL32(76, 193, 255, 230),
        IM_COL32(150, 121, 255, 230),
        IM_COL32(255, 116, 205, 230),
        IM_COL32(240, 240, 240, 230)
    };

    ImVec4 Lerp(const ImVec4& a, const ImVec4& b, float t)
    {
        return ImVec4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }

    void SetupStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        style.WindowRounding = 12.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 6.0f;
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.FramePadding = ImVec2(8.0f, 6.0f);
        style.ItemSpacing = ImVec2(8.0f, 9.0f);
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;

        ImVec4 bg0 = ImVec4(0.06f, 0.07f, 0.09f, 1.00f);
        ImVec4 bg1 = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
        ImVec4 bg2 = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        ImVec4 accent = ImVec4(0.00f, 0.72f, 0.98f, 1.00f);
        ImVec4 accentHi = ImVec4(0.18f, 0.88f, 1.00f, 1.00f);

        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.52f, 0.56f, 0.62f, 1.00f);
        colors[ImGuiCol_WindowBg] = bg0;
        colors[ImGuiCol_ChildBg] = bg1;
        colors[ImGuiCol_PopupBg] = bg0;
        colors[ImGuiCol_Border] = ImVec4(0.17f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBg] = bg1;
        colors[ImGuiCol_FrameBgHovered] = bg2;
        colors[ImGuiCol_FrameBgActive] = bg2;
        colors[ImGuiCol_Button] = bg2;
        colors[ImGuiCol_ButtonHovered] = Lerp(bg2, accent, 0.50f);
        colors[ImGuiCol_ButtonActive] = accentHi;
        colors[ImGuiCol_CheckMark] = accent;
        colors[ImGuiCol_SliderGrab] = accent;
        colors[ImGuiCol_SliderGrabActive] = accentHi;
        colors[ImGuiCol_Header] = bg2;
        colors[ImGuiCol_HeaderHovered] = Lerp(bg2, accent, 0.55f);
        colors[ImGuiCol_HeaderActive] = accentHi;
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = accent;
        colors[ImGuiCol_SeparatorActive] = accentHi;
    }

    void DrawPalette()
    {
        ImGui::Text("Color Palette");
        for (int index = 0; index < static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0])); index++) {
            ImGui::PushID(index);
            ImVec4 col = ImGui::ColorConvertU32ToFloat4(kPalette[index]);
            ImGui::PushStyleColor(ImGuiCol_Button, col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);

            if (Config::ESPColorPreset == index) {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            }

            if (ImGui::Button("   ", ImVec2(28.0f, 20.0f))) {
                Config::ESPColorPreset = index;
            }

            if (Config::ESPColorPreset == index) {
                ImGui::PopStyleVar();
            }

            ImGui::PopStyleColor(3);
            ImGui::PopID();

            if (index != 3 && index != 7) {
                ImGui::SameLine();
            }
        }
    }

    void RenderAimbotTab()
    {
        ImGui::TextColored(ImVec4(0.00f, 0.78f, 1.00f, 1.0f), ICON_FA_BULLSEYE " Aimbot");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("ab_main", ImVec2(0, 0), true);
        ImGui::Checkbox("Aimbot", &Config::AimbotEnabled);
        if (Config::AimbotEnabled) {
            Keybinds::Hotkey("Aimbot Key", &Config::AimbotKeybind, ImVec2(130, 0));
            ImGui::SliderFloat("FOV", &Config::AimbotFOV, 1.0f, 180.0f, "%.1f");
            ImGui::SliderFloat("Smooth", &Config::AimbotSmooth, 1.0f, 50.0f, "%.1f");
            ImGui::Checkbox("Silent Aim", &Config::AimbotSilent);
        }
        ImGui::EndChild();
    }

    void RenderESPTab()
    {
        ImGui::TextColored(ImVec4(0.00f, 0.78f, 1.00f, 1.0f), ICON_FA_EYE " ESP");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("esp_main", ImVec2(0, 0), true);
        ImGui::Checkbox("Enable ESP", &Config::ESPEnabled);
        if (Config::ESPEnabled) {
            ImGui::Checkbox("Box", &Config::ESPBox);
            ImGui::Checkbox("Name", &Config::ESPName);
            ImGui::Checkbox("Health", &Config::ESPHealth);
            ImGui::Checkbox("Line", &Config::ESPLine);
            ImGui::SliderFloat("Thickness", &Config::ESPThickness, 0.5f, 4.0f, "%.2f");
            ImGui::Spacing();
            DrawPalette();
        }
        ImGui::EndChild();
    }

    void RenderMiscTab()
    {
        ImGui::TextColored(ImVec4(0.00f, 0.78f, 1.00f, 1.0f), ICON_FA_WRENCH " Misc");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("misc_main", ImVec2(0, 0), true);
        ImGui::Checkbox("Bypass sv_cheats", &Config::BypassSVCheats);
        ImGui::Spacing();
        if (ImGui::Button("Unload DLL", ImVec2(-1, 36))) {
            RuntimeState::RequestUnload.store(true, std::memory_order_release);
            Config::MenuOpen = false;
            RuntimeLog::Logf("[gui] unload requested");
        }

        if (SDK::g_pEngineClient) {
            IVEngineClient* engine = reinterpret_cast<IVEngineClient*>(SDK::g_pEngineClient);
            ImGui::Spacing();
            if (engine->IsInGame()) ImGui::Text("Map: %s", engine->GetLevelName());
            else ImGui::Text("Status: Main Menu");
        }
        ImGui::EndChild();
    }
}

void GUI::Init(IDirect3DDevice9* pDevice, HWND hWnd)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 16.0f);

    float iconFontSize = 14.0f;
    static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF("c:\\Users\\pozor\\Documents\\traithrow\\src\\resources\\fa-solid-900.ttf", iconFontSize, &iconsConfig, iconsRanges);

    SetupStyle();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX9_Init(pDevice);
    bInitialized = true;
}

void GUI::Shutdown()
{
    if (!bInitialized) return;
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    bInitialized = false;
}

void GUI::Render(IDirect3DDevice9* /*pDevice*/)
{
    RenderFrame();
}

void GUI::OnDeviceLost()
{
    if (bInitialized) ImGui_ImplDX9_InvalidateDeviceObjects();
}

void GUI::OnDeviceReset(IDirect3DDevice9* /*pDevice*/)
{
    if (bInitialized) ImGui_ImplDX9_CreateDeviceObjects();
}

void GUI::RenderFrame()
{
    if (!bInitialized) return;

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime > 0.001f ? io.DeltaTime : 0.001f;

    float targetAlpha = Config::MenuOpen ? 1.0f : 0.0f;
    float alphaLerp = (dt * 10.0f < 1.0f) ? dt * 10.0f : 1.0f;
    g_menuAlpha += (targetAlpha - g_menuAlpha) * alphaLerp;

    for (int index = 0; index < 3; index++) {
        float target = (g_selectedTab == index) ? 1.0f : 0.0f;
        float blendLerp = (dt * 12.0f < 1.0f) ? dt * 12.0f : 1.0f;
        g_tabBlend[index] += (target - g_tabBlend[index]) * blendLerp;
    }

    if (g_menuAlpha <= 0.02f) return;

    ImGui::SetNextWindowSize(ImVec2(820.0f, 560.0f), ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menuAlpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (!Config::MenuOpen) flags |= ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("##traithrow_main", nullptr, flags)) {
        ImGui::BeginChild("##sidebar", ImVec2(210, 0), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Spacing();
        ImGui::SetCursorPosX(18);
        ImGui::TextColored(ImVec4(0.00f, 0.78f, 1.00f, 1.0f), "T R A I T H R O W");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const char* tabs[3] = {
            ICON_FA_BULLSEYE "  Aimbot",
            ICON_FA_EYE "  ESP",
            ICON_FA_WRENCH "  Misc"
        };

        for (int index = 0; index < 3; index++) {
            ImVec4 btn = Lerp(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), ImVec4(0.0f, 0.72f, 0.98f, 0.30f), g_tabBlend[index]);
            ImVec4 txt = Lerp(ImVec4(0.56f, 0.60f, 0.68f, 1.0f), ImVec4(0.90f, 0.97f, 1.00f, 1.0f), g_tabBlend[index]);

            ImGui::PushStyleColor(ImGuiCol_Button, btn);
            ImGui::PushStyleColor(ImGuiCol_Text, txt);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Lerp(btn, ImVec4(0.0f, 0.72f, 0.98f, 0.40f), 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.72f, 0.98f, 0.55f));

            if (ImGui::Button(tabs[index], ImVec2(194, 44))) {
                g_selectedTab = index;
            }
            ImGui::PopStyleColor(4);
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("##content", ImVec2(0, 0), false);
        ImGui::SetCursorPos(ImVec2(18, 18));
        ImGui::BeginChild("##content_inner", ImVec2(ImGui::GetContentRegionAvail().x - 18, ImGui::GetContentRegionAvail().y - 18), false);
        if (g_selectedTab == 0) RenderAimbotTab();
        else if (g_selectedTab == 1) RenderESPTab();
        else RenderMiscTab();
        ImGui::EndChild();
        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
}
