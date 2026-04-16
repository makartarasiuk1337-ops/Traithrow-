cmake_minimum_required(VERSION 3.15)
project(traithrow LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output DLL
add_library(traithrow SHARED
    # Core
    src/core/dllmain.cpp
    src/core/hooks.cpp
    src/core/hooks.h
    src/core/config.h
    src/core/misc.h
    src/core/runtime_state.h

    # GUI
    src/gui/gui.cpp
    src/gui/gui.h
    src/gui/keybinds.cpp
    src/gui/keybinds.h

    # Features
    src/features/aimbot.h
    src/features/aimbot.cpp
    src/features/esp.h
    src/features/esp.cpp

    # SDK
    src/sdk/netvars.h
    src/sdk/netvars.cpp
    src/sdk/sdk.h
    src/sdk/typeinfo.h

    # Utils
    src/utils/math_utils.h
    src/utils/debug_state.h
    src/utils/runtime_log.h

    # MinHook sources
    deps/minhook/minhook-master/src/buffer.c
    deps/minhook/minhook-master/src/hook.c
    deps/minhook/minhook-master/src/trampoline.c
    deps/minhook/minhook-master/src/hde/hde32.c

    # ImGui core
    deps/imgui/imgui-master/imgui.cpp
    deps/imgui/imgui-master/imgui_draw.cpp
    deps/imgui/imgui-master/imgui_tables.cpp
    deps/imgui/imgui-master/imgui_widgets.cpp
    deps/imgui/imgui-master/imgui_demo.cpp

    # ImGui backends - DirectX 9 + Win32
    deps/imgui/imgui-master/backends/imgui_impl_dx9.cpp
    deps/imgui/imgui-master/backends/imgui_impl_win32.cpp
)

target_include_directories(traithrow PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/features
    ${CMAKE_SOURCE_DIR}/src/sdk
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/resources
    ${CMAKE_SOURCE_DIR}/deps/minhook/minhook-master/include
    ${CMAKE_SOURCE_DIR}/deps/minhook/minhook-master/src
    ${CMAKE_SOURCE_DIR}/deps/imgui/imgui-master
    ${CMAKE_SOURCE_DIR}/deps/imgui/imgui-master/backends
    # Fortress Forever 2013 SDK (MP)
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/public
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/public/tier0
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/public/tier1
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/public/tier2
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/public/tier3
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/public/mathlib
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/common
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/client
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/client/ff
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/client/ff/fx
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/client/ff/hud
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/client/ff/vgui
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared/ff
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared/ff/weapons
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared/ff/grenades
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared/ff/projectiles
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared/ff/buildables
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src/game/shared/ff/buildables/base
    ${CMAKE_SOURCE_DIR}/deps/fortressforever-sdk-final/mp/src
)

# Link DirectX 9 and Windows libs
target_link_libraries(traithrow PRIVATE
    d3d9
    dwmapi
)

# Preprocessor definitions
target_compile_definitions(traithrow PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
    CLIENT_DLL
    WIN32
    _WIN32
    _WINDOWS
)

# Set output name
set_target_properties(traithrow PROPERTIES
    OUTPUT_NAME "traithrow"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
