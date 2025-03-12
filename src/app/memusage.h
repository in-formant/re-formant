#ifndef REFORMANT_MEMUSAGE_H
#define REFORMANT_MEMUSAGE_H

#include <cstddef>
#include <cstdint>

namespace reformant {
uint64_t memoryUsage();
}  // namespace reformant

<<<<<<< HEAD
inline uint64_t operator"" _u64(unsigned long long int x) { return x; }
inline size_t operator"" _sz(unsigned long long int x) { return x; }
=======
inline uint64_t operator"" _u64(const uint64_t x) { return x; }
inline size_t operator"" _sz(const size_t x) { return x; }
>>>>>>> ad5d6c670eab97383613c8523ec32898a1ef1cc9

#endif  // REFORMANT_MEMUSAGE_H