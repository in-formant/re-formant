#ifndef STVITERBI_VITERBI_HPP
#define STVITERBI_VITERBI_HPP

#include "AlignedAlloc.hpp"
#include "config.hpp"

namespace stv {
class Viterbi {
   public:
    /*
    pi: (K,)
        Initial state probabilities: Pi[i] is the probability x[0] == i.
        If not given uniform initial distribution is assumed (Pi[:] == 1/K)
    A : array (K, K)
        State transition matrix.
    B : array (K, |O|)
        Emission matrix.
    */
    template <typename DerivedPi, typename DerivedA, typename DerivedB>
    Viterbi(const Eigen::EigenBase<DerivedPi>& Pi, const Eigen::EigenBase<DerivedA>& A,
            const Eigen::EigenBase<DerivedB>& B) {
        Pi_ = Pi;
        A_ = A;
        K_ = A.rows();
        B_ = B;

        statesWithNonzeroTransition_.resize(K_);
        for (int_type s = 0; s < K_; ++s) {
            statesWithNonzeroTransition_[s].reserve(K_);
            for (int_type r = 0; r < K_; ++r) {
                if (A_(r, s) > 0) {
                    statesWithNonzeroTransition_[s].push_back(r);
                }
            }
            statesWithNonzeroTransition_[s].shrink_to_fit();
        }
    }

    template <typename DerivedA, typename DerivedB>
    Viterbi(const Eigen::EigenBase<DerivedA>& A, const Eigen::EigenBase<DerivedB>& B)
        : Viterbi(decltype(Pi_)::Ones(A.rows()) / A.rows(), A, B) {}

    /*
    x : array (T,)
        Maximum a posteriori probability estimate of hidden state trajectory,
        conditioned on observation sequence y under the model parameters A, B,
        Pi. In other words, the optimal Viterbi path.
    T1: array (K, T)
        the probability of the most likely path so far (DP table)
    T2: array (K, T)
        the x_j-1 of the most likely path so far (DP table)
    */
    struct viterbi_return_t {
        Eigen::Map<ArrayXi> x;
        Eigen::Map<ArrayXXf> T1;
        Eigen::Map<ArrayXXi> T2;
    };

    /*
    y : array (T,)
    Observation state sequence. int dtype.
    */
    template <typename DerivedY>
    viterbi_return_t viterbi(const Eigen::DenseBase<DerivedY>& y) {
        const int_type T = y.size();

        memo_x_.resize(T, 0);
        memo_T1_.resize(K_ * T, 0.0);
        memo_T2_.resize(K_ * T, 0);

        Eigen::Map<ArrayXi> x(memo_x_.data(), T);
        Eigen::Map<ArrayXXf> T1(memo_T1_.data(), K_, T);
        Eigen::Map<ArrayXXi> T2(memo_T2_.data(), K_, T);

        // Initialize the tracking tables from first observation
        T1(all, 0) = log(Pi_) + log(B_(all, y(0)));
        T2(all, 0) = 0;

        // Iterate through the observations, updating the tracking tables
        for (int_type j = 1; j < T; ++j) {
            for (int_type i = 0; i < K_; ++i) {
                const auto& indices = statesWithNonzeroTransition_[i];
                int_type maxSub;
                T1(i, j) = (T1(indices, j - 1) + log(A_(indices, i)) + log(B_(i, y(j))))
                               .maxCoeff(&maxSub);
                T2(i, j) = indices[maxSub];
            }
        }

        // Build the output, optimal model trajectory by backtracking
        T1(all, T - 1).maxCoeff(&x(T - 1));

        for (int_type i = T - 1; i >= 1; --i) {
            x(i - 1) = T2(x(i), i);
        }

        return {.x = x, .T1 = T1, .T2 = T2};
    }

   private:
    Eigen::ArrayXf Pi_;
    Eigen::ArrayXXf A_;
    int_type K_;
    Eigen::ArrayXXf B_;

    std::vector<std::vector<int_type, AlignedAllocator<int_type>>>
        statesWithNonzeroTransition_;

    /* to avoid re-allocating constantly. */
    std::vector<int_type, AlignedAllocator<int_type>> memo_x_;
    std::vector<float_type, AlignedAllocator<float_type>> memo_T1_;
    std::vector<int_type, AlignedAllocator<int_type>> memo_T2_;
};
}  // namespace stv

#endif  // STVITERBI_VITERBI_HPP
