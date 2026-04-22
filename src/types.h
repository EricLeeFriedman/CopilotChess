#pragma once

// Explicitly sized integer types.
// Use these throughout the codebase instead of built-in types wherever
// the bit width is meaningful. Backed by MSVC built-in sizes on Win32/Win64.

typedef signed   __int8  int8;
typedef signed   __int16 int16;
typedef signed   __int32 int32;
typedef signed   __int64 int64;

typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

// This codebase targets 64-bit Windows only. Catch a 32-bit build at compile time.
static_assert(sizeof(size_t) == sizeof(uint64), "size_t must be 64 bits — 32-bit builds are not supported");
