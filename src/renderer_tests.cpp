#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "renderer.h"
#include "tests.h"

static AppMemory* s_Memory;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static RendererState MakeRenderer(int32 w, int32 h)
{
    Arena* arena = &s_Memory->test_scratch;
    RendererState rs = {};
    RendererInit(&rs, arena, w, h);
    return rs;
}

static Pixel MakePixel(uint8 r, uint8 g, uint8 b)
{
    Pixel p = {};
    p.r = r; p.g = g; p.b = b; p.a = 0;
    return p;
}

static bool PixelEq(Pixel a, Pixel b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static bool TestRenderer_ClearFillsAllPixels(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel red = MakePixel(255, 0, 0);
    ClearBuffer(&rs, red);

    int32 total = rs.width * rs.height;
    for (int32 i = 0; i < total; ++i)
        if (!PixelEq(rs.pixels[i], red)) return false;

    return true;
}

static bool TestRenderer_DrawRect_BasicFill(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(10, 10);

    Pixel bg  = MakePixel(0, 0, 0);
    Pixel fg  = MakePixel(0, 255, 0);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, 2, 3, 4, 2, fg); // x=2,y=3, w=4,h=2 → rows 3-4, cols 2-5

    for (int32 row = 0; row < rs.height; ++row)
    {
        for (int32 col = 0; col < rs.width; ++col)
        {
            Pixel* px = rs.pixels + row * rs.width + col;
            bool inside = (col >= 2 && col < 6 && row >= 3 && row < 5);
            if (inside)
            {
                if (!PixelEq(*px, fg)) return false;
            }
            else
            {
                if (!PixelEq(*px, bg)) return false;
            }
        }
    }
    return true;
}

static bool TestRenderer_DrawRect_ClipsLeft(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel bg = MakePixel(0, 0, 0);
    Pixel fg = MakePixel(255, 255, 0);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, -2, 0, 4, 4, fg); // only cols 0-1 should be filled

    for (int32 row = 0; row < rs.height; ++row)
    {
        for (int32 col = 0; col < rs.width; ++col)
        {
            Pixel* px = rs.pixels + row * rs.width + col;
            bool inside = (col >= 0 && col < 2 && row >= 0 && row < 4);
            if (inside)
            {
                if (!PixelEq(*px, fg)) return false;
            }
            else
            {
                if (!PixelEq(*px, bg)) return false;
            }
        }
    }
    return true;
}

static bool TestRenderer_DrawRect_ClipsRight(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel bg = MakePixel(0, 0, 0);
    Pixel fg = MakePixel(0, 0, 255);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, 6, 0, 4, 4, fg); // only cols 6-7 should be filled

    for (int32 row = 0; row < rs.height; ++row)
    {
        for (int32 col = 0; col < rs.width; ++col)
        {
            Pixel* px = rs.pixels + row * rs.width + col;
            bool inside = (col >= 6 && col < 8 && row >= 0 && row < 4);
            if (inside)
            {
                if (!PixelEq(*px, fg)) return false;
            }
            else
            {
                if (!PixelEq(*px, bg)) return false;
            }
        }
    }
    return true;
}

static bool TestRenderer_DrawRect_ClipsTop(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel bg = MakePixel(0, 0, 0);
    Pixel fg = MakePixel(128, 0, 128);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, 0, -2, 4, 4, fg); // only rows 0-1 should be filled

    for (int32 row = 0; row < rs.height; ++row)
    {
        for (int32 col = 0; col < rs.width; ++col)
        {
            Pixel* px = rs.pixels + row * rs.width + col;
            bool inside = (col >= 0 && col < 4 && row >= 0 && row < 2);
            if (inside)
            {
                if (!PixelEq(*px, fg)) return false;
            }
            else
            {
                if (!PixelEq(*px, bg)) return false;
            }
        }
    }
    return true;
}

static bool TestRenderer_DrawRect_ClipsBottom(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel bg = MakePixel(0, 0, 0);
    Pixel fg = MakePixel(0, 128, 128);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, 0, 6, 4, 4, fg); // only rows 6-7 should be filled

    for (int32 row = 0; row < rs.height; ++row)
    {
        for (int32 col = 0; col < rs.width; ++col)
        {
            Pixel* px = rs.pixels + row * rs.width + col;
            bool inside = (col >= 0 && col < 4 && row >= 6 && row < 8);
            if (inside)
            {
                if (!PixelEq(*px, fg)) return false;
            }
            else
            {
                if (!PixelEq(*px, bg)) return false;
            }
        }
    }
    return true;
}

static bool TestRenderer_DrawRect_FullyOutOfBounds(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel bg = MakePixel(50, 50, 50);
    Pixel fg = MakePixel(200, 200, 200);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, 100, 100, 4, 4, fg); // entirely outside — buffer must be unchanged

    int32 total = rs.width * rs.height;
    for (int32 i = 0; i < total; ++i)
        if (!PixelEq(rs.pixels[i], bg)) return false;

    return true;
}

static bool TestRenderer_DrawRect_ZeroSize(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = MakeRenderer(8, 8);

    Pixel bg = MakePixel(10, 20, 30);
    Pixel fg = MakePixel(40, 50, 60);
    ClearBuffer(&rs, bg);
    DrawRect(&rs, 2, 2, 0, 0, fg); // zero size — buffer must be unchanged

    int32 total = rs.width * rs.height;
    for (int32 i = 0; i < total; ++i)
        if (!PixelEq(rs.pixels[i], bg)) return false;

    return true;
}

static bool TestRenderer_InitFailsOnArenaExhaustion(void)
{
    // Build a tiny arena backed by a stack buffer that is too small for any pixel buffer.
    uint8  tiny_buf[4] = {};
    Arena  tiny_arena  = {};
    tiny_arena.base   = tiny_buf;
    tiny_arena.size   = sizeof(tiny_buf);
    tiny_arena.offset = 0;

    RendererState rs = {};
    bool result = RendererInit(&rs, &tiny_arena, 64, 64); // needs 64*64*4 = 16 384 bytes

    return result == false && rs.pixels == 0;
}

static const TestEntry k_RendererTests[] = {
    TEST_ENTRY(TestRenderer_ClearFillsAllPixels),
    TEST_ENTRY(TestRenderer_DrawRect_BasicFill),
    TEST_ENTRY(TestRenderer_DrawRect_ClipsLeft),
    TEST_ENTRY(TestRenderer_DrawRect_ClipsRight),
    TEST_ENTRY(TestRenderer_DrawRect_ClipsTop),
    TEST_ENTRY(TestRenderer_DrawRect_ClipsBottom),
    TEST_ENTRY(TestRenderer_DrawRect_FullyOutOfBounds),
    TEST_ENTRY(TestRenderer_DrawRect_ZeroSize),
    TEST_ENTRY(TestRenderer_InitFailsOnArenaExhaustion),
};

void RunRendererTests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_RendererTests, sizeof(k_RendererTests) / sizeof(k_RendererTests[0]),
                 passed, total);
}
