#ifndef REFORMANT_MEMUSAGE_H
#define REFORMANT_MEMUSAGE_H

#include <cstdint>

namespace reformant {
uint64_t memoryUsage();
}  // namespace reformant

inline uint64_t operator"" _u64(const uint64_t x) { return x; }
inline size_t operator"" _sz(const size_t x) { return x; }

#endif  // REFORMANT_MEMUSAGE_H