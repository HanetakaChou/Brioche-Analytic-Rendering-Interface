static constexpr uint32_t const post_processing_vertex_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_post_processing_vertex.inl"
#else
#include "release/_internal_post_processing_vertex.inl"
#endif
};
