
#include <iostream>

#include "routines.h"

PoleArray lpc_poles(const std::vector<double>& data, double sampleRate,
                    double windowDuration, double frameInterval, int lpcOrder,
                    double preEmphasis, LpcType lpcType, WindowType windowType) {
    /* Force "standard" stabilized covariance (a la bsa) */
    if (lpcType == LPC_BSA) {
        windowDuration = 0.025;
        preEmphasis = exp(-62.831853 * 90. / sampleRate);
    }
    if (lpcOrder > MAXORDER || lpcOrder < 2) {
        return {};
    }
    windowDuration = integerize(windowDuration, sampleRate);
    frameInterval = integerize(frameInterval, sampleRate);

    const int size = (int)(.5 + (windowDuration * sampleRate));
    const int step = (int)(.5 + (frameInterval * sampleRate));

    const int length = data.size();
    const int numFrames =
        1 + (int)((length / sampleRate - windowDuration) / frameInterval);

    if (numFrames >= 1) {
        const double lpcStabl = 70.0;
        double energy;
        std::vector<double> lpca(lpcOrder);

        std::vector<Pole> pole(numFrames);
        std::vector<int> data(length);
        for (int i = 0; i < length; ++i) {
            data[i] = (int)data[i];
        }
        bool init = true;
        int dataOff = 0;
        for (int j = 0; j < numFrames; ++j, dataOff += step) {
            pole[j].offset = dataOff;
            pole[j].freq.resize(lpcOrder);
            pole[j].band.resize(lpcOrder);

            switch (lpcType) {
                case LPC_AUTOC:
                    if (!lpc(lpcOrder, lpcStabl, size, data, dataOff, lpca, &energy,
                             preEmphasis, windowType)) {
                        std::cerr << "Problems with lpc() in LpcPoles" << std::endl;
                    }
                    break;
                case LPC_BSA:
                    if (!lpcbsa(lpcOrder, lpcStabl, size, data, dataOff, lpca, &energy,
                                preEmphasis)) {
                        std::cerr << "Problems with lpcbsa() in LpcPoles" << std::endl;
                    }
                    break;
                case LPC_COVAR: {
                    int order = lpcOrder;
                    double alpha, r0;

                    /// TODO: check if wtype=0 or forward wtype
                    w_covar(data, dataOff, &order, size, 0, lpca, &alpha, &r0,
                            preEmphasis, WINDOW_RECTANGULAR);
                    if (order != lpcOrder || alpha <= 0.0) {
                        std::cerr
                            << "Problems with w_covar() in LpcPoles; alpha:" << alpha
                            << "  order:" << order << std::endl;
                    }
                    energy = sqrt(r0 / (size - order));
                    break;
                }
            }
            pole[j].change = 0.0;
            pole[j].rms = energy;
            /* don't waste time on low energy frames */
            if (energy > 1.0) {
                int numForm;
                formant(lpcOrder, sampleRate, lpca, &numForm, pole[j].freq, pole[j].band,
                        init);
                pole[j].npoles = numForm;
                init = false; /* use old poles to start next search */
            } else {
                /* write out no pole frequencies */
                pole[j].npoles = 0;
                init = true; /* restart root search in a neutral zone */
            }
        } /* end LPC pole computation for all frames */
        return {pole, (int)pole.size(), 1. / frameInterval};
    } else {
        std::cerr << "Bad buffer in lpc_poles()" << std::endl;
        return {};
    }
}