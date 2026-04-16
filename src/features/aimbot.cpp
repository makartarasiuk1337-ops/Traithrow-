#include <Windows.h>

#include <basehandle.h>
#include <ai_activity.h>
#include <cbase.h>
#include <ff/c_ff_player.h>
#include <icliententitylist.h>
#include <cdll_int.h>
#include <usercmd.h>

#include "aimbot.h"
#include "config.h"
#include "sdk.h"
#include "math_utils.h"
#include "debug_state.h"

namespace Aimbot {
    static bool IsSilentKeyPressed()
    {
        if (Config::AimbotKeybind == 0) return true;
        return (GetAsyncKeyState(Config::AimbotKeybind) & 0x8000) != 0;
    }

    void Run(CUserCmd* cmd)
    {
        if (!Config::AimbotEnabled) return;
        if (!SDK::g_pEngineClient || !SDK::g_pEntityList) return;

        const bool silentMode = Config::AimbotSilent;
        if (silentMode && !IsSilentKeyPressed()) {
            DebugState::AimbotCandidates = 0;
            DebugState::AimbotFoundTarget = false;
            DebugState::AimbotPredictionMs = 0;
            return;
        }

        IVEngineClient* engine = reinterpret_cast<IVEngineClient*>(SDK::g_pEngineClient);
        IClientEntityList* entityList = reinterpret_cast<IClientEntityList*>(SDK::g_pEntityList);
        if (!engine->IsInGame()) return;

        int localId = engine->GetLocalPlayer();
        C_FFPlayer* local = reinterpret_cast<C_FFPlayer*>(entityList->GetClientEntity(localId));
        if (!local || local->IsDormant() || !local->IsAlive()) return;

        Vector localEye = local->EyePosition();
        QAngle currentAngles;
        engine->GetViewAngles(currentAngles);

        float bestFov = Config::AimbotFOV;
        QAngle bestAngle = currentAngles;
        bool found = false;

        DebugState::AimbotCandidates = 0;
        DebugState::AimbotBestFov = bestFov;
        DebugState::AimbotFoundTarget = false;
        DebugState::AimbotPredictionMs = 0;

        int maxClients = engine->GetMaxClients();
        for (int index = 1; index <= maxClients; index++) {
            if (index == localId) continue;

            __try {
                C_FFPlayer* target = reinterpret_cast<C_FFPlayer*>(entityList->GetClientEntity(index));
                if (!target || target->IsDormant() || !target->IsAlive()) continue;

                DebugState::AimbotCandidates++;

                QAngle aimAngle;
                Math::CalcAngle(localEye, target->EyePosition(), aimAngle);

                QAngle delta = aimAngle - currentAngles;
                Math::ClampAngles(delta);
                float fov = delta.Length();

                if (fov < bestFov) {
                    bestFov = fov;
                    bestAngle = aimAngle;
                    found = true;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                continue;
            }
        }

        DebugState::AimbotBestFov = bestFov;
        if (!found) {
            DebugState::AimbotFoundTarget = false;
            return;
        }
        DebugState::AimbotFoundTarget = true;

        if (Config::AimbotSmooth > 1.0f) {
            QAngle delta = bestAngle - currentAngles;
            Math::ClampAngles(delta);
            bestAngle = currentAngles + delta / Config::AimbotSmooth;
        }
        Math::ClampAngles(bestAngle);

        if (cmd) {
            cmd->viewangles = bestAngle;
            if (!silentMode) {
                engine->SetViewAngles(bestAngle);
            }
        } else if (!silentMode) {
            engine->SetViewAngles(bestAngle);
        }
    }
}

