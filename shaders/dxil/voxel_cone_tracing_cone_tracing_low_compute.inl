#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl"
#else
#include "release/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl"
#endif
#undef BYTE
