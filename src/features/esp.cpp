#include <d3d9.h>
#include <Windows.h>

#include <basehandle.h>
#include <ai_activity.h>
#include <cbase.h>
#include <ff/c_ff_player.h>
#include <icliententitylist.h>
#include <cdll_int.h>

#include "esp.h"
#include "config.h"
#include "sdk.h"
#include "debug_state.h"
#include "runtime_log.h"

#include <imgui.h>

namespace ESP {
    namespace {
        struct ScreenBounds {
            float minX;
            float maxX;
            float minY;
            float maxY;
        };

        static DWORD g_lastSummaryLogTick = 0;
        static DWORD g_lastEntityLogTick[256] = {};

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

        static bool W2S(const Vector& world, ImVec2& screen)
        {
            if (!SDK::g_pEngineClient) {
                DebugState::EspW2SFailCount++;
                return false;
            }

            IVEngineClient* engine = reinterpret_cast<IVEngineClient*>(SDK::g_pEngineClient);
            const VMatrix& matrix = engine->WorldToScreenMatrix();

            float clipW = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];
            if (clipW < 0.001f) {
                DebugState::EspW2SFailCount++;
                return false;
            }

            float clipX = matrix[0][0] * world.x + matrix[0][1] * world.y + matrix[0][2] * world.z + matrix[0][3];
            float clipY = matrix[1][0] * world.x + matrix[1][1] * world.y + matrix[1][2] * world.z + matrix[1][3];

            float ndcX = clipX / clipW;
            float ndcY = clipY / clipW;

            int screenW = 0;
            int screenH = 0;
            engine->GetScreenSize(screenW, screenH);
            if (screenW <= 0 || screenH <= 0) {
                DebugState::EspW2SFailCount++;
                return false;
            }

            screen.x = (screenW * 0.5f) + (ndcX * screenW) * 0.5f;
            screen.y = (screenH * 0.5f) - (ndcY * screenH) * 0.5f;
            return true;
        }

        static bool BuildScreenBounds(const Vector& worldMins, const Vector& worldMaxs, ScreenBounds& outBounds, int& projectedPoints)
        {
            Vector corners[8] = {
                Vector(worldMins.x, worldMins.y, worldMins.z),
                Vector(worldMins.x, worldMaxs.y, worldMins.z),
                Vector(worldMaxs.x, worldMaxs.y, worldMins.z),
                Vector(worldMaxs.x, worldMins.y, worldMins.z),
                Vector(worldMins.x, worldMins.y, worldMaxs.z),
                Vector(worldMins.x, worldMaxs.y, worldMaxs.z),
                Vector(worldMaxs.x, worldMaxs.y, worldMaxs.z),
                Vector(worldMaxs.x, worldMins.y, worldMaxs.z)
            };

            bool initialized = false;
            projectedPoints = 0;
            for (int index = 0; index < 8; index++) {
                ImVec2 p;
                if (!W2S(corners[index], p)) continue;
                projectedPoints++;

                if (!initialized) {
                    outBounds.minX = outBounds.maxX = p.x;
                    outBounds.minY = outBounds.maxY = p.y;
                    initialized = true;
                } else {
                    if (p.x < outBounds.minX) outBounds.minX = p.x;
                    if (p.x > outBounds.maxX) outBounds.maxX = p.x;
                    if (p.y < outBounds.minY) outBounds.minY = p.y;
                    if (p.y > outBounds.maxY) outBounds.maxY = p.y;
                }
            }

            DebugState::EspProjectedPoints += projectedPoints;
            return initialized && projectedPoints >= 2;
        }

