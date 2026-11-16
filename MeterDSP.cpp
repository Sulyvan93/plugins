#include "MeterDSP.h"
#include <numeric>

namespace trem {
void Biquad::coefficients(double B0, double B1, double B2, double A0, double A1, double A2) {
    b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0;
    a1 = A1 / A0; a2 = A2 / A0;
    clear();
}
void Biquad::highPass(double sr, double freq, double q) {
    const double w = 2*pi*freq/sr, c = std::cos(w), alpha = std::sin(w)/(2*q);
    coefficients((1+c)/2, -(1+c), (1+c)/2, 1+alpha, -2*c, 1-alpha);
}
void Biquad::highShelf(double sr, double freq, double q, double gainDb) {
    const double A = std::pow(10, gainDb/40.0), w = 2*pi*freq/sr;
    const double c = std::cos(w), s = std::sin(w);
    const double alpha = s/(2*q), root = 2*std::sqrt(A)*alpha;
    coefficients(A*((A+1)+(A-1)*c+root),
                 -2*A*((A-1)+(A+1)*c),
                  A*((A+1)+(A-1)*c-root),
                     (A+1)-(A-1)*c+root,
                  2*((A-1)-(A+1)*c),
                     (A+1)-(A-1)*c-root);
}
void InterSamplePeak::prepare(bool hq) {
    taps = hq ? 32 : 16; phases = hq ? 8 : 4; clear();
    // Causal, linear-phase, Hann-windowed sinc polyphase interpolator.
    // Only the metering path is delayed; the audio signal is untouched.
    for (int p=0; p<phases; ++p) {
        double total=0;
        for (int j=0; j<taps; ++j) {
            const double x = j - (taps/2 - 1) - (double)p/phases;
            const double sinc = std::abs(x)<1e-12 ? 1.0 : std::sin(pi*x)/(pi*x);
            const double window = 0.5 + 0.5*std::cos(2*pi*x/taps);
            table[p][j] = sinc * window;
            total += table[p][j];
        }
        for (int j=0; j<taps; ++j) table[p][j] /= total;
    }
}
double InterSamplePeak::process(double x) noexcept {
    samples[position] = x;
    double maxAbs = std::abs(x);
    for (int p=1; p<phases; ++p) {
        double value=0;
        for (int j=0; j<taps; ++j)
            value += samples[(position - j + maxTaps) % maxTaps] * table[p][j];
        maxAbs = std::max(maxAbs, std::abs(value));
    }
    position=(position+1)%maxTaps;
    return maxAbs;
}
void MeterEngine::Histogram::push(double energy, float lu) noexcept {
    const int i = std::clamp((int)std::floor((lu+100.0f)*2),0,histogramBins-1);
    ++count[i]; sum[i] += energy;
}
float MeterEngine::Histogram::integrated(int relativeGate) const noexcept {
    double energySum=0; uint64_t blockCount=0;
    for (int i=0;i<histogramBins;++i) {
        if (-100.0f+0.5f*i < -70.0f && relativeGate != 0) continue;
        energySum+=sum[i]; blockCount+=(uint64_t)count[i];
    }
    if (!blockCount) return -100;
    if (!relativeGate) return loudness(energySum / blockCount);
    const float relativeThreshold=loudness(energySum / blockCount)-relativeGate;
    energySum=0; blockCount=0;
    for (int i=0;i<histogramBins;++i) {
        const float l=-100.0f+0.5f*i;
        if (l >= -70.0f && l >= relativeThreshold) {
            energySum+=sum[i]; blockCount+=(uint64_t)count[i];
        }
    }
    return blockCount ? loudness(energySum/blockCount) : -100;
}
float MeterEngine::Histogram::lra() const noexcept {
    // EBU-style relative -20 LU gate of short-term loudness, then 10/95 percentiles.
    double weighted=0; uint64_t countAll=0;
    for (int i=60;i<histogramBins;++i) { // starts at -70 LUFS
        weighted+=sum[i]; countAll+=(uint64_t)count[i];
    }
    if (countAll<20) return 0;
    const float threshold=std::max(-70.0f, loudness(weighted/countAll)-20.0f);
    const int start=std::clamp((int)std::ceil((threshold+100)*2),0,histogramBins-1);
    uint64_t accepted=0;
    for(int i=start;i<histogramBins;++i) accepted+=(uint64_t)count[i];
    if(accepted<20) return 0;
    const auto percentile=[&](double fract) {
        const uint64_t target=(uint64_t)std::ceil(accepted*fract);
        uint64_t acc=0;
        for(int i=start;i<histogramBins;++i) { acc+=(uint64_t)count[i]; if(acc>=target) return -100.0f+0.5f*i; }
        return 40.0f;
    };
    return std::max(0.0f, percentile(0.95)-percentile(0.10));
}
void MeterEngine::prepare(double sr, int numberOfChannels, bool highQuality) {
    samplerate = std::max(8000.0, sr);
    nChannels = std::clamp(numberOfChannels,1,maxChannels);
    hopSamples = std::max(1,(int)std::round(samplerate*0.1));
    for (int c=0;c<maxChannels;++c) {
        // ITU-R BS.1770 style K-weighting approximated using a calibrated
        // high shelf + RLB high-pass with sample-rate-adaptive biquads.
        shelf[c].highShelf(samplerate,1681.974450955533,0.7071752369554196,3.999843853973347);
        highPass[c].highPass(samplerate,38.13547087602444,0.5003270373238773);
        interpolators[c].prepare(highQuality);
    }
    setSurroundWeights(nChannels>3 ? 3 : -1);
    reset();
}
void MeterEngine::setSurroundWeights(int lfeIndex, float rearWeight) noexcept {
    for(int c=0;c<maxChannels;++c) {
        weight[c]=(c>=4 && nChannels>=4) ? rearWeight : 1.0;
        if(c==lfeIndex) weight[c]=0;
    }
}
void MeterEngine::resetLoudness() noexcept {
    integratedHistogram.clear(); shortHistogram.clear();
    hopEnergy.fill(0); hopPointer=0; hopsAvailable=0;
    samplesInHop=0; accumEnergy=0; elapsed=0;
    result.momentary=result.shortTerm=result.integrated=-100;
    result.momentaryMax=result.shortTermMax=-100;
    result.lra=0; result.plr=result.psr=0;
    result.completedHops=0; result.elapsedSeconds=0;
}
void MeterEngine::reset() noexcept {
    resetLoudness();
    rmsPower.fill(0); vuPower.fill(0); ppmFollower.fill(0);
    crossPower=leftPower=rightPower=0;
    for(int c=0;c<maxChannels;++c) {
        shelf[c].clear(); highPass[c].clear(); interpolators[c].clear();
        result.peakDb[c]=result.maxPeakDb[c]=result.rmsDb[c]=result.maxRmsDb[c]=-100;
        result.truePeakDb[c]=-100;
    }
    result.truePeakMax=-100; result.phase=result.balance=0;
    result.vuLeft=result.vuRight=result.ppmLeft=result.ppmRight=-60;
}
void MeterEngine::completeHop() noexcept {
    hopEnergy[hopPointer]=accumEnergy/std::max(1,samplesInHop);
    hopPointer=(hopPointer+1)%30;
    hopsAvailable=std::min(30,hopsAvailable+1);
    auto average=[&](int requested) {
        const int count=std::min(hopsAvailable,requested);
        double sum=0;
        for(int i=0;i<count;++i) sum+=hopEnergy[(hopPointer+30-1-i)%30];
        return count ? sum/count : 0.;
    };
    const double momentaryEnergy=average(4), shortEnergy=average(30);
    result.momentary=loudness(momentaryEnergy);
    result.shortTerm=loudness(shortEnergy);
    result.momentaryMax=std::max(result.momentaryMax,result.momentary);
    result.shortTermMax=std::max(result.shortTermMax,result.shortTerm);
    if(hopsAvailable>=4) integratedHistogram.push(momentaryEnergy,result.momentary);
    if(hopsAvailable>=30) shortHistogram.push(shortEnergy,result.shortTerm);
    result.integrated=integratedHistogram.integrated(gateLu);
    result.lra=shortHistogram.lra();
    result.plr=result.truePeakMax > -90 && result.integrated > -90 ? result.truePeakMax-result.integrated : 0;
    result.psr=result.truePeakMax > -90 && result.shortTerm > -90 ? result.truePeakMax-result.shortTerm : 0;
    ++result.completedHops;
    accumEnergy=0; samplesInHop=0;
}
void MeterEngine::process(const float* const* channels, int numberOfChannels, int numSamples) noexcept {
    const int available=std::min(nChannels,numberOfChannels);
    if(numSamples<=0 || available<1) return;
    const double alphaRms=std::exp(-1/(samplerate*0.3));
    const double alphaPhase=std::exp(-1/(samplerate*0.3));
    const double attack=std::exp(-1/(samplerate*0.01));
    const double release=std::exp(-1/(samplerate*1.5));
    std::array<float,maxChannels> blockPeak{}, blockTP{};
    blockPeak.fill(-100); blockTP.fill(-100);
    for(int i=0;i<numSamples;++i) {
        double weighted=0, l=0,r=0;
        for(int c=0;c<available;++c) {
            const double x=(double)channels[c][i];
            if(c==0) l=x;
            if(c==1) r=x;
            const double k=highPass[c].process(shelf[c].process(x));
            weighted+=weight[c]*k*k;
            const double absX=std::abs(x);
            const float peakDb=toDb(absX);
            blockPeak[c]=std::max(blockPeak[c],peakDb);
            result.maxPeakDb[c]=std::max(result.maxPeakDb[c],peakDb);
            rmsPower[c]=alphaRms*rmsPower[c]+(1-alphaRms)*x*x;
            vuPower[c]=alphaRms*vuPower[c]+(1-alphaRms)*x*x;
            ppmFollower[c]= (absX>ppmFollower[c]?attack:release)*ppmFollower[c] +
                  (1-(absX>ppmFollower[c]?attack:release))*absX;
            result.rmsDb[c]=toDb(std::sqrt(rmsPower[c]));
            result.maxRmsDb[c]=std::max(result.maxRmsDb[c],result.rmsDb[c]);
            const float tp=toDb(interpolators[c].process(x));
            blockTP[c]=std::max(blockTP[c],tp);
            result.truePeakMax=std::max(result.truePeakMax,tp);
        }
        accumEnergy+=weighted;
        ++samplesInHop;
        if(available>=2) {
            leftPower=alphaPhase*leftPower+(1-alphaPhase)*l*l;
            rightPower=alphaPhase*rightPower+(1-alphaPhase)*r*r;
            crossPower=alphaPhase*crossPower+(1-alphaPhase)*l*r;
        }
        if(samplesInHop>=hopSamples) completeHop();
    }
    for(int c=0;c<available;++c) {result.peakDb[c]=blockPeak[c];result.truePeakDb[c]=blockTP[c];}
    result.channels=available;
    result.phase=(float)std::clamp(crossPower / std::sqrt(std::max(1e-20,leftPower*rightPower)),-1.,1.);
    result.balance=(float)((std::sqrt(rightPower)-std::sqrt(leftPower)) /
                     std::max(1e-9,std::sqrt(rightPower)+std::sqrt(leftPower)));
    result.vuLeft=toDb(std::sqrt(vuPower[0]));
    result.vuRight=toDb(std::sqrt(vuPower[available>=2?1:0]));
    result.ppmLeft=toDb(ppmFollower[0]);
    result.ppmRight=toDb(ppmFollower[available>=2?1:0]);
    elapsed+=(double)numSamples/samplerate;
    result.elapsedSeconds=(float)elapsed;
}
}
