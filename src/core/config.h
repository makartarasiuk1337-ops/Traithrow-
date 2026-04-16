#pragma once

namespace Config {
    // Aimbot
    inline bool AimbotEnabled = true;
    inline int AimbotKeybind = 0;
    inline float AimbotFOV = 10.0f;
    inline float AimbotSmooth = 5.0f;
    inline bool AimbotSilent = false;

    // Visuals (ESP)
    inline bool ESPEnabled = true;
    inline bool ESPBox = true;
    inline bool ESPName = true;
    inline bool ESPHealth = true;
    inline bool ESPLine = false;
    inline float ESPThickness = 1.5f;
    inline int ESPColorPreset = 0;
    
    // Misc
    inline bool BypassSVCheats = false;

    // GUI
    inline bool MenuOpen = true;
}
