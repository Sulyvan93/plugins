#pragma once
// Trem de Medir - independent real-time audio metering core.
// MIT license. Does not rely on the SSL source code or algorithms.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace trem {
constexpr int maxChannels = 12;
constexpr int histogramBins = 281; // -100 .. +40 LUFS, 0.5 LU per bin
constexpr double pi = 3.14159265358979323846;

inline float toDb(double x) { return (float)(20.0 * std::log10(std::max(1.0e-12, x))); }
inline float loudness(double meanWeightedSquare) {
    return meanWeightedSquare > 1.0e-15 ?
        (float)(-0.691 + 10.0 * std::log10(meanWeightedSquare)) : -100.0f;
}
struct MeterSnapshot {
    std::array<float, maxChannels> peakDb{};
    std::array<float, maxChannels> maxPeakDb{};
    std::array<float, maxChannels> rmsDb{};
    std::array<float, maxChannels> maxRmsDb{};
    std::array<float, maxChannels> truePeakDb{};
    float momentary = -100, shortTerm = -100, integrated = -100;
    float momentaryMax = -100, shortTermMax = -100, truePeakMax = -100;
    float lra = 0, plr = 0, psr = 0, phase = 0, balance = 0;
    float vuLeft = -60, vuRight = -60, ppmLeft = -60, ppmRight = -60;
    float elapsedSeconds = 0;
    int channels = 2;
    uint64_t completedHops = 0;
};

class Biquad {
public:
    void highPass(double sampleRate, double freq, double q);
    void highShelf(double sampleRate, double freq, double q, double gainDb);
    double process(double x) noexcept {
        const double y = b0*x + state1;
        state1 = b1*x - a1*y + state2;
        state2 = b2*x - a2*y;
        return y;
    }
    void clear() noexcept { state1 = state2 = 0; }
private:
    void coefficients(double B0, double B1, double B2, double A0, double A1, double A2);
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double state1 = 0, state2 = 0;
};

class InterSamplePeak {
public:
    void prepare(bool highQuality);
    double process(double sample) noexcept;
    void clear() noexcept { samples.fill(0.0); position = 0; }
private:
    static constexpr int maxTaps = 32, maxPhases = 8;
    std::array<double, maxTaps> samples{};
    std::array<std::array<double, maxTaps>, maxPhases> table{};
    int taps = 16, phases = 4, position = 0;
};

class MeterEngine {
public:
    void prepare(double sampleRate, int numberOfChannels, bool highQuality = false);
    void reset() noexcept;
    void resetLoudness() noexcept;
    // 0: off; 8: relative -8 LU; 10: relative -10 LU
    void setGate(int relativeGate) noexcept { gateLu = relativeGate; }
    void setSurroundWeights(int lfeChannelIndex, float rearWeight = 1.41f) noexcept;
    void process(const float* const* channels, int numberOfChannels, int numSamples) noexcept;
    MeterSnapshot snapshot() const noexcept { return result; } // Audio thread only; publish copy to GUI separately
private:
    struct Histogram {
        std::array<double, histogramBins> sum{};
        std::array<int, histogramBins> count{};
        void clear() noexcept { sum.fill(0); count.fill(0); }
        void push(double energy, float lu) noexcept;
        float integrated(int relativeGate) const noexcept;
        float lra() const noexcept;
    } integratedHistogram, shortHistogram;
    void completeHop() noexcept;
    double samplerate = 48000;
    int nChannels = 2, hopSamples = 4800, samplesInHop = 0, hopPointer = 0, hopsAvailable = 0;
    int gateLu = 10;
    std::array<Biquad, maxChannels> shelf{}, highPass{};
    std::array<InterSamplePeak, maxChannels> interpolators{};
    std::array<double, maxChannels> rmsPower{}, vuPower{}, ppmFollower{};
    std::array<double, maxChannels> weight{};
    std::array<double, 30> hopEnergy{};
    double accumEnergy = 0, crossPower = 0, leftPower = 0, rightPower = 0;
    double elapsed = 0;
    MeterSnapshot result{};
};
}
