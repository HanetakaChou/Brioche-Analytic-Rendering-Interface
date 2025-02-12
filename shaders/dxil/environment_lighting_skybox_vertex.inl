#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_environment_lighting_skybox_vertex.inl"
#else
#include "release/_internal_environment_lighting_skybox_vertex.inl"
#endif
#undef BYTE
