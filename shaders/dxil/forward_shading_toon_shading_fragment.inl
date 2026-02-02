#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_forward_shading_toon_shading_fragment.inl"
#else
#include "release/_internal_forward_shading_toon_shading_fragment.inl"
#endif
#undef BYTE
