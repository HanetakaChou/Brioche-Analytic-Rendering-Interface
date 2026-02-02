#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_forward_shading_physically_based_rendering_fragment.inl"
#else
#include "release/_internal_forward_shading_physically_based_rendering_fragment.inl"
#endif
#undef BYTE
