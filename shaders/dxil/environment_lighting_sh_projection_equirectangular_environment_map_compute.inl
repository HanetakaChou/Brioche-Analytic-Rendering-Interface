#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
#else
#include "release/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
#endif
#undef BYTE
