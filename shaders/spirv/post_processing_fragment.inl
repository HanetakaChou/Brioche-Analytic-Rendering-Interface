static constexpr uint32_t const post_processing_fragment_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_post_processing_fragment.inl"
#else
#include "release/_internal_post_processing_fragment.inl"
#endif
};
