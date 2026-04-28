#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "renderer.h"

bool RendererInit(RendererState* rs, Arena* arena, int32 width, int32 height)
{
    ASSERT(rs);
    ASSERT(arena);
    ASSERT(width > 0 && height > 0);

    uint64 pixel_count    = (uint64)width * (uint64)height;
    uint64 byte_size      = pixel_count * sizeof(Pixel);
    uint64 aligned_offset = AlignUp(arena->offset, alignof(Pixel));

    // Return false (recoverable) if the arena lacks space for the pixel buffer.
    if (aligned_offset > arena->size || byte_size > arena->size - aligned_offset)
        return false;

    rs->width  = width;
    rs->height = height;
    rs->pixels = ArenaPushArray(arena, Pixel, pixel_count);

    rs->bmi                         = {};
    rs->bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    rs->bmi.bmiHeader.biWidth       = width;
    rs->bmi.bmiHeader.biHeight      = -height; // top-down; negative height means row 0 is the top
    rs->bmi.bmiHeader.biPlanes      = 1;
    rs->bmi.bmiHeader.biBitCount    = 32;
    rs->bmi.bmiHeader.biCompression = BI_RGB;

    return true;
}

void ClearBuffer(RendererState* rs, Pixel color)
{
    ASSERT(rs && rs->pixels);
    int32 total = rs->width * rs->height;
    Pixel* p    = rs->pixels;
    Pixel* end  = p + total;
    while (p < end)
        *p++ = color;
}

void DrawRect(RendererState* rs, int32 x, int32 y, int32 w, int32 h, Pixel color)
{
    ASSERT(rs && rs->pixels);

    // Clip to buffer bounds
    int32 x0 = x;
    int32 y0 = y;
    int32 x1 = x + w;
    int32 y1 = y + h;

    if (x0 < 0)          x0 = 0;
    if (y0 < 0)          y0 = 0;
    if (x1 > rs->width)  x1 = rs->width;
    if (y1 > rs->height) y1 = rs->height;

    if (x0 >= x1 || y0 >= y1) return; // nothing to draw after clipping

    for (int32 row = y0; row < y1; ++row)
    {
        Pixel* dst = rs->pixels + (uint64)row * (uint64)rs->width + (uint64)x0;
        Pixel* end = dst + (x1 - x0);
        while (dst < end)
            *dst++ = color;
    }
}

void DrawFilledCircle(RendererState* rs, int32 cx, int32 cy, int32 radius, Pixel color)
{
    ASSERT(rs && rs->pixels);
    if (radius <= 0) return;

    int32 r2 = radius * radius;
    for (int32 dy = -radius; dy <= radius; ++dy)
    {
        // Find the half-width at this row using integer arithmetic only.
        // Increment dx while (dx+1)^2 + dy^2 <= r^2
        int32 dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r2)
            ++dx;
        DrawRect(rs, cx - dx, cy + dy, dx * 2 + 1, 1, color);
    }
}

void PresentFrame(RendererState* rs, HWND window)
{
    ASSERT(rs && rs->pixels);
    ASSERT(window);

    HDC dc = GetDC(window);

    RECT client;
    GetClientRect(window, &client);
    int32 client_w = (int32)(client.right  - client.left);
    int32 client_h = (int32)(client.bottom - client.top);

    StretchDIBits(
        dc,
        0, 0, client_w, client_h,  // destination rectangle (entire client area)
        0, 0, rs->width, rs->height, // source rectangle (entire pixel buffer)
        rs->pixels,
        &rs->bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );

    ReleaseDC(window, dc);
}
