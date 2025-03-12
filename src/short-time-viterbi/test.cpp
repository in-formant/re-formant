#include <iostream>
#include <numeric>

#include "stv/Viterbi.hpp"
#include "stv/ShortTermViterbi.hpp"

#include <random>
#include <chrono>

using namespace Eigen;

std::random_device rd;
std::mt19937::result_type seed(0xDEADBEEF);
std::mt19937 rnd(seed);

void pretty_print(const ArrayXi& v) {
    if (v.size() == 0) {
        std::cout << "Path | empty";
    } else {
        std::cout << "Path | " << v(0);
        for (int i = 1; i < v.size(); ++i) {
            std::cout << "," << v(i);
        }
    }
}

int main() {
    int N = 360;
    int T = 200;
    std::normal_distribution<> dist_obs(100, 10.0);

    //starting
    ArrayXf Pi = ArrayXf::Ones(N) / N;

    //transition
    ArrayXXf A = ArrayXXf::Zero(N, N);
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            A(x, y) = std::max(12 - std::abs(x - y), 0);
        }
    }
    A.colwise() /= A.rowwise().sum();

    //emission
    ArrayXXf B = ArrayXXf::Zero(N, N);
    float self_emission = 0.1;
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            B(x, y) = (x == y) * self_emission + (1 - self_emission) / N;
        }
    }

    //random observation vector
    ArrayXi y(T);
    for (int i = 0; i < T; ++i) {
        y(i) = static_cast<int>(std::round(dist_obs(rnd)));
    }

    std::cout << "Starting Vanilla Viterbi..." << std::endl;

    stv::Viterbi vit1(Pi, A, B);

    const auto& [viterbi_path, t1, t2] = vit1.viterbi(y);

    pretty_print(viterbi_path);
    std::cout << std::endl;

    std::cout << "Starting Short Time Viterbi..." << std::endl;

    stv::ShortTermViterbi vit2(Pi, A, B);

    std::vector<int> path;
    for (auto yk : y) {
        if (vit2.new_observation(yk)) {
            std::ranges::copy(vit2.known_good_path(), std::back_inserter(path));
        }
    }

    Map<ArrayXi> xpath(path.data(), path.size());
    pretty_print(xpath);
    std::cout << std::endl;

    return 0;
}