#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_post_processing_vertex.inl"
#else
#include "release/_internal_post_processing_vertex.inl"
#endif
#undef BYTE
