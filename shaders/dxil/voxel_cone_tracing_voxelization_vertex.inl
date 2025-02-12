#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_voxel_cone_tracing_voxelization_vertex.inl"
#else
#include "release/_internal_voxel_cone_tracing_voxelization_vertex.inl"
#endif
#undef BYTE
