/*
 * This software has been licensed to the Centre of Speech Technology, KTH
 * by AT&T Corp. and Microsoft Corp. with the terms in the accompanying
 * file BSD.txt, which is a BSD style license.
 *
 *    "Copyright (c) 1987-1990  AT&T, Inc.
 *    "Copyright (c) 1986-1990  Entropic Speech, Inc.
 *    "Copyright (c) 1990-1994  Entropic Research Laboratory, Inc.
 *                   All rights reserved"
 *
 * Written by:  David Talkin
 * Revised by: John Shore
 *
 * Refactored for ReFormant by Clo Yun-Hee Dufour
 */

#include "formants.h"

using namespace reformant;

static constexpr bool debug = false;
static constexpr bool w_verbose = false;

/* a formant tracker based on LPC polynomial roots and dynamic programming */
/***/
/* At each frame, the LPC poles are ordered by increasing frequency.  All
   "reasonable" mappings of the poles to F1, F2, ... are performed.
   The cost of "connecting" each of these mappings with each of the mappings
   in the previous frame is computed.  The lowest cost connection is then
   chosen as the optimum one.  At each frame, each mapping has associated
   with it a cost based on the formant bandwidths and frequencies.  This
   "local" cost is finally added to the cost of the best "connection."  At
   end of utterance (or after a reasonable delay like .5sec) the best
   mappings for the entire utterance may be found by retracing back through
   best candidate mappings, starting at end of utterance (or current frame).
*/

/* Here are the major fudge factors for tweaking the formant tracker. */

/* maximum number of candidate mappings allowed */
static constexpr int MAXCAN = 600;

/* equivalent delta-Hz cost for missing formant */
static constexpr double MISSING = 1;

/* equivalent bandwidth cost of a missing formant */
static constexpr double NOBAND = 1000;

/* cost for proportional frequency changes */
static constexpr double DF_FACT = 20.0;
/* with good "stationarity" function:*/
/*        DF_FACT =  80.0, */

/* cost for proportional dev. from nominal freqs. */
static constexpr double DFN_FACT = 0.3;

/* cost per Hz of bandwidth in the poles */
static constexpr double BAND_FACT = .002;

/*  bias toward selecting low-freq. poles */
/*	F_BIAS	  = 0.0004,   bias toward selecting low-freq. poles */
static constexpr double F_BIAS = 0.000;

/* cost of mapping f1 and f2 to same frequency */
static constexpr double F_MERGE = 2000.0;

// -- Algorithm

FormantTracking::FormantTracking(int nForm, double nomF1)
    : nForm(nForm),
      nomF1(nomF1),
      fnom({500, 1500, 2500, 3500, 4500, 5500, 6500}),
      fmins({50, 400, 1000, 2000, 2000, 3000, 3000}),
      fmaxs({1500, 3500, 4500, 5000, 6000, 6000, 8000}),
      doMerge(true) {}

bool FormantTracking::canBe(const CandySt& st, int poleInd, int formInd) {
    return (st.fre[poleInd] >= fmins[formInd] && st.fre[poleInd] <= fmaxs[formInd]);
}

void FormantTracking::candy(CandySt& st, int cand, int poleInd, int formInd) {
    if (formInd < st.maxForms) st.pc(cand, formInd) = -1;
    if (poleInd < st.maxPoles && formInd < st.maxForms) {
        if (canBe(st, poleInd, formInd)) {
            st.pc(cand, formInd) = poleInd;
            if (doMerge && formInd == 0 && canBe(st, poleInd, formInd + 1)) {
                /* allow for f1,f2 merger */
                ++ncan;
                st.pc(ncan, 0) = st.pc(cand, 0);
                candy(st, ncan, poleInd, formInd + 1); /* same pole, next formant */
            }
            candy(st, cand, poleInd + 1, formInd + 1); /* next pole, next formant */
            if (poleInd + 1 < st.maxPoles && canBe(st, poleInd + 1, formInd)) {
                /* try other frequencies for this formant */
                ++ncan; /* add one to the candidate index/tally */
                for (int i = 0; i < formInd; ++i) { /* clone the lower formants */
                    st.pc(ncan, i) = st.pc(cand, i);
                    candy(st, ncan, poleInd + 1, formInd);
                }
            } else {
                candy(st, cand, poleInd + 1, formInd);
            }
        }
    }
    /* If all pole frequencies have been examined without finding one which will map onto
     * the current formant, go on to the next formant leaving the current formant null. */
    if (poleInd >= st.maxPoles && formInd < st.maxForms - 1 && st.pc(cand, formInd) < 0) {
        int i = 0;
        if (formInd > 0) {
            int j = formInd - 1;
            while (j > 0 && st.pc(cand, j) < 0) --j;
            j = st.pc(cand, j);
            i = (j >= 0) ? j : 0;
        }
        candy(st, cand, i, formInd + 1);
    }
}

