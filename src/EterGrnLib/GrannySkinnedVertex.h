#pragma once
#include <cstdint>

struct TSkinnedPNTVertex
{
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float w0, w1, w2, w3;
    uint32_t i0, i1, i2, i3;
};

struct TSkinnedPNT2Vertex
{
    float px, py, pz;
    float nx, ny, nz;
    float u0, v0;
    float u1, v1;
    float w0, w1, w2, w3;
    uint32_t i0, i1, i2, i3;
};
