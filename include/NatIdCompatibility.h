#pragma once
// natID 3.2.7 exports a typedef using an MSVC class declaration in Buffer.h.
// IntelliSense interprets that declaration as an incomplete class. Suppress
// only that parser-only declaration; actual compiler/SDK ABI stay unchanged.
#if defined(__INTELLISENSE__)
#include <mu/muLib.h>
#pragma push_macro("MU_VS_COMPILER")
#undef MU_VS_COMPILER
#include <mem/Buffer.h>
#pragma pop_macro("MU_VS_COMPILER")
#endif
