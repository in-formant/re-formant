#ifndef REFORMANT_PROCESSING_UTIL_UTIL_H
#define REFORMANT_PROCESSING_UTIL_UTIL_H

#include <vector>

namespace reformant {
namespace util {

template <typename T>
std::vector<T> diff(const std::vector<T>& in) {
    if (in.empty()) return {};

    std::vector<T> out(in.size() - 1);
    for (int i = 1; i < in.size(); ++i) {
        out[i - 1] = in[i] - in[i - 1];
    }
    return out;
}

template <typename T>
std::vector<T> product(const std::vector<T>& a, const std::vector<T>& b) {
    std::vector<T> out(a.size());
    for (int i = 0; i < a.size(); ++i) {
        out[i] = a[i] * b[i];
    }
    return out;
}

template <typename T>
std::vector<int> findIndicesLessThan(const std::vector<T>& in,
                                     const double threshold) {
    std::vector<int> indices;
    for (int i = 0; i < in.size(); ++i) {
        if (in[i] < threshold) indices.push_back(i + 1);
    }
    return indices;
}

template <typename T>
std::vector<T> selectElements(const std::vector<T>& in,
                              const std::vector<int>& indices) {
    std::vector<T> out(indices.size());
    for (int i = 0; i < indices.size(); ++i) {
        out[i] = in[indices[i]];
    }
    return out;
}

template <typename T, typename S>
std::vector<int> signVector(const std::vector<T>& in, const S sign) {
    std::vector<int> out(in.size());
    for (int i = 0; i < in.size(); ++i) {
        if (T(sign) * in[i] > T(0)) {
            out[i] = 1;
        } else if (T(sign) * in[i] < T(0)) {
            out[i] = -1;
        } else {
            out[i] = 0;
        }
    }
    return out;
}

template <typename T>
std::pair<T, T> parabolicInterpolation(const std::vector<T>& array,
                                       const int x) {
    int x_adjusted;
    double x_ = x;

    if (x < 1) {
        x_adjusted = (array[x] <= array[x + 1]) ? x : x + 1;
    } else if (x > int(array.size()) - 1) {
        x_adjusted = (array[x] <= array[x - 1]) ? x : x - 1;
    } else {
        T den = array[x + 1] + array[x - 1] - 2 * array[x];
        T delta = array[x - 1] - array[x + 1];
        return (den == 0)
                   ? std::make_pair(x_, array[x])
                   : std::make_pair(x_ + delta / (2 * den),
                                    array[x] - delta * delta / (8 * den));
    }
    return std::make_pair(x_adjusted, array[x_adjusted]);
}

template <typename T>
std::vector<int> findPeaks(const std::vector<T>& x0, const int sign = +1) {
    std::vector<int> peakInds;

    const int minIdx =
        (int)std::distance(x0.begin(), std::min_element(x0.begin(), x0.end()));
    const int maxIdx =
        (int)std::distance(x0.begin(), std::max_element(x0.begin(), x0.end()));

    const T sel = (x0[maxIdx] - x0[minIdx]) / 4;

    const int len0 = (int)x0.size();

    std::vector<T> dx = diff(x0);
    std::replace(dx.begin(), dx.end(), 0.0, -2.2204e-16);
    std::vector<T> dx0(dx.begin(), dx.end() - 1);
    std::vector<T> dx1(dx.begin() + 1, dx.end());
    std::vector<T> dx2 = product(dx0, dx1);

    std::vector<int> ind =
        findIndicesLessThan(dx2, 0);  // Find where the derivative changes sign

    const std::vector<int> indAux(ind);

    std::vector<T> x = selectElements(x0, indAux);
    x.insert(x.begin(), x0[0]);
    x.insert(x.end(), x0[x0.size() - 1]);

    ind.insert(ind.begin(), 0);
    ind.insert(ind.end(), len0);

    int minMagIdx = (int)distance(x.begin(), min_element(x.begin(), x.end()));
    T minMag = x[minMagIdx];
    T leftMin = minMag;
    int len = (int)x.size();

    if (len > 2) {
        double tempMag = minMag;
        bool foundPeak = false;
        int ii;

        // Deal with first point a little differently since tacked it on
        // Calculate the sign of the derivative since we tacked the first
        //  point on it does not neccessarily alternate like the rest.
        std::vector<double> xSub0(x.begin(), x.begin() + 3);
        std::vector<double> xDiff = diff(xSub0);

        std::vector<int> signDx = signVector(xDiff, sign);

        if (signDx[0] <= 0)  // The first point is larger or equal to the second
        {
            if (signDx[0] == signDx[1])  // Want alternating signs
            {
                x.erase(x.begin() + 1);
                ind.erase(ind.begin() + 1);
                len = len - 1;
            }
        } else  // First point is smaller than the second
        {
            if (signDx[0] == signDx[1])  // Want alternating signs
            {
                x.erase(x.begin());
                ind.erase(ind.begin());
                len = len - 1;
            }
        }

        if (x[0] >= x[1])
            ii = 0;
        else
            ii = 1;

        int maxPeaks = (int)ceil((double)len / 2.0);
        std::vector<int> peakLoc(maxPeaks, 0);
        std::vector<double> peakMag(maxPeaks, 0.0);
        int cInd = 1;
        int tempLoc = 0;

        while (ii < len) {
            ii = ii + 1;  // This is a peak
            // Reset peak finding if we had a peak and the next peak is bigger
            // than the last or the left min was small enough to reset.
            if (foundPeak) {
                tempMag = minMag;
                foundPeak = false;
            }

            // Found new peak that was lager than temp mag and selectivity
            // larger than the minimum to its left.

            if (x[ii - 1] > tempMag && x[ii - 1] > leftMin + sel) {
                tempLoc = ii - 1;
                tempMag = x[ii - 1];
            }

            // Make sure we don't iterate past the length of our rpm::vector
            if (ii == len)
                break;  // We assign the last point differently out of the loop

            ii = ii + 1;  // Move onto the valley

            // Come down at least sel from peak
            if (!foundPeak && tempMag > sel + x[ii - 1]) {
                foundPeak = true;  // We have found a peak
                leftMin = x[ii - 1];
                peakLoc[cInd - 1] = tempLoc;  // Add peak to index
                peakMag[cInd - 1] = tempMag;
                cInd = cInd + 1;
            } else if (x[ii - 1] < leftMin)  // New left minima
                leftMin = x[ii - 1];
        }

        // Check end point
        if (x[x.size() - 1] > tempMag && x[x.size() - 1] > leftMin + sel) {
            peakLoc[cInd - 1] = len - 1;
            peakMag[cInd - 1] = x[x.size() - 1];
            cInd = cInd + 1;
        } else if (!foundPeak &&
                   tempMag >
                       minMag)  // Check if we still need to add the last point
        {
            peakLoc[cInd - 1] = tempLoc;
            peakMag[cInd - 1] = tempMag;
            cInd = cInd + 1;
        }

        // Create output
        if (cInd > 0) {
            std::vector<int> peakLocTmp(peakLoc.begin(),
                                        peakLoc.begin() + (cInd - 1));
            peakInds = selectElements(ind, peakLocTmp);
            // peakMags = rpm::vector<double>(peakLoc.begin(),
            // peakLoc.begin()+cInd-1);
        }
    }

    return peakInds;
}

}  // namespace util
}  // namespace reformant

#endif  // REFORMANT_PROCESSING_UTIL_UTIL_H