        static ImU32 GetEspColor()
        {
            int index = Config::ESPColorPreset;
            if (index < 0) index = 0;
            if (index >= static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]))) index = 0;
            return kPalette[index];
        }

        static void DrawHealthBar(ImDrawList* drawList, const ScreenBounds& bounds, int health)
        {
            float barX = bounds.minX - 6.0f;
            float barH = bounds.maxY - bounds.minY;
            if (barH <= 1.0f) return;

            float hp = static_cast<float>(max(0, min(100, health))) / 100.0f;
            drawList->AddRectFilled(ImVec2(barX - 2.0f, bounds.minY), ImVec2(barX + 2.0f, bounds.maxY), IM_COL32(22, 22, 22, 180));
            drawList->AddRectFilled(
                ImVec2(barX - 2.0f, bounds.minY + barH * (1.0f - hp)),
                ImVec2(barX + 2.0f, bounds.maxY),
                IM_COL32(static_cast<int>((1 - hp) * 255.0f), static_cast<int>(hp * 220.0f), 45, 240)
            );
        }
    }

    void Render(IDirect3DDevice9* /*pDevice*/)
    {
        if (!Config::ESPEnabled) return;
        if (!SDK::g_pEngineClient || !SDK::g_pEntityList) return;

        IVEngineClient* engine = reinterpret_cast<IVEngineClient*>(SDK::g_pEngineClient);
        IClientEntityList* entityList = reinterpret_cast<IClientEntityList*>(SDK::g_pEntityList);
        if (!engine->IsInGame()) return;

        int localId = engine->GetLocalPlayer();
        C_FFPlayer* local = reinterpret_cast<C_FFPlayer*>(entityList->GetClientEntity(localId));
        if (!local) return;

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        if (!drawList) return;

        int screenW = 0;
        int screenH = 0;
        engine->GetScreenSize(screenW, screenH);

        DebugState::EspScanned = 0;
        DebugState::EspValidTargets = 0;
        DebugState::EspDrawnBoxes = 0;
        DebugState::EspNameFailCount = 0;
        DebugState::EspW2SFailCount = 0;
        DebugState::EspExceptionCount = 0;
        DebugState::EspFlatBoxRejectCount = 0;
        DebugState::EspProjectedPoints = 0;

        ImU32 color = GetEspColor();
        float thickness = Config::ESPThickness > 0.5f ? Config::ESPThickness : 0.5f;

        int maxClients = engine->GetMaxClients();
        for (int index = 1; index <= maxClients; index++) {
            if (index == localId) continue;
            DebugState::EspScanned++;

            __try {
                C_FFPlayer* target = reinterpret_cast<C_FFPlayer*>(entityList->GetClientEntity(index));
                if (!target || target->IsDormant() || !target->IsAlive()) continue;

                Vector mins;
                Vector maxs;
                target->GetRenderBoundsWorldspace(mins, maxs);

                ScreenBounds bounds = {};
                int projectedPoints = 0;
                if (!BuildScreenBounds(mins, maxs, bounds, projectedPoints)) {
                    DWORD now = GetTickCount();
                    if (index >= 0 && index < 256 && now - g_lastEntityLogTick[index] > 1000) {
                        RuntimeLog::Logf("[esp] entity %d skipped: projected=%d", index, projectedPoints);
                        g_lastEntityLogTick[index] = now;
                    }
                    continue;
                }

                float width = bounds.maxX - bounds.minX;
                float height = bounds.maxY - bounds.minY;
                if (width <= 1.0f || height <= 1.0f) {
                    DebugState::EspFlatBoxRejectCount++;
                    continue;
                }

                DebugState::EspValidTargets++;

                if (Config::ESPBox) {
                    drawList->AddRect(
                        ImVec2(bounds.minX + 1.0f, bounds.minY + 1.0f),
                        ImVec2(bounds.maxX + 1.0f, bounds.maxY + 1.0f),
                        IM_COL32(0, 0, 0, 120),
                        0.0f,
                        0,
                        thickness + 0.6f
                    );
                    drawList->AddRect(
                        ImVec2(bounds.minX, bounds.minY),
                        ImVec2(bounds.maxX, bounds.maxY),
                        color,
                        0.0f,
                        0,
                        thickness
                    );
                    DebugState::EspDrawnBoxes++;
                }

                if (Config::ESPLine && screenW > 0 && screenH > 0) {
                    drawList->AddLine(
                        ImVec2(screenW * 0.5f, static_cast<float>(screenH) - 6.0f),
                        ImVec2((bounds.minX + bounds.maxX) * 0.5f, bounds.maxY),
                        color,
                        thickness
                    );
                }

                if (Config::ESPHealth) {
                    DrawHealthBar(drawList, bounds, target->GetHealth());
                }

                if (Config::ESPName) {
                    player_info_t info = {};
                    if (engine->GetPlayerInfo(index, &info)) {
                        ImVec2 textSize = ImGui::CalcTextSize(info.name);
                        float centerX = (bounds.minX + bounds.maxX) * 0.5f;
                        drawList->AddText(
                            ImVec2(centerX - textSize.x * 0.5f, bounds.minY - textSize.y - 2.0f),
                            IM_COL32(255, 255, 255, 230),
                            info.name
                        );
                    } else {
                        DebugState::EspNameFailCount++;
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                DebugState::EspExceptionCount++;
            }
        }

        DWORD now = GetTickCount();
        if (now - g_lastSummaryLogTick > 1000) {
            RuntimeLog::Logf(
                "[esp] summary: scanned=%d valid=%d boxes=%d w2s_fail=%d flat=%d ex=%d projected_pts=%d",
                DebugState::EspScanned,
                DebugState::EspValidTargets,
                DebugState::EspDrawnBoxes,
                DebugState::EspW2SFailCount,
                DebugState::EspFlatBoxRejectCount,
                DebugState::EspExceptionCount,
                DebugState::EspProjectedPoints
            );
            g_lastSummaryLogTick = now;
        }
    }
}