void FormantTracking::getFcand(int nPole, const std::vector<double>& freq, int nForm,
                               vector2d<int>& pcan) {
    CandySt st{freq, nPole, nForm, pcan};
    ncan = 0;
    candy(st, ncan, 0, 0);
    pcan = st.pc;
    ++ncan; /* converts ncan as an index to ncan as a candidate count */
}

void FormantTracking::setNominalFreqs(double f1) {
    for (int i = 0; i < MAXFORMANTS; ++i) {
        fnom[i] = (i * 2 + 1) * f1;
        fmins[i] = fnom[i] - ((i + 1) * f1) + 50.0;
        fmaxs[i] = fnom[i] + (i * f1) + 1000.0;
    }
}

double FormantTracking::getStatMax(const PoleArray& ps) {
    double amax = std::numeric_limits<double>::lowest();

    for (int i = 0; i < ps.pole.size(); ++i) {
        const double t = ps.pole[i].rms;
        if (t > amax) amax = t;
    }

    return amax;
}

FormantTrack FormantTracking::track(const PoleArray& ps) {
    if (nomF1 > 0.) {
        setNominalFreqs(nomF1);
    }

    const double rmsmax = getStatMax(ps);

    const double FBIAS = F_BIAS / (.01 * ps.sampleRate);
    /* Setup working values of the cost weights */
    double dffact = (DF_FACT * .01) * ps.sampleRate;
    double bfact = BAND_FACT / (.01 * ps.sampleRate);
    double ffact = DFN_FACT / (.01 * ps.sampleRate);
    double mergeCost = F_MERGE;
    if (mergeCost > 1000.) doMerge = false;

    /* Allocate space for the formant and bandwidth arrays to be passed back */
    fr.resize(nForm, ps.length);
    ba.resize(nForm, ps.length);

    /* Allocate space for the raw candidate array */
    pcan.resize(MAXCAN, nForm);

    /* Allocate space for the DP lattice */
    fl.resize(ps.length);

    for (int i = 0; i < ps.length; ++i) { /* for all analysis frames... */
        ncan = 0;                         /* initialize candidate mapping count to 0 */

        /* moderate the cost of frequency jumps by the relative amplitude */
        double rmsdffact = ps.pole[i].rms;
        rmsdffact = rmsdffact / rmsmax;
        rmsdffact = rmsdffact * dffact;

        const auto& polei = ps.pole[i];

        /* Get all likely mappings of the poles onto formants for this frame. */
        if (polei.npoles > 0) { /* if there ARE pole frequencies available... */
            getFcand(polei.npoles, polei.freq, nForm, pcan);

            /* Allocate space for this frame's candidates in the dp lattice */
            fl[i].prept.resize(ncan);
            fl[i].cumerr.resize(ncan);
            fl[i].cand.resize(ncan, nForm);
            /* allocate cand. slots and install candidates */
            for (int j = 0; j < ncan; ++j) {
                for (int k = 0; k < nForm; ++k) {
                    fl[i].cand(j, k) = pcan(j, k);
                }
            }
        }
        fl[i].ncand = ncan;

        /* compute the distance between the current and previous mappings */
        for (int j = 0; j < ncan; ++j) { /* for each CURRENT mapping... */
            if (i > 0) {                 /* past the first frame? */
                const auto& poleim = ps.pole[i - 1];

                double minErr = 0.;
                int minCan = -1;
                if (fl[i - 1].ncand > 0) minErr = 2.0e30;

                for (int k = 0; k < fl[i - 1].ncand; ++k) { /* for each PREVIOUS map... */
                    double pfErr = 0.;
                    for (int l = 0; l < nForm; ++l) {
                        int ic = fl[i].cand(j, l);
                        int ip = fl[i - 1].cand(k, l);
                        if (ic >= 0 && ip >= 0) {
                            const double ftemp = 2. *
                                                 fabs(polei.freq[ic] - poleim.freq[ip]) /
                                                 (polei.freq[ic] + poleim.freq[ip]);
                            /* cost prop. to sq of deviation to discourage large jumps */
                            pfErr += ftemp * ftemp;
                        } else {
                            pfErr += MISSING;
                        }
                    }
                    /* scale delta-frequency cost and add in prev. cum. cost */
                    const double conErr = (rmsdffact * pfErr) + fl[i - 1].cumerr[k];
                    if (conErr < minErr) {
                        minErr = conErr;
                        minCan = k;
                    }
                }

                fl[i].prept[j] = minCan; /* point to best previous mapping */
                /* (Note that mincan=-1 if there were no candidates in prev. fr.) */
                /* Compute the local costs for this current mapping. */
                double berr = 0.;
                double ferr = 0.;
                double fbias = 0.;
                double merger = 0.;
                for (int k = 0; k < nForm; k++) {
                    const int ic = fl[i].cand(j, k);
                    if (ic >= 0) {
                        if (k == 0) { /* F1 candidate? */
                            double ftemp = polei.freq[ic];
                            if (doMerge && ftemp == polei.freq[fl[i].cand(j, 1)]) {
                                merger = mergeCost;
                            }
                        }
                        berr += polei.band[ic];
                        ferr += (fabs(polei.freq[ic] - fnom[k]) / fnom[k]);
                        fbias += polei.freq[ic];
                    } else { /* if there was no freq. for this formant */
                        fbias += fnom[k];
                        berr += NOBAND;
                        ferr += MISSING;
                    }
                }

                /* Compute the total cost of this mapping and best previous. */
                fl[i].cumerr[j] =
                    (FBIAS * fbias) + (bfact * berr) + merger + (ffact * ferr) + minErr;
            }
        } /* end for each CURRENT mapping... */
    }     /* end for all analysis frames... */

    /* Pick the candidate in the final frame with the lowest cost. */
    /* Starting with that min.-cost cand., work back thru the lattice. */
    int dmaxc = 0;
    int dminc = 100;
    int dcountc = 0;
    int dcountf = 0;

    double minErr;
    int minCan = -1;
    for (int i = ps.length - 1; i >= 0; --i) {
        if (minCan < 0) {          /* need to find best starting candidate? */
            if (fl[i].ncand > 0) { /* have candidates at this frame? */
                minErr = fl[i].cumerr[0];
                minCan = 0;
                for (int j = 1; j < fl[i].ncand; ++j) {
                    if (fl[i].cumerr[j] < minErr) {
                        minErr = fl[i].cumerr[j];
                        minCan = j;
                    }
                }
            }
        }
        if (minCan >= 0) { /* if there is a best "best" candidate at this frame */
            const auto& polei = ps.pole[i];

            int j = fl[i].ncand;
            if (j > dmaxc)
                dmaxc = j;
            else if (j < dminc)
                dminc = j;
            dcountc += j;
            dcountf++;

            for (j = 0; j < nForm; ++j) {
                const int k = fl[i].cand(minCan, j);
                if (k >= 0) {
                    fr(j, i) = polei.freq[k];
                    ba(j, i) = polei.band[k];
                } else { /* IF FORMANT IS MISSING... */
                    if (i < ps.length - 1) {
                        fr(j, i) = fr(j, i + 1); /* replicate backwards */
                        ba(j, i) = ba(j, i + 1);
                    } else {
                        fr(j, i) = fnom[j]; /* or insert neutral values */
                        ba(j, i) = NOBAND;
                    }
                }
            }
            minCan = fl[i].prept[minCan];
        } else { /* if no candidates, fake with "nominal" frequencies. */
            for (int j = 0; j < nForm; ++j) {
                fr(j, i) = fnom[j];
                ba(j, i) = NOBAND;
            }
        } /* note that mincan will remain =-1 if no candidates */
    }     /* end unpacking formant tracks from the dp lattice */

    FormantTrack track;
    track.form.resize(nForm, ps.length);
    track.length = ps.length;
    track.sampleRate = ps.sampleRate;

    for (int i = 0; i < ps.length; ++i) {
        for (int j = 0; j < nForm; ++j) {
            track.form(j, i).offset = ps.pole.at(i).offset;
            track.form(j, i).freq = fr(j, i);
            track.form(j, i).band = ba(j, i);
        }
    }

    return track;
}