#ifndef REFORMANT_PROCESSING_FORMANTS_H
#define REFORMANT_PROCESSING_FORMANTS_H

#include <array>
#include <vector>

#include "../routines/routines.h"
#include "../vector2d.h"

namespace reformant {

inline constexpr int MAXFORMANTS = 7;

struct Formant {
    int offset;
    double freq;
    double band;
};

struct FormantTrack {
    vector2d<Formant> form;
    int length;
    double sampleRate;
};

class FormantTracking {
   public:
    FormantTracking(int nForm, double nomF1);

    FormantTrack track(const PoleArray& ps);

   private:
    // -- Structs.

    struct CandySt {
        std::vector<double> fre;
        int maxPoles; /* number of poles to consider */
        int maxForms; /* number of formants to find */
        vector2d<int> pc;
    };

    /* structure of a DP lattice node for formant tracking */
    struct FormLattice {
        int ncand;                  /* # of candidate mappings for this frame */
        vector2d<int> cand;         /* pole-to-formant map-candidate array */
        std::vector<int> prept;     /* backpointer array for each frame */
        std::vector<double> cumerr; /* cum. errors associated with each cand. */
    };

    // -- Fields.

    int nForm;
    double nomF1;

    std::array<double, MAXFORMANTS> fnom;  /* "nominal" freqs */
    std::array<double, MAXFORMANTS> fmins; /* frequency bounds */
    std::array<double, MAXFORMANTS> fmaxs; /* for 1st 5 formants */

    int ncan;
    bool doMerge;

    vector2d<double> fr;
    vector2d<double> ba;
    vector2d<int> pcan;
    std::vector<FormLattice> fl;

    // -- Methods.

    /* Can this pole be this freq? */
    bool canBe(const CandySt& st, int poleInd, int formInd);

    /* This does the real work of mapping frequencies to formants. */
    void candy(CandySt& st, int cand, int poleInd, int formInd);

    /* Given a set of pole frequencies and allowable formant frequencies for nform
     * formants, calculate all possible mappings of pole frequencies to formants,
     * including, possibly, mappings with missing formants. */
    void getFcand(int nPole, const std::vector<double>& freq, int nForm,
                  vector2d<int>& pcan);

    void setNominalFreqs(double f1);

    /* Find the maximum in the "stationarity" function (stored in rms) */
    double getStatMax(const PoleArray& ps);
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_FORMANTS_H