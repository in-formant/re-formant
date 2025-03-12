#include "ftrack/dsp.hpp"

namespace ftrack {
namespace dsp {
float_type median(std::vector<float_type>& v) {
    if (v.empty()) {
        return 0.0;
    }
    auto n = v.size() / 2;
    std::ranges::nth_element(v, v.begin() + n);
    auto med = v[n];
    if (!(v.size() & 1)) {
        //If the set size is even
        auto max_it = std::max_element(v.begin(), v.begin() + n);
        med = (*max_it + med) / 2.0;
    }
    return med;
}
}
}