#include "ftrack/dsp.hpp"

namespace ftrack {
namespace dsp {
void preemphasis(float_type* x, const int_type len,
                 const float_type preemp, const float_type prev_x) {
    x[0] = x[0] - preemp * prev_x;
    for (int_type i = 1; i < len; ++i) {
        x[i] = x[i] - preemp * x[i - 1];
    }
}
}
}