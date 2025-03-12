#include "ftrack/Tracker.hpp"

#include "ftrack/dsp.hpp"

#include <Eigen/LU>
#include <unsupported/Eigen/Polynomials>

namespace ftrack {
Tracker::Tracker(LpType lptype, float_type nwin_millis, float_type nshift_millis,
                 int_type p, int_type q, int_type npeaks, float_type preemp,
                 float_type fint_millis) {
    lptype_ = lptype;
    fs_ = 8000;
    nwin_ = std::round(fs_ * nwin_millis / 1000);
    nshift_ = std::round(fs_ * nshift_millis / 1000);
    p_ = p;
    q_ = q;
    npeaks_ = npeaks;
    preemp_ = preemp;
    fint_ = std::round(fs_ * fint_millis / 1000);

    input_prev_sample_ = 0;
}

int_type Tracker::nwin() const {
    return nwin_;
}

int_type Tracker::nshift() const {
    return nshift_;
}

ArrayXXf Tracker::process(const std::vector<float_type>& signal) {
    const int_type Ns = signal.size();

    frame_.resize(Ns);
    std::ranges::copy(signal, frame_.begin());

    // Pre-emphasis
    dsp::preemphasis(frame_.data(), Ns, preemp_, input_prev_sample_);
    input_prev_sample_ = frame_[Ns - 1];

    // QCP weight function computation if using Weighted Linear Prediction
    ArrayXf w;
    switch (lptype_) {
        case tvwlp_l1:
        case tvwlp_l2:
            w = calculate_qcp_weight_function();
            break;
        case tvlp_l1:
        case tvlp_l2:
            w = ArrayXf::Zero(Ns);
            break;
    }

    Eigen::Map<ArrayXf> s(frame_.data(), frame_.size());

    ArrayXf x;
    ArrayXf wj;

    std::vector<ArrayXXf> Fi_total;

    for (int j = 0; j < Ns - nwin_; j += nshift_) {
        if (j <= Ns - nwin_ - nshift_) {
            x = s(seq(j, j + nwin_));
            wj = w(seq(j, j + nwin_));
        } else {
            x = s(seq(j + 1, last));
            wj = w(seq(j + 1, last));
        }

        const int_type Nx = x.size();

        ArrayXXf aki;
        switch (lptype_) {
            case tvlp_l2:
                aki = solve_tvlp_l2(x.data(), Nx, p_, q_);
                break;
            case tvwlp_l2:
                aki = solve_tvwlp_l2(x.data(), Nx, p_, q_, wj.data());
                break;
            case tvlp_l1:
                // aki = solve_tvlp_l1(x.data(), Nx, p_, q_);
                break;
            case tvwlp_l1:
                // aki = solve_tvwlp_l1(x.data(), Nx, p_, q_, wj.data());
                break;
        }

        const auto [fi, ak] = tvlp_aki_to_fi(aki, Nx, npeaks_, fs_);

        std::vector<ArrayXXf> Fi_arr;

        const int_type index = std::floor((nwin_ - nshift_) / 2);

        if (j == 0) {
            Fi_arr.emplace_back(fi(seq(0, index - 1), all));
        }

        Fi_arr.emplace_back(fi(seq(index, index + nshift_ - 1), all));

        if (j > Ns - nwin_ - nshift_) {
            Fi_arr.emplace_back(fi(seq(index + nshift_, last), all));
        }

        Fi_total.emplace_back(rowwise_stack(Fi_arr));
    }

    ArrayXXf Fi = rowwise_stack(Fi_total);

    Fi_ = Fi(seq(fint_, last, fint_), all);

    return Fi_;
}

ArrayXf Tracker::calculate_qcp_weight_function() {
    // QCP weight function computation
    // GCI detection using SEDREAMS
    float_type wmin = 0.00001; // Minimum value of weighting function
    float_type DQ = 0.7; // Duration Quotient (rel. to fundamental period)
    float_type PQ = 0.05; // Position Quotient (rel. to fundamental period)
    int_type Nramp = 3; // Length of linear ramp (in samples)

    int_type f0min = 60;
    int_type f0max = 600;
    // Pitch tracking using SRH method
    int_type srh_frames = dsp::SRH_numframes(frame_.size(), fs_);
    if (f0_.size() != srh_frames) {
        f0_.resize(srh_frames);
        vuv_.resize(srh_frames);
        srh_.resize(srh_frames);
    }
    dsp::SRH_PitchTracking(frame_.data(), frame_.size(), fs_, f0min, f0max, f0_.data(),
                           vuv_.data(), srh_.data());
    // f0mean
    float_type f0mean = 0.0;
    int_type f0mean_count = 0;
    for (int_type i = 0; i < srh_frames; ++i) {
        if (vuv_[i]) {
            f0mean += f0_[i];
            f0mean_count += 1;
        }
    }
    f0mean /= f0mean_count;

    // SEDREAMS
    std::vector<int_type> gc = dsp::SEDREAMS_GCIDetection(
        frame_.data(), frame_.size(), fs_, f0mean);

    return qcp_wt(frame_.data(), frame_.size(), p_, DQ, PQ, wmin, Nramp,
                  gc.data(), gc.size(), fs_);
}

ArrayXf qcp_wt(const float_type* x, int_type N, int_type p,
               float_type DQ, float_type PQ, float_type d, int_type Nramp,
               int_type* gci, int_type len_gci, int_type fs) {
    // Create a AME weight function for frame x for LPC order p
    //  DQ = duration quotient (from 0 to 1)
    //  PQ = Position Quotient (from 0 to 1)
    //  d = minimum value of the weight function
    //  Nramp = length of the linear ramp (in samples)
    //  gci = Glottal Closure Instants of the frame x

    ArrayXf UPramp(Nramp), DOWNramp;
    if (Nramp > 0) {
        // UPramp = linspace(d,1,2+Nramp)[2:end-1];
        for (int k = 0; k < Nramp; ++k) {
            UPramp(k) = d + (k + 1) * (1 - d) / (Nramp + 1);
        }
        DOWNramp = UPramp.reverse();
    }

    if (DQ + PQ > 1) {
        DQ = 1 - PQ;
    }

    ArrayXf w = ArrayXf::Constant(N + p, d);

    int_type i(0), T1(0), T2(0);

    for (i = 0; i < len_gci - 1; ++i) {
        int_type T = gci[i + 1] - gci[i];
        T1 = static_cast<int_type>(std::round(DQ * T));
        T2 = static_cast<int_type>(std::round(PQ * T));
        while (T1 + T2 > T) {
            T1 -= 1; // fixme: ??????
        }
        w(seq(gci[i] + T2, gci[i] + T2 + T1 - 1)) = 1;
        if (Nramp > 0) {
            w(seq(gci[i] + T2, gci[i] + T2 + Nramp - 1)) = UPramp;
            if (gci[i] + T2 + T1 - Nramp > 0) {
                w(seq(gci[i] + T2 + T1 - Nramp, gci[i] + T2 + T1 - 1)) = DOWNramp;
            }
        }
    }

    int_type Nend = N - (T2 + gci[i + 1]);

    if (T2 + gci[i + 1] < N) {
        if (T1 + T2 < Nend) {
            w(seq(gci[i + 1] + T2, gci[i + 1] + T2 + T1 - 1)) = 1;
            if (Nramp > 0) {
                w(seq(gci[i + 1] + T2, gci[i + 1] + T2 + Nramp - 1)) = UPramp;
                w(seq(gci[i + 1] + T2 + T1 - Nramp, gci[i + 1] + T2 + T1 - 1)) = DOWNramp;
            }
        } else {
            T1 = Nend - T2;
            w(seq(gci[i + 1] + T2, gci[i + 1] + T2 + T1 - 1)) = 1;
            if (Nramp > 0) {
                w(seq(gci[i + 1] + T2, gci[i + 1] + T2 + Nramp - 1)) = UPramp;
            }
        }
    }

    return w;
}

ArrayXXf solve_tvlp_l2(const float_type* x, const int_type N,
                       const int_type p, const int_type q) {
    VectorXf Yn(N - p);
    MatrixXf Ypu(N - p, p * (q + 1));

    int_type m = 0;
    for (int_type n = p; n < N; ++n) {
        for (int_type i = 0; i < p; ++i) {
            for (int_type j = 0; j < q + 1; ++j) {
                Yn(m) = x[n];
                Ypu(m, i * (q + 1) + j) = std::pow(n - p, j) * x[n - i];
            }
        }
    }

    VectorXf x_l2 = Ypu.completeOrthogonalDecomposition().solve(Yn);

    return x_l2.reshaped(q + 1, p);
}

ArrayXXf solve_tvwlp_l2(const float_type* x, const int_type N,
                        const int_type p, const int_type q,
                        const float_type* w) {
    VectorXf Yn(N - p);
    MatrixXf Ypu(N - p, p * (q + 1));

    int_type m = 0;
    for (int_type n = p; n < N; ++n) {
        for (int_type i = 0; i < p; ++i) {
            for (int_type j = 0; j < q + 1; ++j) {
                Yn(m) = w[n] * x[n];
                Ypu(m, i * (q + 1) + j) = w[n] * std::pow(n - p, j) * x[n - i];
            }
        }
    }

    VectorXf x_l2 = Ypu.completeOrthogonalDecomposition().solve(Yn);

    return x_l2.reshaped(q + 1, p);
}

ArrayXXf solve_tvlp_l1(const float_type* x, const int_type N,
                       const int_type p, const int_type q) {
    VectorXf Yn(N - p);
    MatrixXf Ypu(N - p, p * (q + 1));

    int_type m = 0;
    for (int_type n = p; n < N; ++n) {
        for (int_type i = 0; i < p; ++i) {
            for (int_type j = 0; j < q + 1; ++j) {
                Yn(m) = x[n];
                Ypu(m, i * (q + 1) + j) = std::pow(n - p, j) * x[n - i];
            }
        }
    }

    /*
    Linear system:
        ( minimize |Ax-b|_1 s.t. |x|_1 <= 1 )

        ( minimize f'x s.t. Ax = b )

        f  = [   0_n,  1_m, 1_m]
        A  = [     A, -I_m, I_m]
        b  = [     b]
        lb = [-inf_n,  0_m, 0_m]
    */

    //VectorXf x_l2 = Ypu.completeOrthogonalDecomposition().solve(Yn);
    //return x_l2.reshaped(q + 1, p);
}

std::pair<ArrayXXf, ArrayXXf> tvlp_aki_to_fi(const ArrayXXf& aki, const int_type Nx,
                                             const int_type npeaks, const float_type fs) {
    const int_type q = aki.rows() - 1;
    const int_type p = aki.cols();
    const ArrayXf tn = ArrayXf::LinSpaced(Nx, 0, Nx - 1);

    ArrayXXf akn = ArrayXXf::Zero(p, Nx);
    for (int k = 0; k < p; ++k) {
        for (int i = 0; i <= q; ++i) {
            akn(k, all) = akn(k, all) + aki(i, k) * tn.pow(i);
        }
    }

    ArrayXXf ak(p + 1, Nx);
    ak(0, all) = 1;
    ak(seq(1, last), all) = -akn;

    std::vector<float_type> positive_roots;

    ArrayXXf fi(Nx, npeaks);
    for (int i = 0; i < Nx; ++i) {
        Eigen::PolynomialSolver<double, Eigen::Dynamic> root_solver;
        root_solver.compute(ak(all, i).cast<double>());
        ArrayXf fitmp = root_solver.roots().cwiseArg().cast<float_type>() * fs / 2 / M_PI;

        positive_roots.clear();
        std::ranges::copy_if(fitmp, std::back_inserter(positive_roots),
                             [](auto x) { return x > 0; });
        std::ranges::sort(positive_roots);

        if (!positive_roots.empty()) {
            const int_type sublen = std::min(
                npeaks, static_cast<int>(positive_roots.size()));

            for (int_type j = 0; j < sublen; ++j) {
                fi(i, j) = positive_roots[j];
            }
        }
    }

    return std::make_pair(std::move(fi), std::move(ak));
}

ArrayXXf rowwise_stack(const std::vector<ArrayXXf>& arrays) {
    int_type numCols = arrays[0].cols();
    int_type numRows = 0;
    for (const ArrayXXf& arr : arrays) {
        numRows += arr.rows();
    }
    ArrayXXf stackedArray(numRows, numCols);
    int_type rowOffset = 0;
    for (const ArrayXXf& arr : arrays) {
        size_t curRows = arr.rows();
        stackedArray.middleRows(rowOffset, curRows) = arr;
        rowOffset += curRows;
    }
    return stackedArray;
}
} // namespace ftrack