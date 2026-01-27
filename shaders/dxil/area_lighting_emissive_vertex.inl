#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_area_lighting_emissive_vertex.inl"
#else
#include "release/_internal_area_lighting_emissive_vertex.inl"
#endif
#undef BYTE
