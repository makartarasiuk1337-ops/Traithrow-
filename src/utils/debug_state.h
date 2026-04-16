#pragma once

namespace DebugState {
    inline bool EndSceneHookEnabled = false;
    inline bool ResetHookEnabled = false;
    inline bool CreateMoveHookEnabled = false;
    inline bool WndProcHookEnabled = false;

    inline int EspScanned = 0;
    inline int EspValidTargets = 0;
    inline int EspDrawnBoxes = 0;
    inline int EspNameFailCount = 0;
    inline int EspW2SFailCount = 0;
    inline int EspExceptionCount = 0;
    inline int EspFlatBoxRejectCount = 0;
    inline int EspProjectedPoints = 0;

    inline int AimbotCandidates = 0;
    inline float AimbotBestFov = 0.0f;
    inline bool AimbotFoundTarget = false;
    inline int AimbotPredictionMs = 0;
}
