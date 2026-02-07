#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_forward_shading_physically_based_rendering_vertex.inl"
#else
#include "release/_internal_forward_shading_physically_based_rendering_vertex.inl"
#endif
#undef BYTE
