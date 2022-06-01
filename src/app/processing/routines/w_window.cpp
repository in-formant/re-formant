#include <iostream>

#include "routines.h"

void w_window(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
              double preEmphasis, WindowType type) {
    switch (type) {
        case WINDOW_RECTANGULAR:
            rwindow(in, ioff, out, n, preEmphasis);
            return;
        case WINDOW_HAMMING:
            hwindow(in, ioff, out, n, preEmphasis);
            return;
        case WINDOW_COS4:
            cwindow(in, ioff, out, n, preEmphasis);
            return;
        case WINDOW_HANN:
            hnwindow(in, ioff, out, n, preEmphasis);
            return;
        default:
            std::cerr << "Unknown window type (" << int(type)
                      << ") requested in w_window()" << std::endl;
    }
}
