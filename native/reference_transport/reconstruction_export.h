#pragma once
/* Only the explicit C ABI is public from the subsystem's shared framework. */
#if defined(__clang__) || defined(__GNUC__)
#define RECONSTRUCTION_EXPORT __attribute__((visibility("default")))
#else
#error "RECONSTRUCTION source preparation currently defines the reviewed Apple/Clang ABI only"
#endif
