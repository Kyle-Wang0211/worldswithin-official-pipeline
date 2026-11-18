#pragma once
#include <cstddef>
namespace pw34 {
#ifdef PW34_TEST_INIT_FAULTS
void CpuFaultCheckpoint(const char*,size_t);
inline void InitCheckpoint(const char*s,size_t i){CpuFaultCheckpoint(s,i);}
#else
inline void InitCheckpoint(const char*,size_t){}
#endif
}
