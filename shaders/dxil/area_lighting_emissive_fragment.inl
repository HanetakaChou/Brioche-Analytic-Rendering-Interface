#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_area_lighting_emissive_fragment.inl"
#else
#include "release/_internal_area_lighting_emissive_fragment.inl"
#endif
#undef BYTE
