#include "keybinds.h"
#include <windows.h>
#include <stdio.h>

namespace Keybinds {
    static const char* KeyName(int key)
    {
        switch (key) {
        case 0: return "None";
        case VK_LBUTTON: return "Mouse1";
        case VK_RBUTTON: return "Mouse2";
        case VK_MBUTTON: return "Mouse3";
        case VK_XBUTTON1: return "Mouse4";
        case VK_XBUTTON2: return "Mouse5";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PageUp";
        case VK_NEXT: return "PageDown";
        case VK_SHIFT: return "Shift";
        case VK_CONTROL: return "Ctrl";
        case VK_MENU: return "Alt";
        case VK_SPACE: return "Space";
        case VK_TAB: return "Tab";
        case VK_CAPITAL: return "Caps";
        case VK_ESCAPE: return "Esc";
        case VK_UP: return "Up";
        case VK_DOWN: return "Down";
        case VK_LEFT: return "Left";
        case VK_RIGHT: return "Right";
        default:
            break;
        }

        static char nameBuf[24];
        if (key >= '0' && key <= '9') {
            sprintf_s(nameBuf, "%c", key);
            return nameBuf;
        }
        if (key >= 'A' && key <= 'Z') {
            sprintf_s(nameBuf, "%c", key);
            return nameBuf;
        }
        if (key >= VK_F1 && key <= VK_F24) {
            sprintf_s(nameBuf, "F%d", key - VK_F1 + 1);
            return nameBuf;
        }

        sprintf_s(nameBuf, "Key %d", key);
        return nameBuf;
    }

    void Hotkey(const char* label, int* key, const ImVec2& size_arg)
    {
        ImGui::PushID(label);

        static ImGuiID waiting_for_key_id = 0;
        static bool wait_for_release = false;
        static bool prev_down[256] = {};
        ImGuiID current_id = ImGui::GetID(label);

        ImGui::Text("%s", label);
        ImGui::SameLine();

        if (waiting_for_key_id == current_id) {
            ImGui::Button("[ Press Key ]", size_arg);

            bool any_down = false;
            for (int index = 1; index < 256; index++) {
                if ((GetAsyncKeyState(index) & 0x8000) != 0) {
                    any_down = true;
                    break;
                }
            }

            if (wait_for_release && !any_down) {
                wait_for_release = false;
            }

            if (!wait_for_release) {
                for (int index = 1; index < 256; index++) {
                    bool down = (GetAsyncKeyState(index) & 0x8000) != 0;
                    if (down && !prev_down[index]) {
                        if (index == VK_ESCAPE) *key = 0;
                        else *key = index;

                        waiting_for_key_id = 0;
                        wait_for_release = false;
                        break;
                    }
                }
            }
        }
        else {
            char keyName[64];
            sprintf_s(keyName, "[ %s ]", KeyName(*key));

            if (ImGui::Button(keyName, size_arg)) {
                waiting_for_key_id = current_id;
                wait_for_release = true;
            }
        }

        for (int index = 1; index < 256; index++) {
            prev_down[index] = (GetAsyncKeyState(index) & 0x8000) != 0;
        }

        if (waiting_for_key_id == 0) {
            wait_for_release = false;
        }

        ImGui::PopID();
    }
}

