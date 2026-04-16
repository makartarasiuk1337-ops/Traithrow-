#pragma once
#include <d3d9.h>

namespace ESP {
    // Called every EndScene — draws boxes, names, health on enemy players+bots
    void Render(IDirect3DDevice9* pDevice);
}
