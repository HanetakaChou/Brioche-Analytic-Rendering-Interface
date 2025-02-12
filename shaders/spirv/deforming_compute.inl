static constexpr uint32_t const deforming_compute_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_deforming_compute.inl"
#else
#include "release/_internal_deforming_compute.inl"
#endif
};
