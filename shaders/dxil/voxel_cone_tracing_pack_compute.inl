#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_voxel_cone_tracing_pack_compute.inl"
#else
#include "release/_internal_voxel_cone_tracing_pack_compute.inl"
#endif
#undef BYTE
