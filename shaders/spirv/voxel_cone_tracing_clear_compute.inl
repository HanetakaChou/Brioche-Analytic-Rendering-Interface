static constexpr uint32_t const voxel_cone_tracing_clear_compute_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_voxel_cone_tracing_clear_compute.inl"
#else
#include "release/_internal_voxel_cone_tracing_clear_compute.inl"
#endif
};
