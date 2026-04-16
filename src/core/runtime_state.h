#pragma once

#include <atomic>

namespace RuntimeState {
    inline std::atomic<bool> RequestUnload{ false };
}

