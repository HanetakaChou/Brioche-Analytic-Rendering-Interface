#define BYTE uint8_t
static
#ifndef NDEBUG
#include "debug/_internal_post_processing_fragment.inl"
#else
#include "release/_internal_post_processing_fragment.inl"
#endif
#undef BYTE
