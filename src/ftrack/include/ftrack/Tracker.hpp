#ifndef FTRACK_TRACKER_HPP
#define FTRACK_TRACKER_HPP

#include "config.hpp"
#include "AlignedAlloc.hpp"

namespace ftrack {
enum LpType {
    tvwlp_l1,
    tvwlp_l2,
    tvlp_l1,
    tvlp_l2,
};

class Tracker {
public:
    /*
    lptype - type of TVLP to use [default tvwlp_l2]
    nwin - window or block size for TVLP analysis in milliseconds [default 200ms]
    nshift - window shift for TVLP [default 200ms]
    p - LP or TVLP order [default 8]
    q - polynomial order [default 3]
    npeaks - number of formants to track [default 3]
    preemp - preemphasis factor [default 0.97]
    fint - formant interval in milliseconds [default 10ms]
     */
    explicit Tracker(LpType lptype = tvwlp_l2,
                     float_type nwin_millis = 200, float_type nshift_millis = 200,
                     int_type p = 8, int_type q = 3, int_type npeaks = 3,
                     float_type preemp = 0.97, float_type fint_millis = 10);

    [[nodiscard]] int_type nwin() const;

    [[nodiscard]] int_type nshift() const;

    /*
    process a *chunk* of audio. should contain multiple *frames* worth of audio.
     */
    ArrayXXf process(const std::vector<float_type>& signal);

private:
    LpType lptype_;
    float_type fs_; // = 8 kHz
    int_type nwin_;
    int_type nshift_;
    int_type p_;
    int_type q_;
    int_type npeaks_;
    float_type preemp_;
    int_type fint_;

    float_type input_prev_sample_;
    std::vector<float_type, AlignedAllocator<float_type> > frame_;

    // output
    ArrayXXf Fi_;

    // QCP weight function computation
    std::vector<int_type, AlignedAllocator<int_type> > f0_;
    std::vector<int_type, AlignedAllocator<int_type> > vuv_;
    std::vector<float_type, AlignedAllocator<float_type> > srh_;

    ArrayXf calculate_qcp_weight_function();
};

ArrayXf qcp_wt(const float_type* x, int_type N, int_type p,
               float_type DQ, float_type PQ, float_type d, int_type Nramp,
               int_type* gci_ins, int_type len_gci, int_type fs);

ArrayXXf solve_tvlp_l2(const float_type* x, int_type N, int_type p, int_type q);

ArrayXXf solve_tvwlp_l2(const float_type* x, int_type N, int_type p, int_type q,
                        const float_type* w);

std::pair<ArrayXXf, ArrayXXf> tvlp_aki_to_fi(const ArrayXXf& aki, int_type Nx,
                                             int_type npeaks, float_type fs);

ArrayXXf rowwise_stack(const std::vector<ArrayXXf>& arrays);
} // namespace ftrack

#endif //FTRACK_TRACKER_HPP
