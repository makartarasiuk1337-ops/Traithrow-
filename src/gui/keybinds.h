#pragma once

#include <imgui.h>

namespace Keybinds {
    // Renders the keybind hotkey widget.
    // label: Text label to show.
    // key: Pointer to the variable holding the selected keycode (VK_*)
    // size_arg: Size of the button widget.
    void Hotkey(const char* label, int* key, const ImVec2& size_arg = ImVec2(100, 0));
}
