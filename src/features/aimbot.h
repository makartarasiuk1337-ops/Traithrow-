#pragma once
#include <basehandle.h>
#include <usercmd.h>

namespace Aimbot {
    // Run aimbot logic.
    // - If called from CreateMove hook: pass the cmd to silently modify it.
    // - If cmd == nullptr: called from EndScene, uses SetViewAngles() directly.
    void Run(CUserCmd* cmd = nullptr);
}
