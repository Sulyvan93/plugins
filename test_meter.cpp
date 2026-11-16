#include "../Source/MeterDSP.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

void runTone(trem::MeterEngine& m, int ch, float amp, double freq, double sr,
             double seconds, bool invertRight=false) {
    constexpr int block=256;
    std::array<std::vector<float>,trem::maxChannels> data;
    std::array<const float*,trem::maxChannels> ptr{};
    for(int c=0;c<ch;++c) { data[c].resize(block); ptr[c]=data[c].data(); }
    const int total=(int)(seconds*sr);
    for(int start=0;start<total;start+=block) {
        const int count=std::min(block,total-start);
        for(int i=0;i<count;++i)
            for(int c=0;c<ch;++c)
                data[c][i]=amp*(invertRight && c==1?-1.f:1.f)*std::sin(2*trem::pi*freq*(start+i)/sr);
        m.process(ptr.data(),ch,count);
    }
}
int main() {
    trem::MeterEngine meter;
    meter.prepare(48000,2);
    runTone(meter,2,0.5f,1000,48000,10);
    auto s=meter.snapshot();
    std::printf("Tone 1kHz @ -6.02 dBFS peak: integrated=%.2f LUFS, ST=%.2f, TP=%.2f, phase=%.3f, LRA=%.2f\n",
        s.integrated,s.shortTerm,s.truePeakMax,s.phase,s.lra);
    assert(s.integrated > -6.5f && s.integrated < -6.0f); // expected ~ -6.27 LUFS stereo
    assert(s.truePeakMax > -6.2f && s.truePeakMax < -5.6f);
    assert(s.phase > 0.99f);
    assert(s.lra < 1.1f);
    assert(s.completedHops >= 99);
    assert(s.rmsDb[0] > -9.3 && s.rmsDb[0] < -8.9); // -9.03 dBFS RMS

    // Integrated programme gating should not drag 5 seconds of digital silence into target.
    runTone(meter,2,0.0f,1000,48000,5);
    const auto withSilence=meter.snapshot();
    assert(std::abs(withSilence.integrated-s.integrated) < 1.f);

    meter.reset();
    auto r=meter.snapshot();
    assert(r.integrated==-100 && r.truePeakMax==-100);
    runTone(meter,2,.3f,1000,48000,2,true);
    r=meter.snapshot();
    assert(r.phase < -.99f);

    // Multichannel 7.1.4 with HQ true-peak (8x oversampling).
    meter.prepare(96000,12,true);
    runTone(meter,12,0.1f,440,96000,2);
    r=meter.snapshot();
    assert(r.integrated>-35 && r.integrated<0);
    assert(std::isfinite(r.integrated) && std::isfinite(r.truePeakMax));
    assert(r.channels==12);

    // Robustness at 44.1 kHz and low-level signals.
    meter.prepare(44100,1);
    runTone(meter,1,1.0e-5f,120,44100,1);
    r=meter.snapshot();
    assert(std::isfinite(r.momentary) && std::isfinite(r.truePeakMax));
    std::puts("DSP TESTS PASSED");
}
