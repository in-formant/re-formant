#ifndef STVITERBI_SHORT_TERM_VITERBI_HPP
#define STVITERBI_SHORT_TERM_VITERBI_HPP

#include "config.hpp"
#include "AlignedAlloc.hpp"
#include "Viterbi.hpp"

namespace stv {
class ShortTermViterbi {
public:
    /*
    pi: (K,)
    Initial state probabilities: Pi[i] is the probability x[0] == i.
    If not given uniform initial distribution is assumed (Pi[:] == 1/K)
    A : array (K, K)
    State transition matrix.
    B : array (K, |O|)
    Emission matrix.
    C : int, optional
    Latency constraint. Make a suboptimal decision if a fusion point was not
    encountered in more than C number of observations.
    */
    template <typename DerivedPi, typename DerivedA, typename DerivedB>
    ShortTermViterbi(const Eigen::EigenBase<DerivedPi>& Pi,
                     const Eigen::EigenBase<DerivedA>& A,
                     const Eigen::EigenBase<DerivedB>& B,
                     int_type C = -1) : local_viterbi_(Pi, A, B) {
        Pi_ = Pi;
        A_ = A;
        K_ = A.rows();
        B_ = B;
        C_ = C;
        a_ = 0;
        b_ = 0;
    }

    /*
    returns true if a fusion point was found or the max subsearch length was reached.
    you can retrieve the finalised optimal path with known_good_path().
    */
    bool new_observation(const int_type last_y) {
        y_.push_back(last_y);
        b_ = y_.size() - 1;

        // When a new observation is made, recalculate the Viterbi paths from a to b.
        const int_type T = b_ - a_ + 1;

        const auto& [vit_x, vit_T1, vit_T2] = local_viterbi_.viterbi(
            Eigen::Map<Eigen::ArrayXi>(y_.data() + a_, T));

        // Find if there is a fusion point T (all local paths to T are identical)
        auto best_paths_for_each_state = std::vector(T, std::vector<int_type>(K_));

        // Backtrack for every possible end state.
        for (int_type k = 0; k < K_; ++k) {
            best_paths_for_each_state[T - 1][k] = k;
            for (int_type j = T - 1; j >= 1; --j) {
                best_paths_for_each_state[j - 1][k] = vit_T2(
                    best_paths_for_each_state[j][k], j);
            }
        }

        // Find the latest observation that has identical local paths.
        int_type potential_a = a_;

        for (int_type j = 0; j < T; ++j) {
            // All the indices for this step have to be equal.
            int_type first_state_index = best_paths_for_each_state[j][0];
            bool all_equal = true;
            for (int_type i = 1; i < K_; ++i) {
                if (best_paths_for_each_state[j][i] != first_state_index) {
                    all_equal = false;
                    break;
                }
            }
            if (all_equal) {
                potential_a = a_ + j;
            } else {
                break;
            }
        }

        // If *a* changed at all, we can update.
        if (potential_a > a_) {
            const int_type how_many_final = potential_a - a_ - 1;
            finalpath_.resize(how_many_final);
            std::copy_n(vit_x.begin(), how_many_final, finalpath_.begin());

            int_type prev_a = a_;
            a_ = potential_a;

            //std::cout << "[C] new fusion point found after T=" << how_many_final << std::endl;
            //std::cout << "   [" << prev_a << ", " << prev_a + how_many_final << "]" << std::endl;
            return true;
        }

        // If we have a latency constraint, force a decision.
        if (C_ != -1 && T > C_) {
            finalpath_.resize(T - 1);
            std::copy_n(vit_x.begin(), T - 1, finalpath_.begin());

            int_type prev_a = a_;
            a_ = a_ + T - 1;

            //std::cout << "[C] latency constraint reached (C=" << C_ << ")" << std::endl;
            //std::cout << "   [" << prev_a << ", " << prev_a + T - 1 << "]" << std::endl;
            return true;
        }

        path_.resize(T);
        std::copy_n(vit_x.begin(), T, path_.begin());
        return false;
    }

    [[nodiscard]] const std::vector<int_type>& known_good_path() const {
        return finalpath_;
    }

    [[nodiscard]] const std::vector<int_type>& current_best() const {
        return path_;
    }

private:
    Eigen::ArrayXf Pi_;
    Eigen::ArrayXXf A_;
    int_type K_;
    Eigen::ArrayXXf B_;
    int_type C_;

    // SIMD alignment allocator to align for Eigen.
    std::vector<int_type, AlignedAllocator<int_type> > y_;
    int_type a_; /* Start point */
    int_type b_; /* End point (this is just y.length - 1) */

    Viterbi local_viterbi_;

    std::vector<int_type> finalpath_;
    std::vector<int_type> path_;
};
} // namespace stv

#endif // STVITERBI_SHORT_TERM_VITERBI_HPP

