#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_environment_lighting_skybox_fragment.inl"
#else
#include "release/_internal_environment_lighting_skybox_fragment.inl"
#endif
#undef BYTE
