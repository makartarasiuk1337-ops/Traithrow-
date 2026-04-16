#pragma once

#include <cstdint>
#include <unordered_map>

#include "config.h"
#include "sdk.h"
#include "runtime_log.h"

#include <icvar.h>
#include <tier1/convar.h>
#include <tier1/iconvar.h>

namespace Misc {
    namespace detail {
        inline std::unordered_map<ConCommandBase*, int> g_originalFlags;
        inline bool g_bypassApplied = false;
        constexpr std::uintptr_t kConCommandBaseFlagsOffset = 0x14;

        inline int* GetFlagsPtr(ConCommandBase* commandBase)
        {
            if (!commandBase) return nullptr;
            auto base = reinterpret_cast<std::uintptr_t>(commandBase);
            return reinterpret_cast<int*>(base + kConCommandBaseFlagsOffset);
        }

        inline void ApplyCheatBypass()
        {
            if (g_bypassApplied) return;
            if (!SDK::g_pCvar) return;

            ICvar* icvar = reinterpret_cast<ICvar*>(SDK::g_pCvar);
            int changed = 0;

            for (ICvar::Iterator it(icvar); it.IsValid(); it.Next()) {
                ConCommandBase* base = it.Get();
                if (!base) continue;
                if (!base->IsFlagSet(FCVAR_CHEAT)) continue;

                int* flags = GetFlagsPtr(base);
                if (!flags) continue;

                g_originalFlags[base] = *flags;
                *flags &= ~FCVAR_CHEAT;
                changed++;
            }

            ConVar* svCheats = icvar->FindVar("sv_cheats");
            if (svCheats) {
                svCheats->SetValue(1);
            }

            g_bypassApplied = true;
            RuntimeLog::Logf("[misc] sv_bypass enabled: unlocked %d cvars", changed);
        }

        inline void RestoreCheatFlags()
        {
            if (!g_bypassApplied && g_originalFlags.empty()) return;

            int restored = 0;
            for (const auto& entry : g_originalFlags) {
                ConCommandBase* base = entry.first;
                int oldFlags = entry.second;
                int* flags = GetFlagsPtr(base);
                if (!flags) continue;

                *flags = oldFlags;
                restored++;
            }

            g_originalFlags.clear();
            g_bypassApplied = false;
            RuntimeLog::Logf("[misc] sv_bypass disabled: restored %d cvars", restored);
        }
    }

    inline void OnFrameTick()
    {
        if (Config::BypassSVCheats) {
            detail::ApplyCheatBypass();
        } else {
            detail::RestoreCheatFlags();
        }
    }

    inline void Shutdown()
    {
        detail::RestoreCheatFlags();
    }
}

