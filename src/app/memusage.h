#ifndef REFORMANT_MEMUSAGE_H
#define REFORMANT_MEMUSAGE_H

#include <cstddef>
#include <cstdint>

namespace reformant {
uint64_t memoryUsage();
}  // namespace reformant

inline uint64_t operator"" _u64(unsigned long long int x) { return x; }
inline size_t operator"" _sz(unsigned long long int x) { return x; }

#endif  // REFORMANT_MEMUSAGE_H