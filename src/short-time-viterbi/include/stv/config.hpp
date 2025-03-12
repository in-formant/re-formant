#ifndef STVITERBI_CONFIG_HPP
#define STVITERBI_CONFIG_HPP

#include <Eigen/Dense>

namespace stv {
using float_type = float;
using int_type = int;

using ArrayXf = Eigen::Array<float_type, Eigen::Dynamic, 1>;
using ArrayXXf = Eigen::Array<float_type, Eigen::Dynamic, Eigen::Dynamic>;

using ArrayXi = Eigen::Array<int_type, Eigen::Dynamic, 1>;
using ArrayXXi = Eigen::Array<int_type, Eigen::Dynamic, Eigen::Dynamic>;

using Eigen::all;
}

#endif // STVITERBI_CONFIG_HPP