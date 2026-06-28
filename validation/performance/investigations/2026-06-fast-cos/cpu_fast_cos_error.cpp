#include <cmath>
#include <cstdio>
#include <cstdint>

#include "Misc.h"


struct Stats
    {
    const char* name;
    double minDeg;
    double maxDeg;
    double stepDeg;
    uint64_t count = 0;
    double signedSum = 0.0;
    double absSum = 0.0;
    double sqSum = 0.0;
    double maxAbs = 0.0;
    double maxAbsDeg = 0.0;
    double maxSigned = 0.0;
    double maxSignedDeg = 0.0;
    double minSigned = 0.0;
    double minSignedDeg = 0.0;
    };


static void runStats(Stats& s)
    {
    constexpr double degToRad = 3.14159265358979323846264338327950288 / 180.0;
    bool first = true;
    for (double deg = s.minDeg; deg <= s.maxDeg + 0.5 * s.stepDeg; deg += s.stepDeg)
        {
        const float exact = cosf((float)(deg * degToRad));
        const float approx = tgx::tgx_fast_cos_deg_clamped((float)deg);
        const double err = (double)approx - (double)exact;
        const double abserr = std::fabs(err);

        s.count++;
        s.signedSum += err;
        s.absSum += abserr;
        s.sqSum += err * err;

        if (first || abserr > s.maxAbs)
            {
            s.maxAbs = abserr;
            s.maxAbsDeg = deg;
            }
        if (first || err > s.maxSigned)
            {
            s.maxSigned = err;
            s.maxSignedDeg = deg;
            }
        if (first || err < s.minSigned)
            {
            s.minSigned = err;
            s.minSignedDeg = deg;
            }
        first = false;
        }
    }


static void printStats(const Stats& s)
    {
    const double n = (double)s.count;
    std::printf("%s\n", s.name);
    std::printf("  range_deg        : %.6f .. %.6f, step %.6f, samples %llu\n",
                s.minDeg, s.maxDeg, s.stepDeg, (unsigned long long)s.count);
    std::printf("  mean_signed_err  : %.10g\n", s.signedSum / n);
    std::printf("  mean_abs_err     : %.10g\n", s.absSum / n);
    std::printf("  rms_err          : %.10g\n", std::sqrt(s.sqSum / n));
    std::printf("  max_abs_err      : %.10g at %.6f deg\n", s.maxAbs, s.maxAbsDeg);
    std::printf("  max_signed_err   : %.10g at %.6f deg\n", s.maxSigned, s.maxSignedDeg);
    std::printf("  min_signed_err   : %.10g at %.6f deg\n", s.minSigned, s.minSignedDeg);
    }


int main()
    {
    Stats fullSym { "domain [-180, 180]", -180.0, 180.0, 0.001 };
    Stats positive { "domain [0, 180]", 0.0, 180.0, 0.001 };

    runStats(fullSym);
    runStats(positive);

    printStats(fullSym);
    printStats(positive);

    return 0;
    }
