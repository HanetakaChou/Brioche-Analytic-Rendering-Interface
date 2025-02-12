static constexpr uint32_t const forward_shading_fragment_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_forward_shading_fragment.inl"
#else
#include "release/_internal_forward_shading_fragment.inl"
#endif
};
