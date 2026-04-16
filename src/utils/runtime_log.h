#pragma once

#include <Windows.h>
#include <cstdio>
#include <cstdarg>

namespace RuntimeLog {
    inline bool g_consoleInitialized = false;

    inline void InitConsole()
    {
        if (g_consoleInitialized) return;

        if (AllocConsole()) {
            FILE* stream = nullptr;
            freopen_s(&stream, "CONOUT$", "w", stdout);
            freopen_s(&stream, "CONOUT$", "w", stderr);
            freopen_s(&stream, "CONIN$", "r", stdin);
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleTitleA("traithrow runtime log");
            g_consoleInitialized = true;
        }
    }

    inline void Logf(const char* fmt, ...)
    {
        char buffer[1024] = {};
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");

        if (g_consoleInitialized) {
            std::fprintf(stdout, "%s\n", buffer);
            std::fflush(stdout);
        }
    }
}
