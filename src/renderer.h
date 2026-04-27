#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"

// BGRA pixel — matches the DIB format used by StretchDIBits (BI_RGB with 32 bpp,
// blue in the low byte).
struct Pixel
{
    uint8 b;
    uint8 g;
    uint8 r;
    uint8 a; // unused / padding; kept for 4-byte alignment
};

struct RendererState
{
    Pixel*      pixels;       // CPU-side pixel buffer (width * height elements)
    int32       width;
    int32       height;
    BITMAPINFO  bmi;          // filled once at startup; used every frame by StretchDIBits
};

// Allocate pixel buffer from the renderer arena and fill bmi.
// width and height must be positive.
bool         RendererInit(RendererState* rs, Arena* arena, int32 width, int32 height);

// Fill the entire pixel buffer with a solid colour.
void         ClearBuffer(RendererState* rs, Pixel color);

// Fill an axis-aligned rectangle with a solid colour.
// Clips silently to the buffer bounds.
void         DrawRect(RendererState* rs, int32 x, int32 y, int32 w, int32 h, Pixel color);

// Blit the pixel buffer to the window client area via StretchDIBits.
void         PresentFrame(RendererState* rs, HWND window);
