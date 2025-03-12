#ifndef FTRACK_COMMON_HPP
#define FTRACK_COMMON_HPP

#include <Eigen/Dense>
#include <complex>

namespace ftrack {
using int_type = int;
using float_type = float;
using complex_type = std::complex<float_type>;

using ArrayXi = Eigen::Array<int_type, Eigen::Dynamic, 1>;
using ArrayXXi = Eigen::Array<int_type, Eigen::Dynamic, Eigen::Dynamic>;

using ArrayXf = Eigen::Array<float_type, Eigen::Dynamic, 1>;
using ArrayXXf = Eigen::Array<float_type, Eigen::Dynamic, Eigen::Dynamic>;

using ArrayXcf = Eigen::Array<complex_type, Eigen::Dynamic, 1>;
using ArrayXXcf = Eigen::Array<complex_type, Eigen::Dynamic, Eigen::Dynamic>;

using VectorXf = Eigen::Vector<float_type, Eigen::Dynamic>;
using MatrixXf = Eigen::Matrix<float_type, Eigen::Dynamic, Eigen::Dynamic>;

using Eigen::all;
using Eigen::last;
using Eigen::seq;
}  // namespace ftrack

#endif  // FTRACK_COMMON_HPP
