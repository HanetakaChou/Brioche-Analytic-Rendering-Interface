static constexpr uint32_t const voxel_cone_tracing_pack_compute_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_voxel_cone_tracing_pack_compute.inl"
#else
#include "release/_internal_voxel_cone_tracing_pack_compute.inl"
#endif
};
